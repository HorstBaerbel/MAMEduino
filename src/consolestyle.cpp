#include "consolestyle.h"


const ConsoleStyle::ColorInfo ConsoleStyle::foregroundColors[9] = {
#ifdef WIN32
    { DEFAULT, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE },
    { BLACK, 0 },
    { RED, FOREGROUND_RED | FOREGROUND_INTENSITY },
    { GREEN, FOREGROUND_GREEN | FOREGROUND_INTENSITY },
    { YELLOW, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY },
    { BLUE, FOREGROUND_BLUE | FOREGROUND_INTENSITY },
    { MAGENTA, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY },
    { CYAN, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY },
    { WHITE, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE }
#else
    { DEFAULT, "\033[0m" },
    { BLACK, "\033[0;30m" },
    { RED, "\033[0;31m" },
    { GREEN, "\033[0;32m" },
    { YELLOW, "\033[0;33m" },
    { BLUE, "\033[0;34m" },
    { MAGENTA, "\033[0;35m" },
    { CYAN, "\033[0;36m" },
    { WHITE, "\033[0;37m" }
#endif
};

const ConsoleStyle::ColorInfo ConsoleStyle::backgroundColors[9] = {
#ifdef WIN32
    { DEFAULT, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE },
    { BLACK, 0 },
    { RED, BACKGROUND_RED | BACKGROUND_INTENSITY },
    { GREEN, BACKGROUND_GREEN | BACKGROUND_INTENSITY },
    { YELLOW, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY },
    { BLUE, BACKGROUND_BLUE | BACKGROUND_INTENSITY },
    { MAGENTA, BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY },
    { CYAN, BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY },
    { WHITE, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE }
#else
    { DEFAULT, "\033[0m" },
    { BLACK, "\033[0;40m" },
    { RED, "\033[0;41m" },
    { GREEN, "\033[0;42m" },
    { YELLOW, "\033[0;43m" },
    { BLUE, "\033[0;44m" },
    { MAGENTA, "\033[0;45m" },
    { CYAN, "\033[0;46m" },
    { WHITE, "\033[0;47m" }
#endif
};

std::ostream & operator<<(std::ostream & os, const ConsoleStyle & style)
{
#ifdef WIN32
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdOut, (WORD)style.foregroundColor.colorFlags);
#else
    os << style.foregroundColor.colorString;
#endif
    return os;
}

