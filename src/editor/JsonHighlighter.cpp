#include "editor/JsonHighlighter.h"

#include "ui/Theme.h"

JsonHighlighter::JsonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    const theme::Palette &p = theme::palette();
    m_key.setForeground(p.synKey);
    m_string.setForeground(p.synString);
    m_number.setForeground(p.synNumber);
    m_constant.setForeground(p.synConstant);
    m_punctuation.setForeground(p.synPunctuation);
}

void JsonHighlighter::highlightBlock(const QString &text)
{
    const int n = text.size();
    int i = 0;

    while (i < n) {
        const QChar c = text.at(i);

        if (c.isSpace()) {
            ++i;
            continue;
        }

        if (c == u'"') {
            const int start = i;
            ++i;
            bool closed = false;
            while (i < n) {
                if (text.at(i) == u'\\') {
                    i += 2;
                    continue;
                }
                if (text.at(i) == u'"') {
                    ++i;
                    closed = true;
                    break;
                }
                ++i;
            }
            // A key is a string immediately followed by a colon.
            int j = i;
            while (j < n && text.at(j).isSpace())
                ++j;
            const bool isKey = closed && j < n && text.at(j) == u':';
            setFormat(start, qMin(i, n) - start, isKey ? m_key : m_string);
            continue;
        }

        if (c.isDigit() || (c == u'-' && i + 1 < n && text.at(i + 1).isDigit())) {
            const int start = i;
            ++i;
            while (i < n) {
                const QChar d = text.at(i);
                if (d.isDigit() || d == u'.' || d == u'e' || d == u'E' || d == u'+' || d == u'-')
                    ++i;
                else
                    break;
            }
            setFormat(start, i - start, m_number);
            continue;
        }

        if (c.isLetter()) {
            const int start = i;
            while (i < n && text.at(i).isLetter())
                ++i;
            const QStringView word = QStringView(text).mid(start, i - start);
            if (word == u"true" || word == u"false" || word == u"null")
                setFormat(start, i - start, m_constant);
            continue;
        }

        if (c == u'{' || c == u'}' || c == u'[' || c == u']' || c == u',' || c == u':') {
            setFormat(i, 1, m_punctuation);
            ++i;
            continue;
        }

        ++i;
    }
}
