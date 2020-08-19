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

#include "bls.h"

bls *createBls(double Lm, double Lr, double BW, double C)
{
	bls *sched = malloc(sizeof(bls));
	sched->SizeAF = 0;
	sched->SizeDF = 0;
	sched->Credit = Lr;
	sched->QueueAF = createQueue();
	sched->QueueDF = createQueue();
	sched->Lm = Lm;
	sched->Lr = Lr;
	sched->BW = BW;
	sched->C = C;
	sched->prio = true;
	sched->TimerDQ = getTime();
	sched->time_beginning = getTime();
#ifdef DUMP_STATS		//used to dump the credit through time in a file
	sched->dumpFile = fopen("credit_log.log", "w");
#endif
	return sched;
}

info *popBls(bls * sched)
{
	double time_now = getTime();
	double deltaT = time_now - sched->TimerDQ;
	if (deltaT > 0) {
		if (sched->SizeAF > 0) {
			sched->Credit =
			    MAX(sched->Credit -
				deltaT * (sched->BW) * (sched->C), 0);
		} else {
			sched->Credit =
			    MAX(sched->Credit -
				deltaT * (sched->BW) * (sched->C),
				MIN(sched->Credit, sched->Lr));
		}
		sched->TimerDQ = time_now;
		if (sched->Credit <= sched->Lr && !(sched->prio)) {
			sched->prio = true;
		}
	} else {
		sched->Credit =
		    MIN(sched->Credit - deltaT * (sched->BW) * (sched->C),
			sched->Lm);
		sched->TimerDQ = time_now;
	}

#ifdef DUMP_STATS		//used to dump the credit through time in a file
	fprintf(sched->dumpFile, "%.10f,%f\n",
		time_now - sched->time_beginning, sched->Credit);
#endif
	if (sched->prio && sched->SizeAF > 0) {
		info *inf = popQueue(sched->QueueAF);
		sched->TimerDQ = time_now + ((double)inf->nread) * 8 / sched->C;
		sched->SizeAF -= 1;
		sched->Credit =
		    MIN(sched->Lm,
			sched->Credit + ((double)inf->nread) * 8 * (1 -
								    sched->BW));
#ifdef DUMP_STATS		//used to dump the credit through time in a file
		fprintf(sched->dumpFile, "%.10f,%f\n",
			sched->TimerDQ - sched->time_beginning, sched->Credit);
#endif
		if (sched->Credit >= sched->Lm && sched->prio) {
			sched->prio = false;
		}
		return inf;
	} else if (sched->SizeDF > 0) {
		info *inf = popQueue(sched->QueueDF);
		sched->SizeDF -= 1;
		return inf;
	} else if (sched->SizeAF > 0) {
		info *inf = popQueue(sched->QueueAF);
		sched->TimerDQ = time_now + ((double)inf->nread) * 8 / sched->C;
		sched->SizeAF -= 1;
		sched->Credit =
		    MIN(sched->Lm,
			sched->Credit + ((double)inf->nread) * 8 * (1 -
								    sched->BW));
#ifdef DUMP_STATS		//used to dump the credit through time in a file
		fprintf(sched->dumpFile, "%.10f,%f\n",
			sched->TimerDQ - sched->time_beginning, sched->Credit);
#endif
		if (sched->Credit >= sched->Lm && sched->prio) {
			sched->prio = false;
		}
		return inf;
	} else {
		return NULL;
	}
}

void pushBls(bls * sched, char *packet, uint16_t nread)
{
	info *inf = malloc(sizeof(info));
	char *pckt = malloc(sizeof(char) * nread);
	memcpy(pckt, packet, nread);
	inf->packet = pckt;
	inf->nread = nread;
#ifdef DSCP	// Classification done using DSCP marking with EF: 1, AF: 2 else DF
	struct iphdr *iph;
	iph = (struct iphdr *)(packet);
	uint8_t dscp = (iph->tos) >> 2;
	if (dscp == 2) {
		pushQueue(sched->QueueAF, inf);
		sched->SizeAF += 1;
	} else {
		pushQueue(sched->QueueDF, inf);
		sched->SizeDF += 1;
	}
#else	// Classification done using destination port EF: 20000, AF: 20001 else DF
		  // As we could have UDP or TCP packets, we should use respectively struct udphdr or struct tcphdr
      // However as we manipulate only port numbers, we map UDP or TCP packets with struct tcphdr
			// This is only for testing as classification should be done with DSCP field
	struct tcphdr *tcpptr;
	tcpptr = (struct tcphdr *)(20 + packet);
	unsigned short tdest = ntohs(tcpptr->dest);
	if (tdest >= 20001) {
		pushQueue(sched->QueueAF, inf);
		sched->SizeAF += 1;
	} else {
		pushQueue(sched->QueueDF, inf);
		sched->SizeDF += 1;
	}
#endif
}

bool isEmptyBls(bls * sched)
{
	return (sched->SizeAF + sched->SizeDF == 0);
}

void deallocateBls(bls * sched)
{
	deallocateQueue(sched->QueueAF);
	deallocateQueue(sched->QueueDF);
#ifdef DUMP_STATS		//used to dump the credit through time in a file
	fclose(sched->dumpFile);
#endif
	free(sched);
} 

double getTime()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return ((double)(now.tv_sec)) + ((double)(now.tv_nsec)) / 1000000000;
}
