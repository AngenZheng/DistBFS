#include <stdlib.h>
#include <string.h>
#include "ArrayList.h"

ArrayList_t* listCreate() {
	ArrayList_t *list = (ArrayList_t*) calloc(1, sizeof(ArrayList_t));
	if (!list)
		return NULL;
	list->size = DEFAULT_SIZE;
	list->length = 0;
	if (!(list->data = (DATA_ITEM*) calloc(sizeof(DATA_ITEM), list->size))) {
		free(list);
		return NULL;
	}
	return list;
}

void listDestroy(ArrayList_t *list) {
	free(list->data);
	free(list);
}

DATA_ITEM listGetIdx(ArrayList_t *list, int i) {
	return list->data[i];
}

static int _listExpand(ArrayList_t *list, int max) {
	if (max < list->size)
		return 0;
	int new_size = list->size  * 1.5;
	if (new_size < max)
		new_size = max;

	DATA_ITEM *t;
	if (!(t = realloc(list->data, new_size * sizeof(DATA_ITEM))))
		return -1;
	list->data = t;
	(void) memset(list->data + list->size, 0, (new_size - list->size) * sizeof(DATA_ITEM));
	list->size = new_size;
	return 0;
}

int listPutIdx(ArrayList_t *list, int idx, DATA_ITEM data) {
	if (_listExpand(list, idx + 1))
		return -1;
	list->data[idx] = data;
	if (list->length <= idx)
		list->length = idx + 1;
	return 0;
}

int listAppend(ArrayList_t *arr, DATA_ITEM data) {
	return listPutIdx(arr, arr->length, data);
}

int listLength(ArrayList_t *arr) {
	return arr->length;
}

void listClear(ArrayList_t *list){
	list->length = 0;
}
