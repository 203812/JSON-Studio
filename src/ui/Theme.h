#pragma once

#include <QColor>
#include <QFont>
#include <QString>

// Monokai, as Sublime Text renders it. Every colour in the app comes from here
// so a future theme switcher only has to swap this struct.
namespace theme {

struct Palette {
    // Editor surface
    QColor editorBg{"#272822"};
    QColor editorFg{"#F8F8F2"};
    QColor currentLine{"#3E3D32"};
    QColor selection{"#49483E"};
    QColor caret{"#F8F8F0"};
    QColor indentGuide{"#3B3A32"};

    // Gutter
    QColor gutterFg{"#90908A"};
    QColor gutterFgActive{"#C2C2BF"};

    // Syntax
    QColor synKey{"#F92672"};
    QColor synString{"#E6DB74"};
    QColor synNumber{"#AE81FF"};
    QColor synConstant{"#AE81FF"};
    QColor synPunctuation{"#F8F8F2"};
    QColor synError{"#F92672"};

    // Chrome: sidebar, tab bar, status bar
    QColor chromeBg{"#1E1F1C"};
    QColor chromeFg{"#A8A79A"};
    QColor chromeFgDim{"#75715E"};
    QColor tabActive{"#272822"};
    QColor tabInactive{"#2D2E27"};
    QColor tabHover{"#34352E"};
    QColor border{"#141510"};
    QColor accent{"#A6E22E"};
    QColor warn{"#FD971F"};
};

const Palette &palette();

// Application-wide QSS, generated from palette() so colours stay in one place.
QString styleSheet();

// Consolas 11 on Windows, with sane fallbacks.
QFont editorFont();

} // namespace theme
