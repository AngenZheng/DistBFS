/*
 * BFS.c
 *
 *  Created on: Aug 31, 2014
 *      Author: Angen Zheng
 */


#include <mpi.h>
#include <time.h>
#include <limits.h>

#include "TestUtil.h"
#include "IdList.h"

#define NUM_SRC_VTX 10

static int m_myRank;
static int m_numParts;
static DecompMethod m_dm;

typedef struct{
	int *recvCount;
	IdList_t **sendBuf;
	VERTEX_ID_TYPE **recvBuf;

	VERTEX_ID_TYPE *nborLids;	//mapping nborGids to their lids with respect to their owners
	Zoltan_DD_Directory *zDD;	//distirbuted data directory (mapping vertices to their owners mapping gids to their lids)
}BFSStruct_t;

static BFSStruct_t m_BFS;


static void randomSourceVtx(MPI_Comm comm, GraphStruct localGraph, int *srcPart, int *srcVtxLid){
	srand(time(NULL));
	if(m_myRank == 0){//randomly select a partition
		*srcPart = rand() % m_numParts;
	}
	MPI_Bcast(srcPart, 1, MPI_INT, 0, comm);

	if(m_myRank == *srcPart){//randomly select a vertex of the partition
		int lid, numNbors;
		do{
			lid = rand() % localGraph.numVertices;
			numNbors = localGraph.nborIndex[lid + 1] - localGraph.nborIndex[lid];
			if(numNbors > 0) {
				*srcVtxLid = lid;
				return;
			}
		}while(1);
	}
}


static void preBFS(MPI_Comm comm, GraphStruct localGraph){
	int i;
	VERTEX_ID_TYPE *vtxLids = (VERTEX_ID_TYPE *) malloc(sizeof(VERTEX_ID_TYPE) * localGraph.numVertices);
	for(i=0; i<localGraph.numVertices; i++){
		vtxLids[i] = i;
	}

	Zoltan_DD_Create(
			&(m_BFS.zDD),
			comm,
			1, 						//gid consists of 1 integer
			1,						//lid consists of 1 integer
			0,						//ignore user data
			localGraph.numVertices,	//hash table size
			0						//debug level
			);
	Zoltan_DD_Update(m_BFS.zDD,
			localGraph.vertexGIDs,		/* gids to updte */
			vtxLids,					/* corresponding lids */
			0,							/* corresponding user data*/
			localGraph.partVector,		/* partition vector*/
			localGraph.numVertices		/* # of gids to update*/
			);
	free(vtxLids);

	m_BFS.nborLids = (VERTEX_ID_TYPE *) malloc(sizeof(VERTEX_ID_TYPE) * localGraph.numNbors);
	Zoltan_DD_Find(m_BFS.zDD,
			localGraph.nborGIDs,
			m_BFS.nborLids,
			NULL,
			localGraph.nborParts,
			localGraph.numNbors,
			NULL);


	/****************************************************/
	//		Initialize sending and receiving buffer		//
	/****************************************************/
	m_BFS.recvCount = (int *) malloc(sizeof(int) * m_numParts);
	m_BFS.sendBuf = (IdList_t **) malloc(sizeof(IdList_t *) * m_numParts);
	m_BFS.recvBuf = (VERTEX_ID_TYPE **) malloc(sizeof(VERTEX_ID_TYPE *) * m_numParts);
	for(i=0; i<m_numParts; i++){
		m_BFS.sendBuf[i] = idListCreate();
	}
}

static void postBFS(GraphStruct localGraph){
	Zoltan_DD_Destroy(&(m_BFS.zDD));
	free(m_BFS.nborLids);

	int i;
	for(i=0; i<m_numParts; i++){
		idListDestroy(m_BFS.sendBuf[i]);
	}
	free(m_BFS.sendBuf);
	free(m_BFS.recvBuf);
	free(m_BFS.recvCount);
}


static void doBFS(MPI_Comm comm, GraphStruct localGraph){
	int i, j;
	int level;
	int srcPart;
	int srcVtxLid;
	int curSrc = 0;

	double JET = 0;				//job execution time
	unsigned long ETPS = 0;		//edges traversed per second
	int *d = (int *) malloc(sizeof(int) * localGraph.numVertices);	//d[i] indicates the distance to the ith vertex of the partition
	for (curSrc=0; curSrc<NUM_SRC_VTX; curSrc++){
		memset(d, -1, sizeof(int) * localGraph.numVertices);
		IdList_t *FS = idListCreate();

		//randomly pick one vertex as source vertex
		randomSourceVtx(comm, localGraph, &srcPart, &srcVtxLid);
		if(m_myRank == srcPart){
			d[srcVtxLid] = 0;
			idListAppend(FS, srcVtxLid);
		}

		level = 1;
		int numActiveVertices = 0;
		double JET_ST = MPI_Wtime();
		do{
			//visiting neighbouring vertices of vertices in FS in parallel
			IdList_t * NS = idListCreate();
			for(i=0; i<idListLength(FS); i++){
				VERTEX_ID_TYPE vtxLid = idListGetIdx(FS, i);
				localGraph.active[vtxLid] = 0;	//becomes inactive in the next superstep
				for(j=localGraph.nborIndex[vtxLid]; j<localGraph.nborIndex[vtxLid + 1]; j++){
					VERTEX_ID_TYPE nborLid = m_BFS.nborLids[j];
					int owner = localGraph.nborParts[j];
					if(owner == m_myRank){
						if(d[nborLid] == -1){
							idListAppend(NS, nborLid);
							localGraph.active[nborLid] = 1;	//become active in the next superstep
							d[nborLid] = level;
						}
					}else{
						idListAppend(m_BFS.sendBuf[owner], nborLid);
					}
				}
				ETPS += localGraph.nborIndex[vtxLid + 1] - localGraph.nborIndex[vtxLid];
			}
			idListDestroy(FS);

			//sending newly visited nbors to their owners in parallel
			MPI_Request request;
			for(i=0; i<m_numParts; i++){
				if(m_BFS.sendBuf[i]->length){
					MPI_Isend(m_BFS.sendBuf[i]->data, m_BFS.sendBuf[i]->length * sizeof(VERTEX_ID_TYPE), MPI_BYTE, i, 1, comm, &request);
					MPI_Request_free(&request);
				}
			}
			for(i=0; i<m_numParts; i++){//rank i gather sendCount[i] from each rank
				MPI_Gather(&(m_BFS.sendBuf[i]->length), 1, MPI_INT, m_BFS.recvCount, 1, MPI_INT, i, comm);
			}
			for(i=0; i<m_numParts; i++){
				m_BFS.recvBuf[i] = (VERTEX_ID_TYPE *) malloc(sizeof(VERTEX_ID_TYPE) * m_BFS.recvCount[i]);
				if(m_BFS.recvCount[i]){
					MPI_Recv(m_BFS.recvBuf[i], m_BFS.recvCount[i] * sizeof(VERTEX_ID_TYPE), MPI_BYTE, i, 1, comm, MPI_STATUS_IGNORE);
				}
			}

			//handling newly visited vertices and compute the distance
			for(i=0; i<m_numParts; i++){
				for(j=0; j<m_BFS.recvCount[i]; j++){
					VERTEX_ID_TYPE lid = m_BFS.recvBuf[i][j];
					if(d[lid] == -1){
						d[lid] = level;
						idListAppend(NS, lid);
						localGraph.active[lid] = 1;
					}
				}
				free(m_BFS.recvBuf[i]);
				idListClear(m_BFS.sendBuf[i]);
			}

			FS = NS;
			numActiveVertices = idListLength(FS);
			MPI_Allreduce(MPI_IN_PLACE, &numActiveVertices, 1, MPI_INT, MPI_SUM, comm);	//server as a global barrier

			level ++;
		}while(numActiveVertices > 0);
		JET += MPI_Wtime() - JET_ST;
		idListDestroy(FS);
	}
	free(d);

	MPI_Allreduce(MPI_IN_PLACE, &JET, 1, MPI_DOUBLE, MPI_MAX, comm);
	MPI_Allreduce(MPI_IN_PLACE, &ETPS, 1, MPI_UNSIGNED_LONG, MPI_SUM, comm);
	if(m_myRank == 0){
		printf("%s\t %d\t %f\n",
				strDecompMethod(m_dm),
				m_numParts,
				ETPS / JET / 1000 / 1000
				);
	}
}

static void BFS(MPI_Comm comm, char *fname, DecompMethod dm){
	m_dm = dm;
	MPI_Comm_rank(comm, &m_myRank);
	MPI_Comm_size(comm, &m_numParts);

	GraphStruct localGraph;
	getSubGraph(comm, &localGraph, fname, m_numParts, dm);
	preBFS(comm, localGraph);
	doBFS(comm, localGraph);
	postBFS(localGraph);
	graphDeinit(&localGraph);
}

int main(int argc, char *argv[]){

	MPI_Init(&argc, &argv);
	MPI_Comm comm = MPI_COMM_WORLD;

	BFS(comm, "wave.data", HP);

	MPI_Finalize();
	return 0;
}
