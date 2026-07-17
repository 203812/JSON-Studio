#include "core/JsonTools.h"

#include <nlohmann/json.hpp>

#include <string>

namespace jsontools {
namespace {

using ordered_json = nlohmann::ordered_json;

// nlohmann reports a 1-based *byte* offset of the last character it read.
// Translate that into an editor line/column.
void byteToLineCol(const std::string &s, std::size_t byte, int &line, int &column)
{
    line = 1;
    column = 1;
    const std::size_t limit = byte == 0 ? 0 : qMin(byte - 1, s.size());
    for (std::size_t i = 0; i < limit; ++i) {
        if (s[i] == '\n') {
            ++line;
            column = 1;
        } else {
            // Don't count UTF-8 continuation bytes as columns.
            if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80)
                ++column;
        }
    }
}

// nlohmann's what() is "[json.exception.parse_error.101] parse error at line 1,
// column 3: <detail>". Keep only <detail> - we render position ourselves.
QString tidyMessage(const char *what)
{
    QString m = QString::fromUtf8(what);
    const int colon = m.indexOf(QStringLiteral(": "));
    if (colon >= 0)
        m = m.mid(colon + 2);
    if (!m.isEmpty())
        m[0] = m.at(0).toUpper();
    return m;
}

Result parseThen(const QString &input, int indent, bool produceText)
{
    Result r;
    const std::string s = input.toStdString();

    if (input.trimmed().isEmpty()) {
        r.message = QStringLiteral("Document is empty");
        return r;
    }

    try {
        const ordered_json j = ordered_json::parse(s);
        r.ok = true;
        if (produceText)
            r.text = QString::fromStdString(indent < 0 ? j.dump() : j.dump(indent));
    } catch (const ordered_json::parse_error &e) {
        byteToLineCol(s, e.byte, r.line, r.column);
        r.message = tidyMessage(e.what());
    } catch (const std::exception &e) {
        r.message = QString::fromUtf8(e.what());
    }
    return r;
}

} // namespace

Result format(const QString &input, int indent)
{
    return parseThen(input, qMax(0, indent), true);
}

Result minify(const QString &input)
{
    return parseThen(input, -1, true);
}

Result validate(const QString &input)
{
    return parseThen(input, 0, false);
}

} // namespace jsontools
