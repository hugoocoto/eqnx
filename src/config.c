#include "config.h"

#define INCLUDE_CONF_IMPLEMENTATION
#include "conf.h"

/* defaults */
Config config = (Config) {
        .fontpath = NULL,
        .fontname = NULL,
        .fontsize = 36,
};

void
load_config(const char *path)
{
        Conf conf;
        int s = Conf_open(&conf, (char *) path);
        if (s != CONF_OK) {
                printf("Can't open config.lua\n");
                return;
        }
        if (Conf_get_int(conf, "Font.size", &config.fontsize) != CONF_OK) {
                printf("Can't read Font.size from config.lua\n");
        } else {
                printf("Font.size = %d\n", config.fontsize);
        }
        if (Conf_get_str(conf, "Font.path", &config.fontpath) != CONF_OK) {
                printf("Can't read Font.path from config.lua\n");
        } else {
                printf("Font.path = %s\n", config.fontpath);
                config.fontpath = strdup(config.fontpath);
        }
        if (Conf_get_str(conf, "Font.name", &config.fontname) != CONF_OK) {
                printf("Can't read Font.name from config.lua\n");
        } else {
                config.fontname = strdup(config.fontname);
                printf("Font.name = %s\n", config.fontname);
        }
        Conf_close(conf);
}
