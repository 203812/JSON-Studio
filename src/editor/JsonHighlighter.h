#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

// Hand-rolled JSON tokeniser rather than regexes: it keeps key-vs-string
// classification reliable (a string is a key only when the next non-space
// character is a colon) and it is what the minimap reads back for its colours.
class JsonHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit JsonHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_key;
    QTextCharFormat m_string;
    QTextCharFormat m_number;
    QTextCharFormat m_constant;
    QTextCharFormat m_punctuation;
};
