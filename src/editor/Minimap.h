#pragma once

#include <QWidget>

class CodeEditor;

// Sublime's minimap. Each document line is drawn as a 3px row of 1px-wide
// blocks, coloured from the formats the highlighter already stored on the block
// layout, so the map and the editor can never disagree about syntax colours.
class Minimap : public QWidget
{
    Q_OBJECT

public:
    explicit Minimap(CodeEditor *editor, QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    static constexpr int kWidth = 110;
    static constexpr int kLineHeight = 3;
    static constexpr qreal kCharWidth = 1.0;

    // Pixel offset into the full-height map when the document is taller than
    // the widget; keeps the viewport box tracking the real scroll position.
    int mapOffset() const;
    int visibleLineCount() const;
    void scrollToY(int y);

    CodeEditor *m_editor;
};
