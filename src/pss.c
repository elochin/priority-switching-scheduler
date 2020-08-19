/* 	Implementation within TUN/TAP of the Priority Switching Scheduler (PSS),
    	see for details: http://oatao.univ-toulouse.fr/18448/
	If you use this code please cite the above paper.
    
	Copyright (C) 2018  
	Victor Perrier <victor.perrier@student.isae-supaero.fr> 
	Emmanuel Lochin <emmanuel.lochin@isae-supaero.fr>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/

#include "pscheduler.h"

// buffer for reading from TUN/TAP interface, must be >= 1500
#define BUFSIZE 2000
#define CLIENT 0
#define SERVER 1
#define PERROR(x) do { perror(x); exit(1); } while (0)

char *progname;
char connexion[] = "connexion";
extern char *optarg;

/*******************************************************/
/* tun_alloc: allocates or reconnects a tun/tap device */
/*******************************************************/
int tun_alloc(char *dev, int flags)
{

	struct ifreq ifr;
	int fd, err;
	char *clonedev = "/dev/net/tun";

	if ((fd = open(clonedev, O_RDWR)) < 0) {
		perror("Opening /dev/net/tun");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = flags;

	if (*dev) {
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		perror("ioctl(TUNSETIFF)");
		close(fd);
		return err;
	}

	strcpy(dev, ifr.ifr_name);

	return fd;
}

/**************************************************************************
 * usage: prints usage and exits.                                         *
 **************************************************************************/
void usage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "%s -i <ifacename> [-s|-c <serverIP>] [-p <port>]\n",
		progname);
	fprintf(stderr, "%s -h\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr,
		"-i <ifacename>: Name of interface to use (mandatory)\n");
	fprintf(stderr,
		"-s|-c <serverIP>: run in server mode (-s), or specify server address (-c <serverIP>) (mandatory)\n");
	fprintf(stderr,
		"-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 30001\n");
	fprintf(stderr, "-h: print this help text\n");
	fprintf(stderr, "-C: Capacity of the link in bit/s\n");
	fprintf(stderr, "-b: BW parameter for the AF class\n");
	fprintf(stderr, "-m: Lm parameter for the AF class\n");
	fprintf(stderr, "-r: Lr parameter for the AF class\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	info *inf;
	// PSS values
	double C = 0;
	double BW = 0;
	double Lm = 0;
	double Lr = 0;

	int tun_fd, option;
	int flags = IFF_TUN;
	char if_name[IFNAMSIZ] = "";
	int maxfd;

	char *buffer2;
	char buffer1[BUFSIZE];
	ssize_t nread, l;

	struct sockaddr_in sin, from, sout;
	char remote_ip[16] = "";	// dotted quad IP string
	unsigned short int port = 30001;
	int sock_fd;
	int cliserv = -1;	// must be specified on cmd line
	socklen_t fromlen, soutlen;
	progname = argv[0];

	// Check command line options
	while ((option = getopt(argc, argv, "i:sc:p:h:m:r:C:b:")) > 0) {
		switch (option) {
		case 'h':
			usage();
			break;
		case 'i':
			strncpy(if_name, optarg, IFNAMSIZ - 1);
			break;
		case 's':
			cliserv = SERVER;
			break;
		case 'c':
			cliserv = CLIENT;
			strncpy(remote_ip, optarg, 15);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'm':	// PSS Lm parameter
			if (atoi(optarg) >= 0) 
				Lm = atof(optarg);
			else
				printf("Parameter Lm not valid\n");
			break;
		case 'r':	// PSS Lr parameter
			if (atoi(optarg) >= 0)
				Lr = atof(optarg);
			else
				printf("Parameter Lr not valid\n");
			break;
		case 'b':	// PSS BW parameter
			if (atoi(optarg) >= 0)
				BW = atof(optarg);
			else
				printf("Parameter BW not valid\n");
			break;
		case 'C':	// PSS capacity parameter
			if (atoi(optarg) >= 0)
				C = atof(optarg);
			else
				printf("Parameter C not valid\n");
			break;
		default:
			printf("Unknown option %c\n", option);
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc > 0) {
		printf("Too many options!\n");
		usage();
	}

	if (*if_name == '\0') {
		printf("Must specify interface name!\n");
		usage();
	} else if (cliserv < 0) {
		printf("Must specify client or server mode!\n");
		usage();
	} else if ((cliserv == CLIENT) && (*remote_ip == '\0')) {
		printf("Must specify server address!\n");
		usage();
	}

	// TUN/TAP init
	if ((tun_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0) {
		printf("Error connecting to tun/tap interface %s!\n", if_name);
		exit(1);
	}

	// PSS init
	priorityScheduler *pSched = createPScheduler(Lm, Lr, BW, C);

	printf("Successfully connected to interface %s\n", if_name);

	if ((sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		PERROR("socket()");
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	if (bind(sock_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		PERROR("bind()");

	fromlen = sizeof(from);

	if (cliserv == SERVER) {
		while (1) {
			l = recvfrom(sock_fd, buffer1, BUFSIZE, 0,
				     (struct sockaddr *)&from, &fromlen);
			if (l < 0)
				PERROR("recvfrom()");
			if (strcmp(connexion, buffer1) == 0) {
				printf("Connexion depuis %s:%i demandée\n",
				       inet_ntoa(from.sin_addr),
				       ntohs(from.sin_port));
				break;
			}
		}
		l = sendto(sock_fd, connexion, sizeof(connexion), 0,
			   (struct sockaddr *)&from, fromlen);
		if (l < 0)
			PERROR("sendto()");
	} else {
		from.sin_family = AF_INET;
		from.sin_port = htons(port);
		inet_aton(remote_ip, &from.sin_addr);
		l = sendto(sock_fd, connexion, sizeof(connexion), 0,
			   (struct sockaddr *)&from, sizeof(from));
		if (l < 0)
			PERROR("sendto()");
		printf("Demande de connexion à %s:%i\n",
		       inet_ntoa(from.sin_addr), ntohs(from.sin_port));
		l = recvfrom(sock_fd, buffer1, BUFSIZE, 0,
			     (struct sockaddr *)&from, &fromlen);
		if (l < 0)
			PERROR("recvfrom()");
	}
	printf("Connection established with %s:%i\n", inet_ntoa(from.sin_addr),
	       ntohs(from.sin_port));

	while (1) {

		int ret;
		fd_set rd_set;
		fd_set write_set;

		maxfd = (tun_fd > sock_fd) ? tun_fd : sock_fd;

		FD_ZERO(&rd_set);
		FD_ZERO(&write_set);
		FD_SET(tun_fd, &rd_set);
		FD_SET(sock_fd, &rd_set);

		if (!isEmptyPScheduler(pSched)) {	//allow to dequeue only if there is something to dequeue
			FD_SET(sock_fd, &write_set);
		}

		ret = select(maxfd + 1, &rd_set, &write_set, NULL, NULL);

		if (ret < 0 && errno == EINTR) {
			continue;
		}

		if (ret < 0) {
			perror("select()");
			exit(1);
		}

		if (FD_ISSET(tun_fd, &rd_set)) {
			/* Datas coming from TUN inteface : it is redirected into the scheduler */
			nread = read(tun_fd, buffer1, BUFSIZE);
			if (nread < 0)
				PERROR("read()");
			pushPScheduler(pSched, buffer1, nread);

#ifdef DEBUG
			printf
			    ("tun->net : %d bytes received from TUN/TAP interface\n",
			     (int)nread);
#endif
		}

	  // Dequeue the next packet and send it to the network
		if (FD_ISSET(sock_fd, &write_set)) {
			inf = popPScheduler(pSched);	// Dequeue the next packet
			nread = inf->nread;
			buffer2 = inf->packet;
			if (sendto
			    (sock_fd, buffer2, nread, 0,
			     (struct sockaddr *)&from, fromlen) < 0)
				PERROR("sendto()");

#ifdef DEBUG
			printf("Send packet to %s:%i : %d bytes\n",
			       inet_ntoa(from.sin_addr), ntohs(from.sin_port),
			       (int)nread);
#endif

			// Deallocate the data packet
			deallocateInfo(inf);
		}

		// Data coming from the network, is redirected to the TUN/TAP interface 
		if (FD_ISSET(sock_fd, &rd_set)) {
			soutlen = sizeof(sout);
			l = recvfrom(sock_fd, buffer1, BUFSIZE, 0,
				     (struct sockaddr *)&sout, &soutlen);
			if (l < 0) {
				printf("%d", soutlen);
				printf("error with %s:%i\n",
				       inet_ntoa(sout.sin_addr),
				       ntohs(sout.sin_port));
				PERROR("recvfrom()");
			}
#ifdef DEBUG
			printf("Received packet from %s:%i : %d bytes\n",
			       inet_ntoa(sout.sin_addr), ntohs(sout.sin_port),
			       (int)l);
#endif
			if (write(tun_fd, buffer1, l) < 0)
				PERROR("write()");
		}

	}
	// Deallocate the scheduler
	deallocatePScheduler(pSched);
}
