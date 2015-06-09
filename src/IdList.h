#ifndef _ID_LIST_h_
#define _ID_LIST_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "GraphStruct.h"

#define DEFAULT_SIZE 4096

typedef struct {
	VERTEX_ID_TYPE* data;
	int length;
	int size;
} IdList_t;

IdList_t* idListCreate();
void idListDestroy(IdList_t *al);

VERTEX_ID_TYPE idListGetIdx(IdList_t *al, int i);
void idListIncIdx(IdList_t *list, int i);
void idListSetIdx(IdList_t *list, int i, VERTEX_ID_TYPE val);
void idListRemoveIdx(IdList_t *al, int i);
void idListAppend(IdList_t *al, VERTEX_ID_TYPE data);
void idListAppendAll(IdList_t *list, VERTEX_ID_TYPE *data, int numElems);

int idListLength(IdList_t *al);
void idListClear(IdList_t *al);

//int listPutIdx(idList_t *al, int i, DATA_ITEM data);
//void listSort(idList_t *arr, int (*compar)(const DATA_ITEM, const DATA_ITEM));

#ifdef __cplusplus
}
#endif

#endif
