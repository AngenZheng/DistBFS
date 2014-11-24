/*
 * BFS.h
 *
 *  Created on: Sep 27, 2014
 *      Author: Angen Zheng
 */

#ifndef BFS_H_
#define BFS_H_

#include <mpi.h>
#include "GraphStruct.h"

void graphDistribute(MPI_Comm comm, GraphStruct global_graph, GraphStruct * local_graph);
void getSubGraph(MPI_Comm comm, GraphStruct *sub_graph, char *fname, int numParts);



#endif /* BFS_H_ */
