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

priorityScheduler *createPScheduler(double Lm, double Lr, double BW, double C)
{
	priorityScheduler *pScheduler = malloc(sizeof(priorityScheduler));
	pScheduler->SizeEF = 0;
	pScheduler->QueueEF = createQueue();
	pScheduler->sched = createBls(Lm, Lr, BW, C);
	return pScheduler;
}

info *popPScheduler(priorityScheduler * pScheduler)
{
	if (pScheduler->SizeEF > 0) {
		pScheduler->SizeEF -= 1;
		info *inf = popQueue(pScheduler->QueueEF);
		return inf;
	} else {
		info *inf = popBls(pScheduler->sched);
		return inf;
	}
}

void pushPScheduler(priorityScheduler * pScheduler, char *packet,
		    uint16_t nread)
{
#ifdef DSCP // Classification done using DSCP marking with EF: 1, AF: 2 else DF
	struct iphdr *iph;
	iph = (struct iphdr *)(packet);
	uint8_t dscp = (iph->tos) >> 2;
	if (dscp == 1) {
		info *inf = malloc(sizeof(info));
		char *pckt = malloc(sizeof(char) * nread);
		memcpy(pckt, packet, nread);
		inf->packet = pckt;
		inf->nread = nread;
		pushQueue(pScheduler->QueueEF, inf);
		pScheduler->SizeEF += 1;
	} else {
		pushBls(pScheduler->sched, packet, nread);
	}
#else	// Classification done using destination port EF: 20000, AF: 20001 else DF
	// As we could have UDP or TCP packets, we should use respectively struct udphdr or struct tcphdr
	// However as we manipulate only port numbers, we map UDP or TCP packets with struct tcphdr
	// This is only for testing as classification should be done with DSCP field
	struct tcphdr *tcpptr;
	tcpptr = (struct tcphdr *)(20 + packet);
	unsigned short tdest = ntohs(tcpptr->dest);
	if (tdest == 20000) {
		info *inf = malloc(sizeof(info));
		char *pckt = malloc(sizeof(char) * nread);
		memcpy(pckt, packet, nread);
		inf->packet = pckt;
		inf->nread = nread;
		pushQueue(pScheduler->QueueEF, inf);
		pScheduler->SizeEF += 1;
	} else {
		pushBls(pScheduler->sched, packet, nread);
	}
#endif
}

bool isEmptyPScheduler(priorityScheduler * pScheduler)
{
	return (pScheduler->SizeEF == 0) && (isEmptyBls(pScheduler->sched));
}

void deallocatePScheduler(priorityScheduler * pScheduler)
{
	deallocateBls(pScheduler->sched);
	deallocateQueue(pScheduler->QueueEF);
	free(pScheduler);
}
