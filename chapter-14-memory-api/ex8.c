#include <stdlib.h>
#include <stdio.h>

typedef struct {
  size_t capacity;
  size_t size;
  int *data;
} IntArrayList;

IntArrayList *init() {
  IntArrayList *list = malloc(sizeof(IntArrayList));
  list->capacity = 0;
  list->size = 0;
  list->data = NULL;
  return list;
}

void add(IntArrayList *arraylist, int element) {
  if (arraylist->capacity <= arraylist->size) {
    arraylist->data =
        realloc(arraylist->data, (arraylist->size + 1) * sizeof(int));
    arraylist->capacity++;
  }
  arraylist->data[arraylist->size] = element;
  arraylist->size++;
}

void freeMem(IntArrayList *arrayList) { 
    free(arrayList->data);
    free(arrayList);
}

int main() {
    IntArrayList *array = init(); 
    add(array, 3);
    add(array, 5);
    add(array, 3);

    for (int i=0; i<3;i++) {
        printf("Value is %d:\n", array->data[i]);
    }
    freeMem(array);
}
