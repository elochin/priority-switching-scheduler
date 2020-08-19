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

#include "queue.h"

queue *createQueue()
{
	queue *file = malloc(sizeof(queue));
	file->first = NULL;
	file->last = NULL;
	return file;
}

info *popQueue(queue * file)
{
	if (file->first == NULL) {
		return NULL;
	} else if (file->first == file->last) {
		info *inf = file->first->inf;
		deallocateCellQueue(file->first);
		file->first = NULL;
		file->last = NULL;
		return inf;
	} else {
		info *inf = file->first->inf;
		file->first = file->first->suiv;
		deallocateCellQueue(file->first->prec);
		file->first->prec = NULL;
		return inf;
	}
}

info *popEndQueue(queue * file)
{
	if (file->last == NULL) {
		return NULL;
	} else if (file->first == file->last) {
		info *inf = file->first->inf;
		deallocateCellQueue(file->first);
		file->first = NULL;
		file->last = NULL;
		return inf;
	} else {
		info *inf = file->last->inf;
		file->last = file->last->prec;
		deallocateCellQueue(file->last->suiv);
		file->last->suiv = NULL;
		return inf;
	}
}

void pushQueue(queue * file, info * inf)
{
	cellQueue *newCell = malloc(sizeof(cellQueue));
	if (file->last == NULL) {
		file->first = newCell;
		file->last = newCell;
		newCell->inf = inf;
		newCell->suiv = NULL;
		newCell->prec = NULL;
	} else {
		newCell->prec = file->last;
		newCell->suiv = NULL;
		newCell->inf = inf;
		file->last->suiv = newCell;
		file->last = newCell;
	}
}

void deallocateQueue(queue * file)
{
	cellQueue *currentCell = file->first;
	cellQueue *tempCell;
	while (currentCell != NULL) {
		tempCell = currentCell->suiv;
		deallocateCellQueue(currentCell);
		currentCell = tempCell;
	}
	free(file);
}

void deallocateCellQueue(cellQueue * cellule)
{
	free(cellule);
}

void deallocateInfo(info * inf)
{
	free(inf->packet);
	free(inf);
}
