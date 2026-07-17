#include "editor/Minimap.h"

#include "editor/CodeEditor.h"
#include "ui/Theme.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QWheelEvent>

Minimap::Minimap(CodeEditor *editor, QWidget *parent)
    : QWidget(parent)
    , m_editor(editor)
{
    setObjectName("Minimap");
    setFixedWidth(kWidth);
    setCursor(Qt::ArrowCursor);
    setAutoFillBackground(false);

    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, this, qOverload<>(&QWidget::update));
    connect(m_editor->document(), &QTextDocument::contentsChanged, this, qOverload<>(&QWidget::update));
    // The highlighter re-formats asynchronously; repaint when it does.
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, qOverload<>(&QWidget::update));
}

QSize Minimap::sizeHint() const
{
    return {kWidth, 0};
}

int Minimap::visibleLineCount() const
{
    return m_editor->visibleLineCount();
}

int Minimap::mapOffset() const
{
    const int total = m_editor->document()->blockCount();
    const int contentH = total * kLineHeight;
    if (contentH <= height())
        return 0;

    QScrollBar *vsb = m_editor->verticalScrollBar();
    const int range = vsb->maximum() - vsb->minimum();
    if (range <= 0)
        return 0;

    const qreal ratio = qreal(vsb->value() - vsb->minimum()) / range;
    return qRound(ratio * (contentH - height()));
}

void Minimap::paintEvent(QPaintEvent *)
{
    const theme::Palette &p = theme::palette();
    QPainter painter(this);
    painter.fillRect(rect(), p.editorBg);

    QTextDocument *doc = m_editor->document();
    const int offset = mapOffset();
    const int firstLine = qMax(0, offset / kLineHeight);
    const int lastLine = qMin(doc->blockCount() - 1, (offset + height()) / kLineHeight + 1);

    painter.setPen(Qt::NoPen);

    for (int i = firstLine; i <= lastLine; ++i) {
        const QTextBlock block = doc->findBlockByNumber(i);
        if (!block.isValid())
            continue;

        const int y = i * kLineHeight - offset;
        const QString text = block.text();
        if (text.trimmed().isEmpty())
            continue;

        // Reuse the highlighter's own formats for colour.
        const QList<QTextLayout::FormatRange> formats = block.layout() ? block.layout()->formats()
                                                                       : QList<QTextLayout::FormatRange>();

        if (formats.isEmpty()) {
            int lead = 0;
            while (lead < text.size() && text.at(lead).isSpace())
                ++lead;
            painter.setBrush(QColor(p.editorFg.red(), p.editorFg.green(), p.editorFg.blue(), 150));
            painter.drawRect(QRectF(lead * kCharWidth, y, (text.size() - lead) * kCharWidth, kLineHeight - 1));
            continue;
        }

        for (const QTextLayout::FormatRange &fr : formats) {
            QColor col = fr.format.foreground().color();
            col.setAlpha(190);
            painter.setBrush(col);
            const qreal x = fr.start * kCharWidth;
            const qreal w = qMax(kCharWidth, fr.length * kCharWidth);
            if (x > width())
                break;
            painter.drawRect(QRectF(x, y, w, kLineHeight - 1));
        }
    }

    // Viewport box.
    const int boxY = m_editor->firstVisibleLine() * kLineHeight - offset;
    const int boxH = qMax(kLineHeight, visibleLineCount() * kLineHeight);

    painter.setBrush(QColor(255, 255, 255, 18));
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(0, boxY, width(), boxH));
}

void Minimap::scrollToY(int y)
{
    const int line = (y + mapOffset()) / kLineHeight;
    QScrollBar *vsb = m_editor->verticalScrollBar();
    vsb->setValue(qBound(vsb->minimum(), line - visibleLineCount() / 2, vsb->maximum()));
}

void Minimap::mousePressEvent(QMouseEvent *event)
{
    scrollToY(qRound(event->position().y()));
}

void Minimap::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        scrollToY(qRound(event->position().y()));
}

void Minimap::wheelEvent(QWheelEvent *event)
{
    QCoreApplication::sendEvent(m_editor->viewport(), event);
}
