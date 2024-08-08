#define _GNU_SOURCE
#include <stdlib.h>

/*
       void qsort_r(void *base, size_t nmemb, size_t size,
                  int (*compar)(const void *, const void *, void *),
                  void *arg);
 */

struct item {
    char *name;
    int id;
    double rating;
} items[] = {
    { "A", 157, 0.9 },
    { "B", 517, 0.9 },
    { "C", 175, 0.9 },
    { "D", 571, 0.9 },
    { "E", 127, 0.9 },
    { "F", 147, 0.9 },
    { "G", 117, 0.9 },
    { "H", 107, 0.9 },
    { "I", 111, 0.9 },
    { "J", 227, 0.9 },
    { "K", 157, 0.9 },
    { "L", 157, 0.9 },
    { "M", 157, 0.9 },
    { "N", 157, 0.9 },
    { "O", 157, 0.9 },
    { "P", 157, 0.9 },
    { "Z", 157, 0.9 }
};

int compare_items(const void *a, const void *b, void *arg) {
    const struct item *itemL = a, *itemR = b;
    if (itemL->id < itemR->id) return -1;
    if (itemL->id > itemR->id) return 1;
    return 0;
}

int main() {
    qsort_r(items, sizeof(items)/sizeof(items[0]), sizeof(items[0]), compare_items, 0);
    return 0;
}
