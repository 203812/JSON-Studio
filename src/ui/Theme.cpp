#include "ui/Theme.h"

#include <QFontDatabase>

namespace theme {

const Palette &palette()
{
    static const Palette p;
    return p;
}

QFont editorFont()
{
    const QStringList candidates{"Consolas", "Cascadia Mono", "DejaVu Sans Mono", "Courier New"};
    const QStringList families = QFontDatabase::families();
    for (const QString &name : candidates) {
        if (families.contains(name, Qt::CaseInsensitive)) {
            QFont f(name);
            f.setPointSizeF(11.0);
            f.setStyleHint(QFont::Monospace);
            f.setFixedPitch(true);
            return f;
        }
    }
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSizeF(11.0);
    return f;
}

QString styleSheet()
{
    const Palette &p = palette();
    const auto c = [](const QColor &col) { return col.name(QColor::HexRgb); };

    return QString(R"(
QMainWindow, QWidget#Root {
    background: %{chromeBg};
}

/* ---- Menu bar ---------------------------------------------------------- */
QMenuBar {
    background: %{chromeBg};
    color: %{chromeFg};
    border: none;
    padding: 2px 4px;
}
QMenuBar::item {
    background: transparent;
    padding: 4px 10px;
    margin: 0px;
}
QMenuBar::item:selected  { background: %{tabHover}; color: #F8F8F2; }
QMenuBar::item:pressed   { background: %{tabInactive}; }

QMenu {
    background: #34352E;
    color: #F8F8F2;
    border: 1px solid %{border};
    padding: 4px 0px;
}
QMenu::item { padding: 5px 28px 5px 24px; }
QMenu::item:selected { background: #4E5044; }
QMenu::item:disabled { color: %{chromeFgDim}; }
QMenu::separator { height: 1px; background: %{border}; margin: 4px 8px; }

/* ---- Splitter ---------------------------------------------------------- */
QSplitter::handle { background: %{border}; }
QSplitter::handle:horizontal { width: 1px; }

/* ---- Sidebar ----------------------------------------------------------- */
QWidget#Sidebar { background: %{chromeBg}; }
QLabel#SidebarHeader {
    color: %{chromeFgDim};
    padding: 8px 10px 6px 10px;
    font-size: 11px;
    letter-spacing: 1px;
}
QLabel#SidebarEmpty {
    color: %{chromeFgDim};
    padding: 10px;
}
QTreeView#FileTree {
    background: %{chromeBg};
    color: %{chromeFg};
    border: none;
    outline: none;
    show-decoration-selected: 1;
    padding-left: 2px;
}
QTreeView#FileTree::item { padding: 3px 2px; border: none; }
QTreeView#FileTree::item:hover { background: %{tabInactive}; }
QTreeView#FileTree::item:selected { background: #4E5044; color: #F8F8F2; }
QTreeView#FileTree::branch:hover { background: %{tabInactive}; }

/* ---- Tabs -------------------------------------------------------------- */
QTabWidget::pane { border: none; background: %{editorBg}; }
QTabBar { background: %{chromeBg}; qproperty-drawBase: 0; }
QTabBar::tab {
    background: %{tabInactive};
    color: %{chromeFgDim};
    padding: 7px 12px;
    border: none;
    border-right: 1px solid %{border};
    min-width: 60px;
}
QTabBar::tab:hover { background: %{tabHover}; color: %{chromeFg}; }
QTabBar::tab:selected { background: %{tabActive}; color: #F8F8F2; }
QTabBar::close-button {
    image: none;
    subcontrol-position: right;
    border-radius: 5px;
    margin: 2px;
}
QTabBar::close-button:hover { background: #75715E; }

/* ---- Status bar -------------------------------------------------------- */
QStatusBar {
    background: %{chromeBg};
    color: %{chromeFgDim};
    border-top: 1px solid %{border};
}
QStatusBar::item { border: none; }
QStatusBar QLabel { color: %{chromeFgDim}; padding: 2px 10px; }
QLabel#StatusMessage { color: %{chromeFg}; }
QLabel#StatusError { color: %{synError}; }
QLabel#StatusOk { color: %{accent}; }

/* ---- Scrollbars: Sublime keeps these thin and unobtrusive -------------- */
QScrollBar:vertical {
    background: %{editorBg};
    width: 12px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #4E4F44;
    min-height: 24px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover { background: #6B6C5E; }
QScrollBar:horizontal {
    background: %{editorBg};
    height: 12px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: #4E4F44;
    min-width: 24px;
    border-radius: 6px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover { background: #6B6C5E; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

/* ---- Command palette --------------------------------------------------- */
QWidget#PaletteFrame {
    background: #34352E;
    border: 1px solid %{border};
}
QLineEdit#PaletteInput {
    background: #1E1F1C;
    color: #F8F8F2;
    border: 1px solid %{border};
    padding: 7px 9px;
    selection-background-color: %{selection};
    selection-color: #F8F8F2;
}
QListView#PaletteList {
    background: #34352E;
    color: #F8F8F2;
    border: none;
    outline: none;
}
QListView#PaletteList::item { padding: 6px 9px; }
QListView#PaletteList::item:selected { background: #4E5044; }

/* ---- Editor ------------------------------------------------------------ */
QPlainTextEdit#CodeEditor {
    background: %{editorBg};
    color: %{editorFg};
    border: none;
    selection-background-color: %{selection};
    selection-color: #F8F8F2;
}
)")
        .replace("%{chromeBg}", c(p.chromeBg))
        .replace("%{chromeFgDim}", c(p.chromeFgDim))
        .replace("%{chromeFg}", c(p.chromeFg))
        .replace("%{tabInactive}", c(p.tabInactive))
        .replace("%{tabActive}", c(p.tabActive))
        .replace("%{tabHover}", c(p.tabHover))
        .replace("%{editorBg}", c(p.editorBg))
        .replace("%{editorFg}", c(p.editorFg))
        .replace("%{selection}", c(p.selection))
        .replace("%{synError}", c(p.synError))
        .replace("%{accent}", c(p.accent))
        .replace("%{border}", c(p.border));
}

} // namespace theme
