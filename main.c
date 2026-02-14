#include <stdio.h>

#define PRINT(x, ...) printf("%-*.*s --> " x "\n", 10, 10, #x, ##__VA_ARGS__)

int
main()
{
        int w = 10;
        PRINT("|%s|", "A");
        PRINT("|%.*s|", w, "A");
        PRINT("|%*s|", w, "A");
        PRINT("|%-*s|", w, "A");
        PRINT("|%*.*s|", w, w, "A");
        PRINT("|%-*.*s|", w, w, "A");
}
