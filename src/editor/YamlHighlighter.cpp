#include "editor/YamlHighlighter.h"

#include "ui/Theme.h"

YamlHighlighter::YamlHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    const theme::Palette &p = theme::palette();
    m_key.setForeground(p.synKey);
    m_string.setForeground(p.synString);
    m_number.setForeground(p.synNumber);
    m_constant.setForeground(p.synConstant);
    m_punctuation.setForeground(p.synPunctuation);
    m_comment.setForeground(p.chromeFgDim);
    m_anchor.setForeground(p.accent);
}

int YamlHighlighter::indentOf(const QString &text) const
{
    int i = 0;
    while (i < text.size() && text.at(i) == u' ')
        ++i;
    return i;
}

// Colours a value: quoted string, number, or one of YAML's constants.
void YamlHighlighter::highlightScalar(const QString &text, int start, int len)
{
    if (len <= 0)
        return;

    // Trim the span so formats land on the value, not the padding around it.
    int s = start;
    while (s < start + len && text.at(s).isSpace())
        ++s;
    int e = start + len;
    while (e > s && text.at(e - 1).isSpace())
        --e;
    const int n = e - s;
    if (n <= 0)
        return;

    const QChar first = text.at(s);

    if (first == u'&' || first == u'*') {
        setFormat(s, n, m_anchor);
        return;
    }
    if (first == u'"' || first == u'\'') {
        setFormat(s, n, m_string);
        return;
    }
    if (first == u'|' || first == u'>') {
        setFormat(s, n, m_punctuation);
        return;
    }

    const QStringView word = QStringView(text).mid(s, n);
    if (word == u"true" || word == u"false" || word == u"null" || word == u"~" || word == u"yes"
        || word == u"no" || word == u"on" || word == u"off") {
        setFormat(s, n, m_constant);
        return;
    }

    // Numeric only if it reads like one number: a sign may lead or follow an
    // exponent, never sit in the middle. Otherwise "2026-07-16" (a date) and
    // "1.2.3" (a version) would colour as numbers.
    bool numeric = first.isDigit() || ((first == u'-' || first == u'+') && n > 1);
    int digits = 0;
    int dots = 0;
    for (int i = 0; numeric && i < n; ++i) {
        const QChar c = text.at(s + i);
        if (c.isDigit()) {
            ++digits;
        } else if (c == u'.') {
            ++dots;
        } else if (c == u'-' || c == u'+') {
            const QChar before = i > 0 ? text.at(s + i - 1) : QChar();
            if (i != 0 && before != u'e' && before != u'E')
                numeric = false;
        } else if (c != u'e' && c != u'E') {
            numeric = false;
        }
    }
    setFormat(s, n, (numeric && digits > 0 && dots <= 1) ? m_number : m_string);
}

void YamlHighlighter::highlightBlock(const QString &text)
{
    const int prevState = previousBlockState();
    const int n = text.size();

    // Inside a block scalar: everything indented deeper than its opener is
    // literal text, and a blank line does not end it.
    if (prevState > kNoState) {
        const int openerIndent = prevState - 1;
        if (text.trimmed().isEmpty() || indentOf(text) > openerIndent) {
            setFormat(0, n, m_string);
            setCurrentBlockState(prevState);
            return;
        }
    }
    setCurrentBlockState(kNoState);

    if (n == 0)
        return;

    const int indent = indentOf(text);
    int i = indent;
    if (i >= n)
        return;

    // Comment line.
    if (text.at(i) == u'#') {
        setFormat(i, n - i, m_comment);
        return;
    }

    // Document markers.
    if (QStringView(text).mid(i).startsWith(u"---") || QStringView(text).mid(i).startsWith(u"...")) {
        setFormat(i, 3, m_punctuation);
        return;
    }

    // Sequence entries can be followed by a key on the same line ("- name: x").
    while (i < n && text.at(i) == u'-' && i + 1 < n && text.at(i + 1) == u' ') {
        setFormat(i, 1, m_punctuation);
        i += 2;
        while (i < n && text.at(i) == u' ')
            ++i;
    }
    if (i >= n)
        return;
    // A lone "-" opening a nested block.
    if (i == n - 1 && text.at(i) == u'-') {
        setFormat(i, 1, m_punctuation);
        return;
    }

    // Trailing comment, so it does not get parsed as part of a value.
    int limit = n;
    for (int c = i; c < n; ++c) {
        if (text.at(c) == u'#' && c > 0 && text.at(c - 1) == u' ') {
            setFormat(c, n - c, m_comment);
            limit = c;
            break;
        }
    }

    // Find the key separator: a colon followed by space or end of line.
    int colon = -1;
    bool inQuote = false;
    QChar quoteCh;
    for (int c = i; c < limit; ++c) {
        const QChar ch = text.at(c);
        if (inQuote) {
            if (ch == quoteCh)
                inQuote = false;
            continue;
        }
        if (ch == u'"' || ch == u'\'') {
            inQuote = true;
            quoteCh = ch;
            continue;
        }
        if (ch == u':' && (c + 1 >= limit || text.at(c + 1) == u' ')) {
            colon = c;
            break;
        }
    }

    if (colon < 0) {
        // No key on this line: it is a bare sequence value.
        highlightScalar(text, i, limit - i);
        return;
    }

    setFormat(i, colon - i, m_key);
    setFormat(colon, 1, m_punctuation);

    // Does the value open a block scalar?
    int v = colon + 1;
    while (v < limit && text.at(v) == u' ')
        ++v;
    if (v < limit && (text.at(v) == u'|' || text.at(v) == u'>')) {
        setFormat(v, limit - v, m_punctuation);
        setCurrentBlockState(indent + 1);
        return;
    }

    highlightScalar(text, colon + 1, limit - colon - 1);
}
