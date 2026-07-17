#include "editor/CodeEditor.h"

#include "editor/JsonHighlighter.h"
#include "editor/YamlHighlighter.h"
#include "ui/Theme.h"

#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>

namespace {

// Thin proxy widget: it exists to occupy the gutter rect and hand painting back
// to CodeEditor, which owns the state it needs.
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

    QSize sizeHint() const override { return {m_editor->lineNumberAreaWidth(), 0}; }

protected:
    void paintEvent(QPaintEvent *event) override { m_editor->paintLineNumberArea(event); }

private:
    CodeEditor *m_editor;
};

} // namespace

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setObjectName("CodeEditor");
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFrameShape(QFrame::NoFrame);
    setCursorWidth(2);

    const QFont f = theme::editorFont();
    setFont(f);
    document()->setDefaultFont(f);

    const QFontMetricsF fm(f);
    setTabStopDistance(m_indentWidth * fm.horizontalAdvance(u' '));

    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter = new JsonHighlighter(document());

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

void CodeEditor::setLanguage(Language lang)
{
    if (m_language == lang && m_highlighter)
        return;

    m_language = lang;
    delete m_highlighter;
    switch (lang) {
    case Language::Yaml:
        m_highlighter = new YamlHighlighter(document());
        break;
    case Language::Json:
        m_highlighter = new JsonHighlighter(document());
        break;
    }
    emit languageChanged(lang);
}

void CodeEditor::setIndentWidth(int spaces)
{
    m_indentWidth = qMax(1, spaces);
    const QFontMetricsF fm(font());
    setTabStopDistance(m_indentWidth * fm.horizontalAdvance(u' '));
}

void CodeEditor::setErrorLine(int line)
{
    if (m_errorLine == line)
        return;
    m_errorLine = line;
    m_lineNumberArea->update();
}

int CodeEditor::firstVisibleLine() const
{
    return firstVisibleBlock().blockNumber();
}

int CodeEditor::visibleLineCount() const
{
    const int lineH = qMax(1, fontMetrics().height());
    return qMax(1, viewport()->height() / lineH);
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    for (int max = qMax(1, blockCount()); max >= 10; max /= 10)
        ++digits;
    // Sublime keeps generous air either side of the numbers.
    return 14 + fontMetrics().horizontalAdvance(u'9') * qMax(digits, 2) + 10;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection line;
        line.format.setBackground(theme::palette().currentLine);
        line.format.setProperty(QTextFormat::FullWidthSelection, true);
        line.cursor = textCursor();
        line.cursor.clearSelection();
        selections.append(line);
    }
    setExtraSelections(selections);
}

void CodeEditor::paintLineNumberArea(QPaintEvent *event)
{
    const theme::Palette &p = theme::palette();
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), p.editorBg);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    const int currentLine = textCursor().blockNumber();

    painter.setFont(font());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const int lineNo = blockNumber + 1;

            if (lineNo == m_errorLine) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(p.synError);
                const int h = qRound(blockBoundingRect(block).height());
                painter.drawRect(0, top, 3, h);
                painter.setBrush(Qt::NoBrush);
            }

            painter.setPen(blockNumber == currentLine ? p.gutterFgActive : p.gutterFg);
            painter.drawText(0,
                             top,
                             m_lineNumberArea->width() - 10,
                             qRound(blockBoundingRect(block).height()),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(lineNo));
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    // Indent guides.
    QPainter painter(viewport());
    painter.setPen(theme::palette().indentGuide);
    const qreal spaceW = QFontMetricsF(font()).horizontalAdvance(u' ');

    QTextBlock block = firstVisibleBlock();
    while (block.isValid()) {
        const QRectF geo = blockBoundingGeometry(block).translated(contentOffset());
        if (geo.top() > event->rect().bottom())
            break;

        if (block.isVisible()) {
            const QString text = block.text();
            int indent = 0;
            while (indent < text.size() && text.at(indent) == u' ')
                ++indent;
            // Skip guides on blank lines; they read as noise.
            if (indent < text.size()) {
                for (int col = m_indentWidth; col < indent; col += m_indentWidth) {
                    const qreal x = contentOffset().x() + col * spaceW;
                    painter.drawLine(QPointF(x, geo.top()), QPointF(x, geo.bottom() - 1));
                }
            }
        }
        block = block.next();
    }
}

QString CodeEditor::leadingWhitespace(const QString &line) const
{
    int i = 0;
    while (i < line.size() && (line.at(i) == u' ' || line.at(i) == u'\t'))
        ++i;
    return line.left(i);
}

void CodeEditor::handleNewline()
{
    QTextCursor cur = textCursor();
    const QString line = cur.block().text();
    const int posInBlock = cur.positionInBlock();

    const QString indent = leadingWhitespace(line);
    const QString step(m_indentWidth, u' ');

    // Nearest non-space characters either side of the caret.
    QChar prev;
    for (int i = posInBlock - 1; i >= 0; --i) {
        if (!line.at(i).isSpace()) {
            prev = line.at(i);
            break;
        }
    }
    QChar next;
    for (int i = posInBlock; i < line.size(); ++i) {
        if (!line.at(i).isSpace()) {
            next = line.at(i);
            break;
        }
    }

    // YAML opens a block after a colon; JSON after a bracket.
    const bool opening = m_language == Language::Yaml
                             ? (prev == u':' || prev == u'-')
                             : (prev == u'{' || prev == u'[');
    const bool closingFollows = m_language == Language::Json && (next == u'}' || next == u']');

    cur.beginEditBlock();
    if (opening && closingFollows) {
        // Caret between a matching pair: open a body and park the caret in it.
        cur.insertText(QStringLiteral("\n") + indent + step + QStringLiteral("\n") + indent);
        cur.movePosition(QTextCursor::PreviousBlock);
        cur.movePosition(QTextCursor::EndOfBlock);
        setTextCursor(cur);
    } else if (opening) {
        cur.insertText(QStringLiteral("\n") + indent + step);
    } else {
        cur.insertText(QStringLiteral("\n") + indent);
    }
    cur.endEditBlock();
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && !(event->modifiers() & Qt::ShiftModifier) && !textCursor().hasSelection()) {
        handleNewline();
        ensureCursorVisible();
        return;
    }

    if (event->key() == Qt::Key_Tab && !textCursor().hasSelection()) {
        textCursor().insertText(QString(m_indentWidth, u' '));
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}
