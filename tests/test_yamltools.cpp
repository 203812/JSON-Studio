// Tests for JSON <-> YAML. The interesting risk is scalar typing: YAML plain
// scalars are untyped, so "1" quoted must stay a string while 1 bare must
// become a number. Round-tripping is the strongest check we have.

#include "Check.h"

#include "core/JsonTools.h"
#include "core/YamlTools.h"

namespace {

void testJsonToYamlBasics()
{
    const auto r = yamltools::jsonToYaml(QStringLiteral(R"({"name":"studio","port":8080,"debug":true})"));
    check::that(r.ok, "json->yaml: parses", r.message);
    check::that(r.text.contains(QStringLiteral("name: studio")), "json->yaml: string value", r.text);
    check::that(r.text.contains(QStringLiteral("port: 8080")), "json->yaml: int value", r.text);
    check::that(r.text.contains(QStringLiteral("debug: true")), "json->yaml: bool value", r.text);
}

void testJsonToYamlOrder()
{
    const auto r = yamltools::jsonToYaml(QStringLiteral(R"({"z":1,"m":2,"a":3})"));
    check::that(r.ok, "json->yaml: ordered doc parses");
    const int z = r.text.indexOf(QStringLiteral("z:"));
    const int m = r.text.indexOf(QStringLiteral("m:"));
    const int a = r.text.indexOf(QStringLiteral("a:"));
    check::that(z >= 0 && z < m && m < a, "json->yaml: key order preserved", r.text);
}

void testJsonToYamlNested()
{
    const auto r = yamltools::jsonToYaml(QStringLiteral(R"({"a":{"b":[1,2]},"e":{},"f":[]})"));
    check::that(r.ok, "json->yaml: nested parses", r.message);
    check::that(r.text.contains(QStringLiteral("- 1")), "json->yaml: sequence item", r.text);
    // Empty containers must not silently vanish.
    check::that(r.text.contains(QStringLiteral("e: {}")), "json->yaml: empty object kept", r.text);
    check::that(r.text.contains(QStringLiteral("f: []")), "json->yaml: empty array kept", r.text);
}

void testJsonToYamlRejectsBadJson()
{
    const auto r = yamltools::jsonToYaml(QStringLiteral("{not json"));
    check::that(!r.ok, "json->yaml: bad json rejected");
    check::that(!r.message.isEmpty(), "json->yaml: bad json has a message");

    const auto e = yamltools::jsonToYaml(QStringLiteral("  "));
    check::that(!e.ok, "json->yaml: empty rejected");
}

void testYamlToJsonScalarTyping()
{
    const QString yaml = QStringLiteral("a: 1\n"
                                        "b: \"1\"\n"
                                        "c: 1.5\n"
                                        "d: true\n"
                                        "e: \"true\"\n"
                                        "f: null\n"
                                        "g: ~\n"
                                        "h: hello\n");
    const auto r = yamltools::yamlToJson(yaml, -1);
    check::that(r.ok, "yaml->json: parses", r.message);

    // Quoting is the only thing separating b from a, and e from d.
    check::that(r.text.contains(QStringLiteral("\"a\":1")), "yaml->json: bare 1 is a number", r.text);
    check::that(r.text.contains(QStringLiteral("\"b\":\"1\"")), "yaml->json: quoted 1 stays a string", r.text);
    check::that(r.text.contains(QStringLiteral("\"c\":1.5")), "yaml->json: float", r.text);
    check::that(r.text.contains(QStringLiteral("\"d\":true")), "yaml->json: bare true is a bool", r.text);
    check::that(r.text.contains(QStringLiteral("\"e\":\"true\"")), "yaml->json: quoted true stays a string",
                r.text);
    check::that(r.text.contains(QStringLiteral("\"f\":null")), "yaml->json: null", r.text);
    check::that(r.text.contains(QStringLiteral("\"g\":null")), "yaml->json: ~ is null", r.text);
    check::that(r.text.contains(QStringLiteral("\"h\":\"hello\"")), "yaml->json: bare word is a string", r.text);
}

void testYamlToJsonStructure()
{
    const QString yaml = QStringLiteral("server:\n"
                                        "  host: localhost\n"
                                        "  ports:\n"
                                        "    - 80\n"
                                        "    - 443\n");
    const auto r = yamltools::yamlToJson(yaml, -1);
    check::that(r.ok, "yaml->json: nested parses", r.message);
    check::eq(r.text, QStringLiteral(R"({"server":{"host":"localhost","ports":[80,443]}})"),
              "yaml->json: nested structure");
}

void testYamlToJsonComments()
{
    const auto r = yamltools::yamlToJson(QStringLiteral("# leading\na: 1  # trailing\n"), -1);
    check::that(r.ok, "yaml->json: comments parse", r.message);
    check::eq(r.text, QStringLiteral(R"({"a":1})"), "yaml->json: comments dropped");
}

void testYamlToJsonBlockScalar()
{
    const auto r = yamltools::yamlToJson(QStringLiteral("text: |\n  line one\n  line two\n"), -1);
    check::that(r.ok, "yaml->json: block scalar parses", r.message);
    check::eq(r.text, QStringLiteral(R"({"text":"line one\nline two\n"})"), "yaml->json: block scalar content");
}

void testYamlToJsonErrors()
{
    // Bad indentation: a real parser error with a position.
    const auto r = yamltools::yamlToJson(QStringLiteral("a: 1\n  b: 2\n"), -1);
    check::that(!r.ok, "yaml->json: bad indent rejected");
    check::that(r.line > 0, "yaml->json: error has a 1-based line", QStringLiteral("line=%1").arg(r.line));
    check::that(!r.message.contains(QStringLiteral("yaml-cpp")), "yaml->json: message tidied", r.message);

    const auto e = yamltools::yamlToJson(QStringLiteral("   "), -1);
    check::that(!e.ok, "yaml->json: empty rejected");
}

void testRoundTrip()
{
    // The strongest signal: JSON -> YAML -> JSON must be a fixed point.
    const QString original = QStringLiteral(
        R"({"name":"studio","version":"0.1.0","port":8080,"ratio":4.8,"debug":true,)"
        R"("successor":null,"tags":["a","b"],"nested":{"deep":{"x":1}},"empty":{},"none":[]})");

    const auto yaml = yamltools::jsonToYaml(original);
    check::that(yaml.ok, "roundtrip: to yaml", yaml.message);

    const auto back = yamltools::yamlToJson(yaml.text, -1);
    check::that(back.ok, "roundtrip: back to json", back.message);
    check::eq(back.text, original, "roundtrip: json -> yaml -> json is lossless");
}

void testRoundTripStringyValues()
{
    // Values that look like other types must survive the trip as strings.
    const QString original = QStringLiteral(
        R"({"version":"1.0","zip":"01234","flag":"true","nil":"null","num":"42","when":"12:30"})");

    const auto yaml = yamltools::jsonToYaml(original);
    check::that(yaml.ok, "roundtrip stringy: to yaml", yaml.message);

    const auto back = yamltools::yamlToJson(yaml.text, -1);
    check::that(back.ok, "roundtrip stringy: back to json", back.message);
    check::eq(back.text, original, "roundtrip stringy: type-lookalike strings stay strings");
}

} // namespace

void runYamlToolsTests()
{
    testJsonToYamlBasics();
    testJsonToYamlOrder();
    testJsonToYamlNested();
    testJsonToYamlRejectsBadJson();
    testYamlToJsonScalarTyping();
    testYamlToJsonStructure();
    testYamlToJsonComments();
    testYamlToJsonBlockScalar();
    testYamlToJsonErrors();
    testRoundTrip();
    testRoundTripStringyValues();
}
