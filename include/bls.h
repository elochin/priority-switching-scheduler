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

#ifndef BLS
#define BLS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <assert.h>


#include <linux/if_ether.h>
#include <net/ethernet.h>


#include "queue.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct bls bls;


//stucture of the BLS, with all internal variables
struct bls
{
    int SizeAF;
    int SizeDF;
    queue* QueueAF;
    queue* QueueDF;
    double Credit;
    double Lm;
    double Lr;
    double BW;
    double C;
    bool prio;
    double TimerDQ;
    double time_beginning;
#ifdef DUMP_STATS //used to dump the credit through time in a file
    FILE* dumpFile;
#endif
};

/* Function used to operate BLS */


bls* createBls(double Lm,double Lr,double BW,double C); // allocate memory to create a bls structure

info* popBls(bls* scheduler);// pop the next element to dequeue (following BLS algorithm)

void pushBls(bls* scheduler,char* packet,uint16_t nread); // add an element to the BLS, the classification between AF and DF is done here

bool isEmptyBls(bls* scheduler); // return a boolean indicating if the BLS is empty

void deallocateBls(bls* scheduler); // deallocate the BLS scheduler, and all variables inside

double getTime(); // get the time

#endif
