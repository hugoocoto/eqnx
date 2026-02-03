#define BACKGROUND 0XFF1D2021
#define FOREGROUND 0XFFD4BE98
#define BLACK 0XFF928374
#define RED 0XFFEA6962
#define GREEN 0XFFA9B665
#define YELLOW 0XFFE78A4E
#define BLUE 0XFF7DAEA3
#define MAGENTA 0XFFD3869B
#define CYAN 0XFF89B482
#define WHITE 0XFFDDC7A1

static const uint32_t COLORS[] = {
        [0] = BLACK,
        [1] = RED,
        [2] = GREEN,
        [3] = YELLOW,
        [4] = BLUE,
        [5] = MAGENTA,
        [6] = CYAN,
        [7] = WHITE,
        [8] = BACKGROUND,
        [9] = FOREGROUND,
};

#define COLORS_LEN (sizeof(COLORS) / sizeof(uint32_t))
