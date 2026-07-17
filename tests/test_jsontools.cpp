// Tests for the pure JSON logic. The byte-offset -> line/column mapping is the
// part most likely to be subtly wrong, so it gets the most cases.

#include "Check.h"

#include "core/JsonTools.h"

namespace {

void testFormat()
{
    const auto r = jsontools::format(QStringLiteral(R"({"b":1,"a":[2,3]})"), 2);
    check::that(r.ok, "format: valid input parses", r.message);
    check::eq(r.text, QStringLiteral("{\n  \"b\": 1,\n  \"a\": [\n    2,\n    3\n  ]\n}"),
              "format: 2-space output");

    // Key order must survive; that is why we use ordered_json.
    const auto r4 = jsontools::format(QStringLiteral(R"({"z":1,"a":2})"), 4);
    check::that(r4.text.indexOf(QStringLiteral("\"z\"")) < r4.text.indexOf(QStringLiteral("\"a\"")),
                "format: key order preserved");
    check::that(r4.text.contains(QStringLiteral("\n    \"z\"")), "format: 4-space indent honoured");
}

void testMinify()
{
    const auto r = jsontools::minify(QStringLiteral("{\n  \"a\" : [ 1,  2 ]\n}"));
    check::that(r.ok, "minify: parses");
    check::eq(r.text, QStringLiteral(R"({"a":[1,2]})"), "minify: no whitespace");
}

void testValidateOk()
{
    check::that(jsontools::validate(QStringLiteral(R"({"a":null})")).ok, "validate: object ok");
    check::that(jsontools::validate(QStringLiteral("[]")).ok, "validate: empty array ok");
    check::that(jsontools::validate(QStringLiteral("42")).ok, "validate: bare number ok");
    check::that(jsontools::validate(QStringLiteral(R"("hi")")).ok, "validate: bare string ok");
}

void testEmpty()
{
    const auto r = jsontools::validate(QStringLiteral("   \n  \t "));
    check::that(!r.ok, "validate: whitespace-only is not valid");
    check::that(!r.message.isEmpty(), "validate: empty doc has a message");
}

void testErrorPosition()
{
    // Missing value after "b": the parser hits "}" on line 4.
    const QString bad = QStringLiteral("{\n  \"a\": 1,\n  \"b\":\n}");
    const auto r = jsontools::validate(bad);
    check::that(!r.ok, "validate: malformed is rejected");
    check::eq(r.line, 4, "error line points at the offending token");
    check::that(r.column > 0, "error column is 1-based", QStringLiteral("col=%1").arg(r.column));

    // Error on the very first character.
    const auto r1 = jsontools::validate(QStringLiteral("}"));
    check::that(!r1.ok, "validate: lone brace rejected");
    check::eq(r1.line, 1, "first-char error is line 1");
    check::eq(r1.column, 1, "first-char error is column 1");

    // Error after several newlines.
    const auto r2 = jsontools::validate(QStringLiteral("[\n1,\n2,\n@\n]"));
    check::that(!r2.ok, "validate: stray @ rejected");
    check::eq(r2.line, 4, "error line counts newlines");
    check::eq(r2.column, 1, "error column resets each line");
}

void testUtf8Columns()
{
    // Column must count characters, not UTF-8 bytes: the accented chars before
    // the error are multi-byte, and a naive byte count would overshoot.
    const QString s = QString::fromUtf8("[\"\xC3\xA9\xC3\xA9\xC3\xA9\" @]");
    const auto r = jsontools::validate(s);
    check::that(!r.ok, "validate: utf8 sample rejected");
    check::eq(r.line, 1, "utf8: single line");
    // ["ééé" @]  -> the @ is the 8th character (1-based).
    check::eq(r.column, 8, "utf8: column counts characters, not bytes");
}

void testMessageTidied()
{
    const auto r = jsontools::validate(QStringLiteral("{,}"));
    check::that(!r.ok, "validate: {,} rejected");
    check::that(!r.message.contains(QStringLiteral("json.exception")), "message: exception id stripped",
                r.message);
    check::that(!r.message.contains(QStringLiteral("parse error at line")), "message: position prefix stripped",
                r.message);
    check::that(!r.message.isEmpty() && r.message.at(0).isUpper(), "message: starts capitalised", r.message);
}

} // namespace

void runJsonToolsTests()
{
    testFormat();
    testMinify();
    testValidateOk();
    testEmpty();
    testErrorPosition();
    testUtf8Columns();
    testMessageTidied();
}
