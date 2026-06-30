#ifndef CONFIG_H_
#define CONFIG_H_

typedef struct Config {
        const char *fontpath;
        const char *fontname;
        int fontsize;
} Config;

extern Config config;
void load_config(const char* path);

#endif // !CONFIG_H_
