#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

// YAML needs real cross-line state, unlike JSON: a block scalar ("key: |")
// swallows every following line indented deeper than its parent. That indent is
// carried in the block state, so the highlighter knows where the literal ends.
class YamlHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit YamlHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    // Block state encoding: -1 / 0 = normal, otherwise (indent + 1) of the line
    // that opened the block scalar. Qt uses -1 for "no state", so we never
    // store a bare 0 for a real indent.
    static constexpr int kNoState = 0;

    void highlightScalar(const QString &text, int start, int len);
    int indentOf(const QString &text) const;

    QTextCharFormat m_key;
    QTextCharFormat m_string;
    QTextCharFormat m_number;
    QTextCharFormat m_constant;
    QTextCharFormat m_punctuation;
    QTextCharFormat m_comment;
    QTextCharFormat m_anchor;
};
