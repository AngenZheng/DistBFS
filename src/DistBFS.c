/*
 * BFS.c
 *
 *  Created on: Aug 31, 2014
 *      Author: Angen Zheng
 *      Email: angen.zheng@gmail.com
 */

#include <time.h>
#include <limits.h>
#include <mpi.h>
#include "BFS.h"
#include "ArrayList.h"

#define DUMMY_MSG_ENABLE 1
#define DUMMY_MSG_SIZE (1024 * 1024 * 4)

static int myRank;

int* BFS(MPI_Comm comm, GraphStruct localGraph, int srcLid, int srcRank){
	int i, j;
	/**************Compute the mapping of vertex GID to vertex lid*****************/
	int totalVtx;
	MPI_Allreduce(&(localGraph.numVertices), &totalVtx, 1, MPI_INT, MPI_SUM, comm);
	int *gid2lid = (int *) calloc(sizeof(int), totalVtx);
	for(i=0; i<localGraph.numVertices; i++){
		int gid = localGraph.vertexGIDs[i];
		gid2lid[gid] = i;
	}

	/*******Initialize sending and receiving buffer********/
	int *recvCount = (int *) malloc(sizeof(int) * localGraph.numParts);
	int **recvBuf = (int **) malloc(sizeof(int *) * localGraph.numParts);
	unsigned long *recvDummy = (unsigned long *) malloc(sizeof(unsigned long) * localGraph.numParts);
	unsigned long *sendDummy = (unsigned long *) malloc(sizeof(unsigned long) * localGraph.numParts);
	ArrayList_t **sendBuf = (ArrayList_t **) malloc(sizeof(ArrayList_t *) * localGraph.numParts);
	for(i=0; i<localGraph.numParts; i++){
		sendBuf[i] = listCreate();
	}
#ifdef DUMMY_MSG_ENABLE
	char *DUMMY_DATA = malloc(sizeof(char) * DUMMY_MSG_SIZE);
#endif
	/*****************************************************************/
	int *d = (int *) malloc(sizeof(int)* localGraph.numVertices);
	memset(d, -1, sizeof(int) * localGraph.numVertices);
	ArrayList_t * FS = listCreate();	//vertices currently active
	if(srcRank == myRank){
		d[srcLid] = 0;
		listAppend(FS, srcLid);
	}
	int level = 1;
	long ETPS = 0;
	int numActiveVertices = 0;
	do{
		//visiting neighbouring vertices in parallel
		ArrayList_t * NS = listCreate();	//vertices active in the next computation step
		memset(sendDummy, 0, sizeof(unsigned long) * localGraph.numParts);
		for(i=0; i<listLength(FS); i++){
			int lid = listGetIdx(FS, i);
			for(j=localGraph.nborIndex[lid]; j<localGraph.nborIndex[lid + 1]; j++){
				int nborGID = localGraph.nborGIDs[j];
				int owner = localGraph.nborProcs[j];
				if(owner == myRank){
					int lid = gid2lid[nborGID];
					if(d[lid] == -1){
						listAppend(NS, lid);
						d[lid] = level;
					}
				}else{
					listAppend(sendBuf[owner], nborGID);
					sendDummy[owner] += localGraph.edgeWgts[j];
				}
			}

			int numNbors = localGraph.nborIndex[lid + 1] - localGraph.nborIndex[lid];
			ETPS += numNbors;
		}
		listDestroy(FS);
		FS = NS;

		MPI_Request request;
		//sending newly visited nbors to their owners in parellel
		for(i=0; i<localGraph.numParts; i++){
			if(sendBuf[i]->length){
				MPI_Isend(sendBuf[i]->data, sendBuf[i]->length, MPI_INT, i, 1, comm, &request);
				MPI_Request_free(&request);
#ifdef DUMMY_MSG_ENABLE
				int tag = 2;
				long num = sendDummy[i] / DUMMY_MSG_SIZE;
				while(num > 0){
					MPI_Isend(DUMMY_DATA, DUMMY_MSG_SIZE, MPI_CHAR, i, tag++, comm, &request);
					MPI_Request_free(&request);
					num --;
				}
				num = sendDummy[i] % DUMMY_MSG_SIZE;
				if(num){
					MPI_Isend(DUMMY_DATA, num, MPI_CHAR, i, tag++, comm, &request);
					MPI_Request_free(&request);
				}
#endif
			}
		}
		for(i=0; i<localGraph.numParts; i++){//rank i gather sendCount[i] from each rank
			MPI_Gather(&(sendBuf[i]->length), 1, MPI_INT, recvCount, 1, MPI_INT, i, comm);
			MPI_Gather(sendDummy + i, 1, MPI_UNSIGNED_LONG, recvDummy, 1, MPI_UNSIGNED_LONG, i, comm);
		}
		for(i=0; i<localGraph.numParts; i++){
			recvBuf[i] = (int *) malloc(sizeof(int) * recvCount[i]);
			if(recvCount[i]){
				MPI_Recv(recvBuf[i], recvCount[i], MPI_INT, i, 1, comm, MPI_STATUS_IGNORE);
#ifdef DUMMY_MSG_ENABLE
				int tag = 2;
				long num = recvDummy[i] / DUMMY_MSG_SIZE;
				while(num > 0){
					MPI_Recv(DUMMY_DATA, DUMMY_MSG_SIZE, MPI_CHAR, i, tag++, comm, MPI_STATUS_IGNORE);
					num--;
				}
				num = recvDummy[i] % DUMMY_MSG_SIZE;
				if(num){
					MPI_Recv(DUMMY_DATA, num, MPI_CHAR, i, tag++, comm, MPI_STATUS_IGNORE);
				}
#endif
			}
		}

		//handling newly visited vertices and compute the distance
		for(i=0; i<localGraph.numParts; i++){
			for(j=0; j<recvCount[i]; j++){
				int gid = recvBuf[i][j];
				int lid = gid2lid[gid];
				if(d[lid] == -1){
					d[lid] = level;
					listAppend(FS, lid);
				}
			}
			free(recvBuf[i]);
		}
		numActiveVertices = listLength(FS);
		MPI_Allreduce(MPI_IN_PLACE, &numActiveVertices, 1, MPI_INT, MPI_SUM, comm);
		for(i=0; i<localGraph.numParts; i++){
			listClear(sendBuf[i]);
		}
//		if(myRank == 0) printf("step-%d\n", level);
		level ++;
	}while(numActiveVertices > 0);

	free(gid2lid);
	free(sendDummy);
	free(recvDummy);
	for(i=0; i<localGraph.numParts; i++){
		listDestroy(sendBuf[i]);
	}
	free(sendBuf);
	free(recvBuf);
	free(recvCount);
#ifdef DUMMY_MSG_ENABLE
	free(DUMMY_DATA);
#endif
	return d;
}

int main(int argc, char *argv[]) {
	MPI_Init(&argc, &argv);
	MPI_Comm comm = MPI_COMM_WORLD;
	char *fname = "graph.txt";
	int numParts;
	MPI_Comm_rank(comm, &myRank);
	MPI_Comm_size(comm, &numParts);


	GraphStruct localGraph;
	getSubGraph(comm, &localGraph, fname, numParts);
	srand(time(NULL));
	int srcRank = rand() % numParts;
	int srcLid = rand() % localGraph.numVertices;
	int *d = BFS(comm, localGraph, srcLid, srcRank);

	int i;
	int totalVtx;
	MPI_Allreduce(&(localGraph.numVertices), &totalVtx, 1, MPI_INT, MPI_SUM, comm);
	int *globalDist = (int *) calloc(sizeof(int), totalVtx);
	for(i=0; i<localGraph.numVertices; i++){
		 globalDist[localGraph.vertexGIDs[i]] = d[i];
	}
	free(d);
	MPI_Allreduce(MPI_IN_PLACE, globalDist, totalVtx, MPI_INT, MPI_MAX, comm);
	if(myRank == srcRank){
		for(i=0; i<totalVtx; i++){
			printf("d(%d, %d)=%d\n", localGraph.vertexGIDs[srcLid], i, globalDist[i]);
		}
	}
	free(globalDist);
	graphDeinit(&localGraph);
	MPI_Finalize();
	return 0;
}
