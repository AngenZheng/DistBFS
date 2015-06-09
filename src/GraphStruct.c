/*
 * TopoLBGraph.c
 *
 *  Created on: Dec 17, 2013
 *      Author: Angen Zheng
 */

#include "GraphStruct.h"

void graphInit(GraphStruct *graph, int num_vtxs, int num_nbors){
//	int num_nbors = num_edges << 1;
	graph->numVertices = num_vtxs;
	graph->numNbors = num_nbors;

	if(num_vtxs > 0){
		graph->active = (int *) calloc(sizeof(int), num_vtxs);
//		graph->vertexLIDs = (VERTEX_ID_TYPE *) malloc(sizeof(VERTEX_ID_TYPE) * num_vtxs);
		graph->vertexGIDs = (VERTEX_ID_TYPE *) malloc(sizeof(VERTEX_ID_TYPE) * num_vtxs);
		graph->vertexSize = (int *) malloc(sizeof(int) * (num_vtxs));
		graph->partVector = (int *) malloc(sizeof(int) * (num_vtxs));
		graph->vertexWgts = (float *) malloc(sizeof(float) * (num_vtxs));
		graph->nborIndex = (int *) calloc(sizeof(int), (num_vtxs + 1));

		if(graph->numNbors > 0){
			graph->nborGIDs = (VERTEX_ID_TYPE *) malloc(sizeof(VERTEX_ID_TYPE) * num_nbors);
			graph->nborParts = (int *) malloc(sizeof(int) * num_nbors);
			graph->edgeWgts = (float *) malloc(sizeof(float) * num_nbors);
		}
//		graph->initialPartNo2CoreID = (int *) calloc(sizeof(int), graph->numParts);

//		int i;
//		graph->vtxTimeVector = (BitVector_t *) malloc(sizeof(BitVector_t) * num_vtxs);
//		for(i=0; i<num_vtxs; i++){
//			INIT_BITVECTOR(graph->vtxTimeVector + i, TIME_VECTOR_LENGTH);
//		}
	}
}
void graphInitVertexLists(GraphStruct *graph, int num_vtxs){
	graph->numVertices = num_vtxs;
	if(num_vtxs > 0){
//		graph->vertexLIDs = (VERTEX_ID_PTR) calloc(num_vtxs, sizeof(VERTEX_ID_TYPE));
		graph->vertexGIDs = (VERTEX_ID_TYPE *) calloc(num_vtxs, sizeof(VERTEX_ID_TYPE));
		graph->vertexSize = (int *) calloc(num_vtxs, sizeof(int));
		graph->partVector = (int *) calloc(num_vtxs, sizeof(int));
		graph->vertexWgts = (float *) calloc(num_vtxs, sizeof(float));
		graph->nborIndex = (int *) calloc((num_vtxs + 1), sizeof(int));
	}
}

void graphInitEdgeLists(GraphStruct *graph, int num_nbors){
	graph->numNbors = num_nbors;
	if(num_nbors > 0){
//		graph->vertexLIDs = (VERTEX_ID_PTR) calloc(num_vtxs, sizeof(VERTEX_ID_TYPE));
		graph->nborGIDs = (VERTEX_ID_TYPE *) calloc(num_nbors, sizeof(VERTEX_ID_TYPE));
		graph->nborParts = (int *) calloc(num_nbors, sizeof(int));
		graph->edgeWgts = (float *) calloc(num_nbors, sizeof(float));
	}
}

void graphDeinit(GraphStruct * graph) {
    if(graph->numVertices > 0){
    	free(graph->active);
//    	free(graph->vertexLIDs);
    	free(graph->vertexGIDs);
    	free(graph->vertexSize);
    	free(graph->partVector);
    	free(graph->vertexWgts);
    	free(graph->nborIndex);

    	if(graph->numNbors > 0){
			free(graph->nborGIDs);
			free(graph->nborParts);
			free(graph->edgeWgts);
    	}

//        free(graph->initialPartNo2CoreID);

//        int i;
//        for(i=0; i<graph->numVertices; i++){
//        	FREE_BITVECTOR(graph->vtxTimeVector + i);
//        }
//        free(graph->vtxTimeVector);
    }
    graph->numVertices = 0;
    graph->numNbors = 0;
}

void graphPrint(GraphStruct graph){
	printf("num_vtx=%d, num_nbors=%d\n", graph.numVertices, graph.numNbors);

	int i, j, num_nbors;
	for(i=0; i<graph.numVertices; i++){
		num_nbors = graph.nborIndex[i + 1] - graph.nborIndex[i];

		printf("%d %d %.1f %d %d ", graph.vertexGIDs[i], graph.partVector[i], graph.vertexWgts[i], graph.vertexSize[i], num_nbors);
		for(j=graph.nborIndex[i]; j<graph.nborIndex[i + 1]; j++){
			printf("%d %.1f ", graph.nborGIDs[j], graph.edgeWgts[j]);
		}
		printf("\n");
	}
}



int getNextLine(FILE *fp, char *buf, int bufsize);

static int bufsize=1<<20;	//1MB line buffer
static char line[1<<20];

int graphLoad(GraphStruct * graph, FILE * gdata) {
	int numGlobalVertices, numGlobalEdges, numParts;
	int i, j, nnbors;

	/* Get the number of vertices */
	getNextLine(gdata, line, bufsize);
	sscanf(line, "%d", &numGlobalVertices);

	/* Get the number of edges  */
	getNextLine(gdata, line, bufsize);
	sscanf(line, "%d", &numGlobalEdges);

	/* Get the number of partitions  */
	getNextLine(gdata, line, bufsize);
	sscanf(line, "%d", &numParts);

	/* Allocate arrays to read in entire graph */
	graphInit(graph, numGlobalVertices, numGlobalEdges << 1);

	char * token;
	//TODO partition placement???
//	//read current partition mapping to physical cores in pairs of <partNO, coreID>
	for (i = 0; i < numParts; i++) {
		getNextLine(gdata, line, bufsize);
//		token = strtok(line, " ");
//		part = atoi(token);
//		token = strtok(NULL, " ");
//		memcpy(graph->partPlacement + part * 5, token, 5);
	}
//	graph->partPlacement[5 * numParts] = '\0';

	//each line is in the form of vtx_id  part_id vtx_wgt vtx_size  num_nbors  nbor_id  edge_wgt  nbor_id  edge_wgt
	for (i = 0; i < numGlobalVertices; i++) {
		getNextLine(gdata, line, bufsize);

		token = strtok(line, " ");
		graph->vertexGIDs[i] = atoi(token);
		token = strtok(NULL, " ");
		graph->partVector[i] = atoi(token);

		token = strtok(NULL, " ");
		graph->vertexWgts[i] = (float) atof(token);
		token = strtok(NULL, " ");
		graph->vertexSize[i] = atoi(token);
		token = strtok(NULL, " ");
		nnbors = atoi(token);

		graph->nborIndex[i + 1] = graph->nborIndex[i] + nnbors;
		for (j = graph->nborIndex[i]; j<graph->nborIndex[i + 1]; j++) {
			token = strtok(NULL, " ");
			graph->nborGIDs[j] = atoi(token);
			token = strtok(NULL, " ");
			graph->edgeWgts[j] = (float) atof(token);
		}
	}
	return 0;
}

/* Function to find next line of information in input file */
int getNextLine(FILE *fp, char *buf, int bufsize) {
	int i, cval, len;
	char *c;

	while (1) {
		c = fgets(buf, bufsize, fp);
		if (c == NULL)
			return 0; /* end of file */
		len = strlen(c);
		for (i = 0, c = buf; i < len; i++, c++) {
			cval = (int) *c;
			if (isspace(cval) == 0)
				break;
		}
		if (i == len)
			continue; /* blank line */
		if (*c == '#')
			continue; /* comment */
		if (c != buf) {
			strcpy(buf, c);
		}
		break;
	}
	return strlen(buf); /* number of characters */
}
