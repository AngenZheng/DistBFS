/*
 * TopoFMUtil.c
 *
 *  Created on: Apr 28, 2015
 *      Author: Angen Zheng
 */

#include <metis.h>
#include "TestUtil.h"

static char *strDM[] = {"HP", "RP", "LDG", "METIS"};
char* strDecompMethod(DecompMethod dm){
	return strDM[dm];
}


/*
 * Distribute the global communication graph evenly across each rank
 * */
void graphDistributeV1(MPI_Comm comm, GraphStruct global_graph, GraphStruct * local_graph){
	int num_procs;
	MPI_Comm_size(comm, &num_procs);

	GraphStruct * sub_graphs = (GraphStruct *) malloc(sizeof(GraphStruct) * num_procs);
	//creating sub_graphs
	int i, j, proc_id;
	VERTEX_ID_TYPE vtx_lid, vtx_gid;
	int nnbors, nbor_index, nbor_proc;
	for (i = 0; i < num_procs; i++) {
		sub_graphs[i].numVertices = 0;
		sub_graphs[i].numNbors = 0;
	}

	int num_vtxs = global_graph.numVertices / num_procs;
	if(global_graph.numVertices % num_procs != 0){
		//evenly distribute last extra nodes across all processes
		num_vtxs ++;
	}

	for (i = 0; i < global_graph.numVertices; i++) {
		proc_id = (global_graph.vertexGIDs[i]) / num_vtxs;
		sub_graphs[proc_id].numVertices++;
		sub_graphs[proc_id].numNbors += (global_graph.nborIndex[i + 1]
				- global_graph.nborIndex[i]);
	}

	for (i = 0; i < num_procs; i++) {
		graphInit(&sub_graphs[i], sub_graphs[i].numVertices, sub_graphs[i].numNbors);
		sub_graphs[i].numVertices = 0;
		sub_graphs[i].numNbors = 0;
//		memcpy(sub_graphs[i].partPlacement, global_graph.partPlacement, (global_graph.numParts * 5 + 1) * sizeof(char));
//		memcpy(sub_graphs[i].initialPartNo2CoreID, global_graph.initialPartNo2CoreID, global_graph.numParts * sizeof(int));
//		memcpy(sub_graphs[i].initialCoreID2PartNo, global_graph.initialCoreID2PartNo, global_graph.numParts * sizeof(int)); //TODO assume one partitoin per core
	}

	for (i = 0; i < global_graph.numVertices; i++) {
		vtx_gid = (int) global_graph.vertexGIDs[i];
		proc_id = vtx_gid / num_vtxs;
		vtx_lid = sub_graphs[proc_id].numVertices ++;
		sub_graphs[proc_id].vertexGIDs[vtx_lid] = vtx_gid;
		sub_graphs[proc_id].vertexSize[vtx_lid] = global_graph.vertexSize[i];
		sub_graphs[proc_id].vertexWgts[vtx_lid] = global_graph.vertexWgts[i];
		sub_graphs[proc_id].partVector[vtx_lid] = global_graph.partVector[i];

		nnbors = global_graph.nborIndex[i + 1] - global_graph.nborIndex[i];
		sub_graphs[proc_id].nborIndex[vtx_lid + 1] = sub_graphs[proc_id].nborIndex[vtx_lid] + nnbors;
		for (j = global_graph.nborIndex[i]; j < global_graph.nborIndex[i + 1]; j++) {
			nbor_index = sub_graphs[proc_id].numNbors ++;
			nbor_proc = global_graph.nborGIDs[j] / num_vtxs;

			sub_graphs[proc_id].nborGIDs[nbor_index] = global_graph.nborGIDs[j];
			sub_graphs[proc_id].nborParts[nbor_index] = nbor_proc;
			sub_graphs[proc_id].edgeWgts[nbor_index] = global_graph.edgeWgts[j];
		}
	}
	*local_graph = sub_graphs[0];

	//sending sub_graphs to processes
	for (i = 1; i < num_procs; i++) {
		graphSend(comm, sub_graphs[i], i);
		graphDeinit(&sub_graphs[i]);
	}
	free(sub_graphs);
	/* signal all procs it is OK to go on */
	int ack = 0;
	for (i = 1; i < num_procs; i++) {
		MPI_Send(&ack, 1, MPI_INT, i, 0, comm);
	}
}

//distribute the graph according to the partition number (rank-i responsible for partition i);
void graphDistributeV2(MPI_Comm comm, GraphStruct global_graph, GraphStruct * local_graph){
	int num_procs;
	MPI_Comm_size(comm, &num_procs);

	int i, j, proc_id;
	VERTEX_ID_TYPE vtx_lid;
	int nnbors, nbor_index, nbor_proc;
	GraphStruct *sub_graphs = (GraphStruct *) malloc(sizeof(GraphStruct) * num_procs);
	for (i = 0; i < num_procs; i++) {
		sub_graphs[i].numVertices = 0;
		sub_graphs[i].numNbors = 0;
	}
	int *global_part_vector = malloc(sizeof(int) * global_graph.numVertices);
	for (i = 0; i < global_graph.numVertices; i++) {
		proc_id = global_graph.partVector[i];
		sub_graphs[proc_id].numVertices++;
		sub_graphs[proc_id].numNbors += (global_graph.nborIndex[i + 1] - global_graph.nborIndex[i]);
		global_part_vector[global_graph.vertexGIDs[i]] = global_graph.partVector[i];
	}

	for (i = 0; i < num_procs; i++) {
//		printf("part-%d %d %d\n", i, sub_graphs[i].numVertices, sub_graphs[i].numNbors);
		graphInit(&sub_graphs[i], sub_graphs[i].numVertices, sub_graphs[i].numNbors);
		sub_graphs[i].numVertices = 0;
		sub_graphs[i].numNbors = 0;
//		memcpy(sub_graphs[i].partPlacement, global_graph.partPlacement, (global_graph.numParts * 5 + 1) * sizeof(char));
//		memcpy(sub_graphs[i].initialPartNo2NodeID, global_graph.initialPartNo2NodeID, global_graph.numParts * sizeof(int));
//		memcpy(sub_graphs[i].initialPartNo2CoreID, global_graph.initialPartNo2CoreID, global_graph.numParts * sizeof(int));
//		memcpy(sub_graphs[i].initialCoreID2PartNo, global_graph.initialCoreID2PartNo, global_graph.numParts * sizeof(int)); //TODO assume one partition per core
	}

	for (i = 0; i < global_graph.numVertices; i++) {
		proc_id = global_graph.partVector[i];
		vtx_lid = sub_graphs[proc_id].numVertices ++;
		sub_graphs[proc_id].vertexGIDs[vtx_lid] = global_graph.vertexGIDs[i];
		sub_graphs[proc_id].vertexSize[vtx_lid] = global_graph.vertexSize[i];
		sub_graphs[proc_id].vertexWgts[vtx_lid] = global_graph.vertexWgts[i];
		sub_graphs[proc_id].partVector[vtx_lid] = global_graph.partVector[i];

		nnbors = global_graph.nborIndex[i + 1] - global_graph.nborIndex[i];
		sub_graphs[proc_id].nborIndex[vtx_lid + 1] = sub_graphs[proc_id].nborIndex[vtx_lid] + nnbors;
		for (j = global_graph.nborIndex[i]; j < global_graph.nborIndex[i + 1];
				j++) {
			nbor_index = sub_graphs[proc_id].numNbors ++;
			nbor_proc = global_part_vector[global_graph.nborGIDs[j]];

			sub_graphs[proc_id].nborParts[nbor_index] = nbor_proc;
			sub_graphs[proc_id].nborGIDs[nbor_index] = global_graph.nborGIDs[j];
			sub_graphs[proc_id].edgeWgts[nbor_index] = global_graph.edgeWgts[j];
		}
	}
	free(global_part_vector);
	*local_graph = sub_graphs[0];

	for (i = 1; i < num_procs; i++) {
		graphSend(comm, sub_graphs[i], i);
		graphDeinit(&sub_graphs[i]);
	}
	free(sub_graphs);

	int ack = 0;
	for (i = 1; i < num_procs; i++) {
		MPI_Send(&ack, 1, MPI_INT, i, 0, comm);
	}
}

//void graphIsend(MPI_Comm comm, GraphStruct sub_graph, int dest_proc) {
//	int send_count[3];
//	int ack = 0, count_tag = 10, id_tag = 15;
//
//	send_count[0] = sub_graph.numVertices;
//	send_count[1] = sub_graph.numNbors;
//	send_count[2] = sub_graph.numParts;
//
//	MPI_Request req;
//	MPI_Isend(send_count, 3, MPI_INT, dest_proc, count_tag, comm, &req);
//	MPI_Request_free(&req);
//
//	if (send_count[0] > 0) {
//		MPI_Isend(sub_graph.vertexGIDs, send_count[0] * sizeof(VERTEX_ID_TYPE), MPI_BYTE, dest_proc, id_tag, comm, &req);
//		MPI_Request_free(&req);
//		MPI_Isend(sub_graph.nborIndex, send_count[0] + 1, MPI_INT, dest_proc, id_tag + 1, comm, &req);
//		MPI_Request_free(&req);
//		MPI_Isend(sub_graph.partVector, send_count[0], MPI_INT, dest_proc, id_tag + 2, comm, &req);
//		MPI_Request_free(&req);
//		MPI_Isend(sub_graph.vertexSize, send_count[0], MPI_INT, dest_proc, id_tag + 3, comm, &req);
//		MPI_Request_free(&req);
//		MPI_Isend(sub_graph.vertexWgts, send_count[0], MPI_FLOAT, dest_proc, id_tag + 4, comm, &req);
//		MPI_Request_free(&req);
//
//		if (send_count[1] > 0) {
//			MPI_Isend(sub_graph.nborGIDs, send_count[1] * sizeof(VERTEX_ID_TYPE), MPI_BYTE, dest_proc, id_tag + 5, comm, &req);
//			MPI_Request_free(&req);
//			MPI_Isend(sub_graph.nborParts, send_count[1], MPI_INT, dest_proc, id_tag + 6, comm, &req);
//			MPI_Request_free(&req);
//			MPI_Isend(sub_graph.edgeWgts, send_count[1], MPI_FLOAT, dest_proc, id_tag + 7, comm, &req);
//			MPI_Request_free(&req);
//		}
//	}
//	//signal the receiver to continue
//	MPI_Isend(&ack, 1, MPI_INT, dest_proc, 0, comm, &req);
//	MPI_Request_free(&req);
//}
///* The counterpart of graphIsend function */
//void graphIRecv(MPI_Comm comm, GraphStruct *local_graph, int sender){
//	int send_count[3];
//	int ack = 0, count_tag = 10, id_tag = 15;
//
//	MPI_Recv(send_count, 3, MPI_INT, sender, count_tag, comm, MPI_STATUS_IGNORE);
//	if (send_count[0] > 0) {
//		graphInit(local_graph, send_count[0], send_count[1], send_count[2]);
//
//		MPI_Recv(local_graph->vertexGIDs, send_count[0] * sizeof(VERTEX_ID_TYPE), MPI_BYTE, sender, id_tag, comm, MPI_STATUS_IGNORE);
//		MPI_Recv(local_graph->nborIndex, send_count[0] + 1, MPI_INT, sender, id_tag + 1, comm, MPI_STATUS_IGNORE);
//		MPI_Recv(local_graph->partVector, send_count[0], MPI_INT, sender, id_tag + 2, comm, MPI_STATUS_IGNORE);
//		MPI_Recv(local_graph->vertexSize, send_count[0], MPI_INT, sender, id_tag + 3, comm, MPI_STATUS_IGNORE);
//		MPI_Recv(local_graph->vertexWgts, send_count[0], MPI_FLOAT, sender, id_tag + 4, comm, MPI_STATUS_IGNORE);
//		if (send_count[1] > 0) {
//			MPI_Recv(local_graph->nborGIDs, send_count[1] * sizeof(VERTEX_ID_TYPE), MPI_BYTE, sender, id_tag + 5, comm, MPI_STATUS_IGNORE);
//			MPI_Recv(local_graph->nborParts, send_count[1], MPI_INT, sender, id_tag + 6, comm, MPI_STATUS_IGNORE);
//			MPI_Recv(local_graph->edgeWgts, send_count[1], MPI_FLOAT, sender, id_tag + 7, comm, MPI_STATUS_IGNORE);
//		}
//	}
//	MPI_Recv(&ack, 1, MPI_INT, sender, 0, comm, MPI_STATUS_IGNORE);
//}


void graphSend(MPI_Comm comm, GraphStruct sub_graph, int dest_proc) {
//	int num_procs;
//	MPI_Comm_size(comm, &num_procs);

	int send_count[2];
	MPI_Status status;
	int ack = 0, ack_tag = 5, count_tag = 10, id_tag = 15;

	send_count[0] = sub_graph.numVertices;
	send_count[1] = sub_graph.numNbors;

	MPI_Send(send_count, 2, MPI_INT, dest_proc, count_tag, comm);
	MPI_Recv(&ack, 1, MPI_INT, dest_proc, ack_tag, comm, &status);

	if (send_count[0] > 0) {
		MPI_Send(sub_graph.vertexGIDs, send_count[0] * sizeof(VERTEX_ID_TYPE), MPI_BYTE,
				dest_proc, id_tag, comm);
		MPI_Send(sub_graph.nborIndex, send_count[0] + 1, MPI_INT, dest_proc,
				id_tag + 1, comm);
		MPI_Send(sub_graph.partVector, send_count[0], MPI_INT, dest_proc,
				id_tag + 2, comm);
		MPI_Send(sub_graph.vertexSize, send_count[0], MPI_INT, dest_proc,
				id_tag + 3, comm);
		MPI_Send(sub_graph.vertexWgts, send_count[0], MPI_FLOAT, dest_proc,
				id_tag + 4, comm);

		if (send_count[1] > 0) {
			MPI_Send(sub_graph.nborGIDs, send_count[1] * sizeof(VERTEX_ID_TYPE), MPI_BYTE,
					dest_proc, id_tag + 5, comm);
			MPI_Send(sub_graph.nborParts, send_count[1], MPI_INT, dest_proc,
					id_tag + 6, comm);
			MPI_Send(sub_graph.edgeWgts, send_count[1], MPI_FLOAT, dest_proc,
					id_tag + 7, comm);
		}
//		if(send_count[2] > 0){
//			MPI_Send(sub_graph.partPlacement, sub_graph.numParts * 5, MPI_CHAR, dest_proc, id_tag + 8, comm);
//		}
//		MPI_Send(sub_graph.initialPartNo2NodeID, sub_graph.numParts, MPI_INT, dest_proc, id_tag + 8, comm);
//		MPI_Send(sub_graph.initialPartNo2CoreID, sub_graph.numParts, MPI_INT, dest_proc, id_tag + 9, comm);
//		MPI_Send(sub_graph.initialCoreID2PartNo, sub_graph.numParts, MPI_INT, dest_proc, id_tag + 10, comm); //TODO assume one partition per core
	}
}

void graphRecv(MPI_Comm comm, GraphStruct * local_graph, int sender){
//	int num_procs;
//	MPI_Comm_size(comm, &num_procs);

	MPI_Status status;
	int send_count[2];
	int ack = 0, ack_tag = 5, count_tag = 10, id_tag = 15;

	MPI_Recv(send_count, 2, MPI_INT, sender, count_tag, comm, &status);
	MPI_Send(&ack, 1, MPI_INT, sender, ack_tag, comm);

	if (send_count[0] > 0) {
		graphInit(local_graph, send_count[0], send_count[1]);

		MPI_Recv(local_graph->vertexGIDs, send_count[0] * sizeof(VERTEX_ID_TYPE), MPI_BYTE, sender,
				id_tag, comm, &status);
		MPI_Recv(local_graph->nborIndex, send_count[0] + 1, MPI_INT, sender,
				id_tag + 1, comm, &status);
		MPI_Recv(local_graph->partVector, send_count[0], MPI_INT, sender, id_tag + 2,
				comm, &status);
		MPI_Recv(local_graph->vertexSize, send_count[0], MPI_INT, sender, id_tag + 3,
				comm, &status);
		MPI_Recv(local_graph->vertexWgts, send_count[0], MPI_FLOAT, sender,
				id_tag + 4, comm, &status);
		if (send_count[1] > 0) {
			MPI_Recv(local_graph->nborGIDs, send_count[1] * sizeof(VERTEX_ID_TYPE), MPI_BYTE, sender,
					id_tag + 5, comm, &status);
			MPI_Recv(local_graph->nborParts, send_count[1], MPI_INT, sender,
					id_tag + 6, comm, &status);
			MPI_Recv(local_graph->edgeWgts, send_count[1], MPI_FLOAT, sender,
					id_tag + 7, comm, &status);
		}
//		if (send_count[2] > 0) {
//			MPI_Recv(local_graph->partPlacement, local_graph->numParts * 5, MPI_CHAR, 0, id_tag + 8, comm, &status);
//		}
//		MPI_Recv(local_graph->initialPartNo2NodeID, local_graph->numParts, MPI_INT, 0, id_tag + 8, comm, &status);
//		MPI_Recv(local_graph->initialPartNo2CoreID, local_graph->numParts, MPI_INT, 0, id_tag + 9, comm, &status);
//		MPI_Recv(local_graph->initialCoreID2PartNo, local_graph->numParts, MPI_INT, 0, id_tag + 10, comm, &status); //TODO assume one partition per core
	}

	MPI_Recv(&ack, 1, MPI_INT, sender, 0, comm, &status);
	if (ack < 0) {
		int my_rank;
		MPI_Comm_rank(comm, &my_rank);
		printf("oops! Rank %d receive subgraph from rank %d failed!\n", my_rank, sender);
	}
}

void hashingPartitioning(GraphStruct *globalGraph, int numParts){
	int i;
	for(i=0; i<globalGraph->numVertices; i++){
		int part = globalGraph->vertexGIDs[i] % numParts;
		globalGraph->partVector[i] = part;
	}
}

void rangePartitioning(GraphStruct *globalGraph, int numParts){
	int i;
	int n = globalGraph->numVertices / numParts;
	if(globalGraph->numVertices % numParts)
		n++;
	for(i=0; i<globalGraph->numVertices; i++){
		int part = globalGraph->vertexGIDs[i] / n;
		globalGraph->partVector[i] = part;
	}
}

void LDGPartitioning(GraphStruct *globalGraph, int nparts){
	int i, j;
	VERTEX_ID_TYPE nborLid, nborPart;
	float *nborDist = (float *) malloc(sizeof(float) * nparts);
	float *loadCapacity = (float *) malloc(sizeof(float) * nparts);
	for(i=0; i<nparts; i++){
		loadCapacity[i] = globalGraph->numVertices / nparts + 1;	//TODO vertex-weighted graph
	}
	int *gid2lid = (int *) malloc(sizeof(int) * globalGraph->numVertices);
	for(i=0; i<globalGraph->numVertices; i++){
		gid2lid[globalGraph->vertexGIDs[i]] = i;
		globalGraph->partVector[i] = -1;
	}

	for(i=0; i<globalGraph->numVertices; i++){
		memset(nborDist, 0, sizeof(float) * nparts);
		for(j=globalGraph->nborIndex[i]; j<globalGraph->nborIndex[i+1]; j++){
			nborLid = gid2lid[globalGraph->nborGIDs[j]];
			nborPart = globalGraph->partVector[nborLid];
			if(nborPart != -1){
				nborDist[nborPart] += globalGraph->edgeWgts[j];
			}
		}

		float max = -1;
		int destPart = -1;
		for(j=0; j<nparts; j++){
			if(loadCapacity[j] <= 0) continue;

			if(max < nborDist[j]){
				destPart = j;
				max = nborDist[j];
			}else if(max == nborDist[j]){
				if(loadCapacity[j] < loadCapacity[destPart]){
					destPart = j;
				}
			}
		}
//		printf("%d %d\n", i, destPart);
		loadCapacity[destPart] --;//TODO vertex-weighted graph
		globalGraph->partVector[i] = destPart;
	}
	free(gid2lid);
	free(nborDist);
	free(loadCapacity);
}

void metisPartitioning(GraphStruct *globalGraph, int numParts){
	idx_t edgecut;
	idx_t ncon = 1;
	real_t ubvec = 1.02;
	idx_t nparts = numParts;
	idx_t nVertex = (idx_t) globalGraph->numVertices;
	idx_t *nborIndex = (idx_t *) calloc(sizeof(idx_t), nVertex + 1);
	idx_t *nborGIDs = (idx_t *) malloc(sizeof(idx_t) * globalGraph->numNbors);

	idx_t *vertexWgts = (idx_t *) malloc(sizeof(idx_t) * nVertex);
	idx_t *edgeWgts = (idx_t *) malloc(sizeof(idx_t) * globalGraph->numNbors);

	int i, j;
	for(i=0; i<nVertex; i++){
		VERTEX_ID_TYPE gid = globalGraph->vertexGIDs[i];
		vertexWgts[gid] = globalGraph->vertexWgts[i];
		nborIndex[gid + 1] = globalGraph->nborIndex[i + 1] - globalGraph->nborIndex[i];
	}
	for(i=0; i<nVertex; i++){
		nborIndex[i+1] += nborIndex[i];
	}
	for(i=0; i<nVertex; i++){
		VERTEX_ID_TYPE gid = globalGraph->vertexGIDs[i];
		int start = nborIndex[gid];
		for(j=globalGraph->nborIndex[i]; j<globalGraph->nborIndex[i+1]; j++){
			nborGIDs[start] = globalGraph->nborGIDs[j];
			edgeWgts[start] = globalGraph->edgeWgts[j];
			start ++;
		}
	}
	idx_t *colors = (idx_t *) malloc(sizeof(idx_t) * nVertex);
	METIS_PartGraphKway(
			  &nVertex,		// this many vertices
			  &ncon,		// # of balanced constraints
			  nborIndex,	// adjacency array indices of vertices
			  nborGIDs,		// adjacency array
			  vertexWgts,   // vertex weights
			  NULL,			// vertex size
			  edgeWgts,		// edge weights
			  &nparts,		// # of partitions
			  NULL,     	// partition weights constraint array
			  &ubvec,		// tolerance of degree of imbalance
			  NULL,      	// options
			  &edgecut,     // output: total edges cut with partition
			  colors); 		// output: partition colors

	for(i=0; i<nVertex; i++){
		VERTEX_ID_TYPE gid = globalGraph->vertexGIDs[i];
		globalGraph->partVector[i] = colors[gid];
	}
	free(vertexWgts);
	free(nborIndex);
	free(nborGIDs);
	free(edgeWgts);
	free(colors);
}


void getSubGraph(MPI_Comm comm, GraphStruct *sub_graph, char *fname, int numParts, DecompMethod dt){
	int my_rank;
	MPI_Comm_rank(comm, &my_rank);

	if (my_rank == 0) {
		FILE *fp = fopen(fname, "r");
		if (fp == NULL) {
			printf("File %s not found\n", fname);
		}
		GraphStruct globalGraph;
		graphLoad(&globalGraph, fp);

		if(dt == HP){
			hashingPartitioning(&globalGraph, numParts);	//hashing partitioning
		}else if(dt == RP){
			rangePartitioning(&globalGraph, numParts);		//range partitioning
		}else if(dt == LDG){
			LDGPartitioning(&globalGraph, numParts);
		}else{
			metisPartitioning(&globalGraph, numParts);
		}

		graphDistributeV2(comm, globalGraph, sub_graph);
		graphDeinit(&globalGraph);
		fclose(fp);
	} else {
		graphRecv(comm, sub_graph, 0);
	}
}



//int computeTotalNumNbors(MPI_Comm comm, GraphStruct local_graph) {
//	int global_num_nbors;
//	MPI_Allreduce(&(local_graph.numNbors), &global_num_nbors, 1, MPI_INT,
//			MPI_SUM, comm);
//	return global_num_nbors;
//}
//
//int computeTotalNumVertex(MPI_Comm comm, GraphStruct local_graph) {
//	int global_num_vtxs;
//	//TODO allreduce
//	MPI_Allreduce(&(local_graph.numVertices), &global_num_vtxs, 1, MPI_INT, MPI_SUM, comm);
//	return global_num_vtxs;
//}
//
//int* computeGlobalPartVector(MPI_Comm comm, GraphStruct local_graph,
//		int *local_parts, int global_size) {
//	int i;
//	int * global_parts = (int *) calloc(sizeof(int), global_size);
//	for (i = 0; i < local_graph.numVertices; i++) {
//		global_parts[local_graph.vertexGIDs[i]] = local_parts[i];
//	}
//	//TODO allreduce
//	MPI_Allreduce(MPI_IN_PLACE, global_parts, global_size, MPI_INT, MPI_MAX, comm);
//	return global_parts;
//}
//
//static float* compPartCommMatrix(MPI_Comm comm, GraphStruct localGraph, int *curPartVector){
//	int i, j;
//	int base;
//	int nborGid;
//	int vtxPart, nborPart;
//	int size = localGraph.numParts * localGraph.numParts;
//
//	float *localCommMatrix = calloc(sizeof(float), size);
//	for(i=0; i<localGraph.numVertices; i++){
//		vtxPart = curPartVector[i];
//		base = vtxPart * localGraph.numParts;
//		for(j=localGraph.nborIndex[i]; j<localGraph.nborIndex[i+1]; j++){
//			nborGid = localGraph.nborGIDs[j];
//			nborPart = localGraph.nborParts[j];
//			localCommMatrix[base + nborPart] += localGraph.edgeWgts[j];
//		}
//	}
//	float *commMatrix = NULL;
//	if(m_myRank == 0){
//		commMatrix = calloc(sizeof(float), size);
//	}
//	MPI_Reduce(localCommMatrix, commMatrix, size, MPI_FLOAT, MPI_SUM, 0, comm);
//	free(localCommMatrix);
//	return commMatrix;
//}

//void computeGlobalGraph(MPI_Comm comm, int rootRank, GraphStruct localGraph, GraphStruct *globalGraph){
//	int i, j, k;
//	VERTEX_ID_TYPE vtxGID;
//	int nVertex, nNbors;
//	int nParts = localGraph.numParts;
//	MPI_Reduce(&(localGraph.numVertices), &nVertex, 1, MPI_INT, MPI_SUM, 0, comm);
//	MPI_Reduce(&(localGraph.numNbors), &nNbors, 1, MPI_INT, MPI_SUM, 0, comm);
//
//	int myRank;
//	MPI_Comm_rank(comm, &myRank);
//	int *nborIndex = (int *) malloc(sizeof(int) * (nVertex + 1));
//	for(i=0; i<localGraph.numVertices; i++){
//		vtxGID = localGraph.vertexGIDs[i];
//		nborIndex[vtxGID + 1] = localGraph.nborIndex[i+1] - localGraph.nborIndex[i];
//	}
//	if(myRank == rootRank){
//		graphInit(globalGraph, nVertex, nNbors, nParts);
//	}
//	MPI_Reduce(nborIndex + 1, globalGraph->nborIndex + 1, nVertex, MPI_INT, MPI_MAX, rootRank, comm);
//	free(nborIndex);
//
//	if(myRank == rootRank){
//		for(i=0; i<nVertex; i++){
//			globalGraph->nborIndex[i+1] += globalGraph->nborIndex[i];
//		}
//		GraphStruct subGraph = localGraph;
//		for(i=0; i<nParts; i++){
//			if(i != rootRank){
//				graphRecv(comm, &subGraph, i);
//			}
//			for(j=0; j<subGraph.numVertices; j++){
//				vtxGID = subGraph.vertexGIDs[j];
//				globalGraph->vertexWgts[vtxGID] =  subGraph.vertexWgts[j];
//				globalGraph->vertexSize[vtxGID] = subGraph.vertexSize[j];
//				globalGraph->partVector[vtxGID] = subGraph.partVector[j];
//
//				int start = globalGraph->nborIndex[vtxGID];
//				for(k=localGraph.nborIndex[j]; k<subGraph.nborIndex[j+i]; k++){
//					globalGraph->nborGIDs[start] = subGraph.nborGIDs[k];
//					globalGraph->nborParts[start] = subGraph.nborParts[k];
//					globalGraph->edgeWgts[start] = subGraph.edgeWgts[k];
//					start++;
//				}
//			}
//			if(i != rootRank){
//				graphDeinit(&subGraph);
//			}
//		}
//	}else{
//		int ack = 1;
//		graphSend(comm, localGraph, rootRank);	//each rank send its own subgraph to the root rank
//		MPI_Send(&ack, 1, MPI_INT, rootRank, 0, comm);	//signal root rank to continue
//	}
//	/*
//	for(i=0; i<localGraph.numVertices; i++){
//		vtxGID = localGraph.vertexGIDs[i];
//		globalGraph->vertexWgts[vtxGID] =  localGraph.vertexWgts[i];
//		globalGraph->vertexSize[vtxGID] = localGraph.vertexSize[i];
//		globalGraph->initialPartVector[vtxGID] = localGraph.initialPartVector[i];
//
//		int start = globalGraph->nborIndex[vtxGID];
//		for(j=localGraph.nborIndex[i]; i<localGraph.nborIndex[i+i]; j++){
//			globalGraph->nborGIDs[start] = localGraph.nborGIDs[j];
//			globalGraph->nborProcs[start] = localGraph.nborProcs[j];
//			globalGraph->edgeWgts[start] = localGraph.edgeWgts[j];
//			start++;
//		}
//	}
//
//	MPI_Allreduce(MPI_IN_PLACE, globalGraph->vertexWgts, nVertex, MPI_FLOAT, MPI_MAX, comm);
//	MPI_Allreduce(MPI_IN_PLACE, globalGraph->vertexSize, nVertex, MPI_INT, MPI_MAX, comm);
//	MPI_Allreduce(MPI_IN_PLACE, globalGraph->initialPartVector, nVertex, MPI_INT, MPI_MAX, comm);
//
//	MPI_Allreduce(MPI_IN_PLACE, globalGraph->nborIndex, nVertex + 1, MPI_INT, MPI_MAX, comm);
//	MPI_Allreduce(MPI_IN_PLACE, globalGraph->nborGIDs, nNbors, MPI_INT, MPI_MAX, comm);
//	MPI_Allreduce(MPI_IN_PLACE, globalGraph->nborProcs, nNbors, MPI_INT, MPI_MAX, comm);
//	MPI_Allreduce(MPI_IN_PLACE, globalGraph->edgeWgts, nNbors, MPI_FLOAT, MPI_MAX, comm);
//
//	if(myRank != 0){//TODO only the root rank contains the global graph
//		graphDeinit(globalGraph);
//	}
//	*/
//}

