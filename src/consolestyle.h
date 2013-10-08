#include <iostream>

#ifdef WIN32
    #include <windows.h>
#endif


//See here: http://www.cplusplus.com/forum/unices/36461/
//and here: http://ascii-table.com/ansi-escape-sequences.php
class ConsoleStyle
{
public:
    enum Color { DEFAULT, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE};

private:
    struct ColorInfo
    {
        Color color;
#ifdef WIN32
        WORD colorFlags;
#else
        std::string colorString;
#endif
    };
    static const ColorInfo foregroundColors[9];
    static const ColorInfo backgroundColors[9];

    ColorInfo foregroundColor;

public:
    /*!
    Create console output style.
    \param[in] color Set a text foreground color.
    */
    ConsoleStyle(Color color = DEFAULT) : foregroundColor(foregroundColors[color]) {};

    /*!
    Apply style to output stream.
    */
    friend std::ostream & operator<<(std::ostream & os, const ConsoleStyle & style);
};

