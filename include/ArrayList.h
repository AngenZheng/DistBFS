#ifndef _ArrayList_h_
#define _ArrayList_h_

#ifdef __cplusplus
"C" {
#endif

#define DEFAULT_SIZE 64
typedef int DATA_ITEM;


typedef struct ArrayList {
	DATA_ITEM* data;
	int length;
	int size;
} ArrayList_t;

ArrayList_t* listCreate();
void listDestroy(ArrayList_t *al);

DATA_ITEM listGetIdx(ArrayList_t *al, int i);
int listPutIdx(ArrayList_t *al, int i, DATA_ITEM data);
int listAppend(ArrayList_t *al, DATA_ITEM data);
int listLength(ArrayList_t *al);
void listClear(ArrayList_t *al);

#ifdef __cplusplus
}
#endif

#endif
