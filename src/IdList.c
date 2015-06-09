#include <stdlib.h>
#include <string.h>
#include "IdList.h"

typedef VERTEX_ID_TYPE DATA_ITEM;

IdList_t* idListCreate() {
	IdList_t *list = (IdList_t*) calloc(1, sizeof(IdList_t));
	if (!list)
		return NULL;
	list->size = DEFAULT_SIZE;
	list->length = 0;
	if (!(list->data = (DATA_ITEM *) calloc(sizeof(DATA_ITEM), list->size))) {
		free(list);
		return NULL;
	}
	return list;
}

void idListDestroy(IdList_t *list) {
	free(list->data);
	free(list);
}

DATA_ITEM idListGetIdx(IdList_t *list, int i) {
	return list->data[i];
}

void idListSetIdx(IdList_t *list, int i, DATA_ITEM val) {
	list->data[i] = val;
}

void idListIncIdx(IdList_t *list, int i) {
	list->data[i] ++;
}

void idListRemoveIdx(IdList_t *list, int i){
	for(; i<list->length - 1; i++){
		list->data[i] = list->data[i + 1];
	}
	list->length-- ;
}

void idListAppend(IdList_t *list, DATA_ITEM data) {
	if(list->length == list->size){//expend
		list->size = list->size * 1.2;
		list->data = realloc(list->data, list->size * sizeof(DATA_ITEM));
	}
	list->data[list->length ++] = data;
}

void idListAppendAll(IdList_t *list, DATA_ITEM *data, int numElems) {
	if(list->length + numElems >= list->size){//expend
		list->size = list->size * 1.2 + numElems;
		list->data = realloc(list->data, list->size * sizeof(DATA_ITEM));
	}
	memcpy(list->data + list->length, data, sizeof(DATA_ITEM) * numElems);
	list->length += numElems;
}

int idListLength(IdList_t *list) {
	return list->length;
}

void idListClear(IdList_t *list){
	list->length = 0;
}

//static int _listExpand(idList_t *list, int max) {
//	if (max < list->size)
//		return 0;
//	int new_size = list->size  * 1.5;
//	if (new_size < max)
//		new_size = max;
//
//	DATA_ITEM *t;
//	if (!(t = realloc(list->data, new_size * sizeof(DATA_ITEM))))
//		return -1;
//	list->data = t;
//	(void) memset(list->data + list->size, 0, (new_size - list->size) * sizeof(DATA_ITEM));
//	list->size = new_size;
//	return 0;
//}
//
//int listPutIdx(idList_t *list, int idx, DATA_ITEM data) {
//	if (_listExpand(list, idx + 1))
//		return -1;
//	list->data[idx] = data;
//	if (list->length <= idx)
//		list->length = idx + 1;
//	return 0;
//}
