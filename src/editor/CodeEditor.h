#pragma once

#include "editor/Syntax.h"

#include <QPlainTextEdit>

class QSyntaxHighlighter;

// The text surface. Word wrap stays off so the vertical scrollbar counts lines
// one-for-one, which is what Minimap relies on to map scroll position to lines.
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    Language language() const { return m_language; }
    void setLanguage(Language lang);

    int indentWidth() const { return m_indentWidth; }
    void setIndentWidth(int spaces);

    // Painted by LineNumberArea, which forwards its events back here.
    int lineNumberAreaWidth() const;
    void paintLineNumberArea(QPaintEvent *event);

    // QPlainTextEdit keeps firstVisibleBlock() protected; Minimap needs the
    // scroll position to place its viewport box.
    int firstVisibleLine() const;
    int visibleLineCount() const;

    // Marks a parse error line in the gutter; -1 clears it.
    void setErrorLine(int line);
    int errorLine() const { return m_errorLine; }

signals:
    void languageChanged(Language lang);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    void handleNewline();
    QString leadingWhitespace(const QString &line) const;

    QWidget *m_lineNumberArea = nullptr;
    QSyntaxHighlighter *m_highlighter = nullptr;
    Language m_language = Language::Json;
    int m_indentWidth = 2;
    int m_errorLine = -1;
};
