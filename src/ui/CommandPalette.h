#pragma once

#include <QFrame>
#include <QList>
#include <QString>

#include <functional>

class QLineEdit;
class QListWidget;

// Ctrl+Shift+P. Also the extension point for every tool we add later (diff,
// YAML, codegen, schema): register a Command and it is reachable, no menu
// surgery required.
class CommandPalette : public QFrame
{
    Q_OBJECT

public:
    struct Command {
        QString title;    // "JSON: Format Document"
        QString shortcut; // display only, e.g. "Ctrl+Shift+F"
        std::function<void()> run;
    };

    explicit CommandPalette(QWidget *parent = nullptr);

    void setCommands(QList<Command> commands);
    void showPalette();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void refilter(const QString &query);
    void accept();
    void reposition();

    // Subsequence match, the way Sublime's palette behaves: "jfd" hits
    // "JSON: Format Document". Returns -1 for no match, lower score is better.
    static int fuzzyScore(const QString &haystack, const QString &needle);

    QLineEdit *m_input = nullptr;
    QListWidget *m_list = nullptr;
    QList<Command> m_commands;
    QList<int> m_filtered; // indices into m_commands
};
