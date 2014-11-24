/*
 * GraphStruct.h
 *
 *  Created on: Dec 17, 2013
 *      Author: Angen Zheng
 *      Email: angen.zheng@gmail.com
 */

#ifndef GRAPHSTRUCT_H_
#define GRAPHSTRUCT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct {
	int numParts;				/* total number of partitions of entire graph */
	int numVertices; 			/* total number of vertices own by the MPI repartitioning process */
	int numNbors; 				/* total number of neighbors of my vertices */

	int *active;				/* indicate if the vertex is active */
	int *vertexGIDs; 			/* global ID of each vertex */
	int *vertexSize;			/* object size in bytes of each vertex */
	int *partVector;			/* partition vector of vertex */

	int *nborIndex; 			/* nborIndex[i] is location of the first neighbor of vertex i,  (nbor = nborIndex[i + 1] -  nborIndex[i]) equals to the number of neighbors of vertex i*/
	int *nborGIDs; 				/* nborGIDs[nborIndex[i], ...., nborIndex[i] + nbor - 1] are neighbors of vertex i */
	int *nborProcs; 			/* the rank number of the MPI process owning the neighbor in nborGID.
								If the user uses the graph distributor to distribute the global graph across processes, the user doesn't need to worried about this field.
								Otherwise, the user need to fill the following array */

	float *vertexWgts;			/* vertex weights*/
	float *edgeWgts;			/* edge weights*/

	int *initialPartNo2CoreID;	/* map partNo to globalCoreID: globalCoreID = initialPartNo2CoreID[partNo]; globalCoreID= nodeID * numCoresPerNode + localCoreID; both nodeID and localCoreID start from 0.*/
} GraphStruct;

void graphInit(GraphStruct *graph, int num_vtxs, int num_edges, int num_parts);
void graphDeinit(GraphStruct *graph);
void graphPrint(GraphStruct graph);

int graphLoad(GraphStruct *global_graph, FILE * gdata);

#ifdef __cplusplus
}
#endif

#endif /* GRAPHSTRUCT_H_ */
