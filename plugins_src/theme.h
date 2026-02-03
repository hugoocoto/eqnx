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
#define TRANSPARENT 0X00000000

static const uint32_t COLORS[] = {
        [0] = TRANSPARENT,
        [1] = BLACK,
        [2] = RED,
        [3] = GREEN,
        [4] = YELLOW,
        [5] = BLUE,
        [6] = MAGENTA,
        [7] = CYAN,
        [8] = WHITE,
        [9] = BACKGROUND,
        [10] = FOREGROUND,
};

#define COLORS_LEN (sizeof(COLORS) / sizeof(uint32_t))

// As any number < 0x00xxxxxx is transparent, we should have a transparent color
// in the table for convenience. Non transparent colors won't collide with table
// indexes for the same reason.
