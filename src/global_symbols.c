#include <stdlib.h>

#include "hm.h"

#define SYMBOL_TABLE_SIZE 128
static Hmap sym_table;

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
symbol_add(char *symbol, void *value)
{
        void *v;
        hmget(sym_table, symbol, &v);
        if (v) return -1;
        hmadd(&sym_table, symbol, value);
        return 0;
}

void *
symbol_get(char *symbol)
{
        void *v = NULL;
        hmget(sym_table, symbol, &v);
        return v;
}
