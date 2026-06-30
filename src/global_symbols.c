#include <stdlib.h>

#include "hm.h"

#define SYMBOL_TABLE_SIZE 128
static Hmap sym_table;

struct Entry {
        int size;
        char *mem;
};

__attribute__((constructor)) void
init()
{
        hmnew(&sym_table, SYMBOL_TABLE_SIZE);
}

__attribute__((destructor)) void
fini()
{
        hmdestroy(&sym_table);
}

int
symbol_register(char *symbol, void **store, int size)
{
        struct Entry *v;
        if (size <= 0) return -1;

        hmget(sym_table, symbol, (void **) &v);
        if (v) {
                if (size != v->size) return -1;
                *store = v->mem;
        } else {
                v = malloc(sizeof(struct Entry));
                v->mem = malloc(size);
                v->size = size;
                hmadd(&sym_table, symbol, v);
                *store = v->mem;
        }
        return 0;
}
