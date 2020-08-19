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

#ifndef QUEUE
#define QUEUE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>

#include <assert.h>


// Basic structure to store the packet and some information (length)
typedef struct info info;
struct info{
    char* packet;
    uint16_t nread;
};

// Basic element of a chained queue
typedef struct cellQueue cellQueue;
struct cellQueue
{
    cellQueue* prec;
    cellQueue* suiv;
    info* inf;
};

// Queue structure, with pointers towards  the first and last element
typedef struct queue queue;
struct queue{
    cellQueue* first;
    cellQueue* last;
};

/* Function used to operate Queues */

queue* createQueue(); // allocate memory to create a queue structure

info* popQueue(queue* file); // pop the first element of the queue, if it empty return NULL. At the end, queue has one less element

info* popEndQueue(queue* file); // pop the last element of the queue, if it empty return NULL. At the end, queue has one less element

void pushQueue(queue* file,info* inf); // add the info element at the end of the queue. At the end, queue has one more element

void deallocateQueue(queue* file); // deallocate all the stuctures within the queue

void deallocateCellQueue(cellQueue* cellule);// deallocte the structure of the cell (but not the information contained inside!)

void deallocateInfo(info* inf); // deallocate the information stucture and the packet inside

#endif
