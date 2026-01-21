#ifndef PLUG_H_
#define PLUG_H_ 1

#include "minicoro.h"

typedef struct Plugin {
        int (*main)(int, char **);
        char name[32];
        void *handle;
        mco_coro *co;
        struct {
                struct Plugin **data;
                int capacity;
                int count;
        } childs;
} Plugin;

void mainloop();
Plugin *child_run(char *plugpath);

#endif // !PLUG_H_
