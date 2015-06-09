/*
 * TopoLBGraph.h
 *
 *  Created on: Dec 17, 2013
 *      Author: Angen Zheng
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
#include <zoltan.h>
//#include "BitVector.h"

//#define TIME_VECTOR_LENGTH 256
typedef ZOLTAN_ID_TYPE VERTEX_ID_TYPE;

typedef struct {
	int numVertices; 			/* number of vertices own by the MPI process locallly*/
	int numNbors; 				/* number of neighbors of my vertices */

	int *active;				/* indicate if the vertex is active */
//	VERTEX_ID_TYPE *vertexLIDs;	/* local id of each vertex */
	VERTEX_ID_TYPE *vertexGIDs; /* global ID of each vertex */
	int *vertexSize;			/* object size in bytes of each vertex */
	int *partVector;			/* partition vector of vertex */

	int *nborIndex; 			/* nborIndex[i] is location of the first neighbor of vertex i,  (nbor = nborIndex[i + 1] -  nborIndex[i]) equals to the number of neighbors of vertex i*/
	VERTEX_ID_TYPE *nborGIDs; 	/* nborGIDs[nborIndex[i], ...., nborIndex[i] + nbor - 1] are neighbors of vertex i */
	int *nborParts; 			/* the rank number of the MPI process owning the neighbor in nborGID.
								If the user uses the graph distributor to distribute the global graph across processes, the user doesn't need to worried about this field.
								Otherwise, the user need to fill the following array */

	float *vertexWgts;			/* vertex weights*/
	float *edgeWgts;			/* edge weights*/

//	int *initialPartNo2CoreID;	/* map partNo to globalCoreID: globalCoreID = initialPartNo2CoreID[partNo]; globalCoreID= nodeID * numCoresPerNode + localCoreID; both nodeID and localCoreID start from 0.*/


//	BitVector_t *vtxTimeVector;	/* indicate if the vertex is active during past 256 computation steps */
} GraphStruct;

void graphInit(GraphStruct *graph, int num_vtxs, int num_edges);
void graphInitVertexLists(GraphStruct *graph, int num_vtxs);
void graphInitEdgeLists(GraphStruct *graph, int num_nbors);
void graphDeinit(GraphStruct *graph);
void graphPrint(GraphStruct graph);

int graphLoad(GraphStruct *global_graph, FILE * gdata);

#ifdef __cplusplus
}
#endif

#endif /* GRAPHSTRUCT_H_ */
