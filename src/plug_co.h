#ifndef PLUG_CO_H
#define PLUG_CO_H 1

#include "plug_api.h"

void coro_entry(mco_coro *co);
int plugin_default_main(int argc, char **argv);
void plug_destroy(Plugin *p);
void plug_release(Plugin *p);
Plugin *plug_open(const char *plugdir);
void plug_add_child(Plugin *parent, Plugin *child);
int plug_exec(Plugin *p);

#endif
