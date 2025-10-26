#pragma once
#include <cstdlib>
#include <utility>

struct ConsoleSize { int cols; int rows; };

inline ConsoleSize get_console_size() {
    ConsoleSize out{80, 30}; // reasonable default

#if defined(_WIN32)
    #include <windows.h>

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    return {columns,rows};
    
#else
    #include <sys/ioctl.h>
    #include <unistd.h>

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    return{w.ws_col, w.ws_row};

#endif
}

