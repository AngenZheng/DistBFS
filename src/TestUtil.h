/*
 * TopoFMUtil.h
 *
 *  Created on: Apr 28, 2015
 *      Author: Angen Zheng
 */

#ifndef _TOPOFMUTIL_H_
#define _TOPOFMUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <mpi.h>
#include "GraphStruct.h"

typedef enum {HP, RP, LDG, METIS} DecompMethod;

char* strDecompMethod(DecompMethod dm);
void getSubGraph(MPI_Comm comm, GraphStruct *sub_graph, char *fname, int numParts, DecompMethod dt);

//void TopoEval(MPI_Comm comm, GraphStruct localGraph, float *m_partNetCommCost, int *curPartVector, int repartFeq);

/*
 * Distribute the global communication graph evenly across each rank
 * */
void graphDistributeV1(MPI_Comm comm, GraphStruct global_graph, GraphStruct * local_graph);
//distribute the graph according to the partition number (rank-i responsible for partition i);
void graphDistributeV2(MPI_Comm comm, GraphStruct global_graph, GraphStruct * local_graph);

void graphSend(MPI_Comm comm, GraphStruct sub_graph, int dest_proc);
void graphRecv(MPI_Comm comm, GraphStruct * local_graph, int sender);

//void graphIsend(MPI_Comm comm, GraphStruct sub_graph, int dest_proc);
//void graphIRecv(MPI_Comm comm, GraphStruct *local_graph, int sender);

#ifdef __cplusplus
}
#endif

#endif /* TOPOFMUTIL_H_ */
