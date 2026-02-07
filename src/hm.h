#ifndef HM_H_
#define HM_H_

typedef struct Hnode {
        char *key;
        void *value;
        struct Hnode *next;
} Hnode;

typedef struct
{
        int size;
        Hnode *node_arr;
} Hmap;

void hmnew(Hmap *table, int size);
void hmadd(Hmap *table, const char *key, void *value);
void hmpop(Hmap *table, const char *key);
void *hmget(Hmap table, const char *key, void **value);
void hmdestroy(Hmap *table);

#endif
