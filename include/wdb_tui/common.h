#ifndef WDB_TUI_COMMON_H
#define WDB_TUI_COMMON_H

#include <cdk.h>

// Screen
#define SIDE_MENU_COLS 20
#define SIDE_MENU_LINES LINES
#define DISPLAYS_COLS (COLS - SIDE_MENU_COLS)
#define DISPLAYS_LINES LINES

// Colors
#define WDB_COLOR_NORMAL 0
#define WDB_COLOR_CMD 1
#define WDB_COLOR_SUCCESS 2
#define WDB_COLOR_ERROR 3
#define WDB_COLOR_INFO 4

#endif
