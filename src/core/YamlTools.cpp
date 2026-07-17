#include "core/YamlTools.h"

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include <string>

namespace yamltools {
namespace {

using ordered_json = nlohmann::ordered_json;

// Deeply nested input would otherwise recurse until the stack gives out, and a
// stack overflow is a crash rather than an error message.
constexpr int kMaxDepth = 200;

struct TooDeep {
};

// ---------------------------------------------------------------- JSON -> YAML

// Would YAML read this text back as something other than the string it is?
// "1.0" resolves to a float, "true" to a bool, and "01234" to octal 668 - a zip
// code silently becoming a different number. Those must be emitted quoted.
// This mirrors scalarToJson below: we quote exactly when a plain scalar would
// not survive the round trip.
bool resolvesToNonString(const std::string &s)
{
    if (s.empty())
        return true;
    if (s == "~" || s == "null" || s == "Null" || s == "NULL")
        return true;

    const YAML::Node n(s);
    bool b = false;
    if (YAML::convert<bool>::decode(n, b))
        return true;
    std::int64_t i = 0;
    if (YAML::convert<std::int64_t>::decode(n, i))
        return true;
    double d = 0.0;
    if (YAML::convert<double>::decode(n, d))
        return true;
    return false;
}

bool needsQuoting(const std::string &s)
{
    if (resolvesToNonString(s))
        return true;

    // Padding would be eaten by a plain scalar.
    if (std::isspace(static_cast<unsigned char>(s.front()))
        || std::isspace(static_cast<unsigned char>(s.back())))
        return true;

    // A leading YAML indicator changes how the line parses.
    static const std::string kIndicators = "-?:,[]{}#&*!|>'\"%@`";
    if (kIndicators.find(s.front()) != std::string::npos)
        return true;

    // These sequences end a plain scalar mid-string.
    if (s.find(": ") != std::string::npos || s.find(" #") != std::string::npos)
        return true;

    return false;
}

void emitJson(YAML::Emitter &out, const ordered_json &j, int depth)
{
    if (depth > kMaxDepth)
        throw TooDeep{};

    switch (j.type()) {
    case ordered_json::value_t::object:
        if (j.empty()) {
            out << YAML::Flow;
            out << YAML::BeginMap << YAML::EndMap;
            return;
        }
        out << YAML::BeginMap;
        for (const auto &item : j.items()) {
            out << YAML::Key << item.key() << YAML::Value;
            emitJson(out, item.value(), depth + 1);
        }
        out << YAML::EndMap;
        return;

    case ordered_json::value_t::array:
        if (j.empty()) {
            out << YAML::Flow;
            out << YAML::BeginSeq << YAML::EndSeq;
            return;
        }
        out << YAML::BeginSeq;
        for (const auto &v : j)
            emitJson(out, v, depth + 1);
        out << YAML::EndSeq;
        return;

    case ordered_json::value_t::string: {
        const std::string s = j.get<std::string>();
        // Multi-line text is left to the emitter, which picks a block or an
        // escaped form; both read back as the same string.
        if (s.find('\n') == std::string::npos && needsQuoting(s))
            out << YAML::DoubleQuoted;
        out << s;
        return;
    }
    case ordered_json::value_t::boolean:
        out << j.get<bool>();
        return;
    case ordered_json::value_t::number_integer:
        out << j.get<std::int64_t>();
        return;
    case ordered_json::value_t::number_unsigned:
        out << j.get<std::uint64_t>();
        return;
    case ordered_json::value_t::number_float:
        out << j.get<double>();
        return;
    case ordered_json::value_t::null:
        out << YAML::Null;
        return;
    default:
        out << YAML::Null;
        return;
    }
}

// ---------------------------------------------------------------- YAML -> JSON

// A plain (unquoted) YAML scalar is untyped: "true", "1" and "1.5" must become
// JSON bool/int/float, while a quoted "1" must stay a string. yaml-cpp tags
// quoted scalars "!" and leaves plain ones "?", which is exactly that
// distinction.
ordered_json scalarToJson(const YAML::Node &node)
{
    const std::string &s = node.Scalar();

    if (node.Tag() == "!")
        return s;

    if (s.empty() || s == "~" || s == "null" || s == "Null" || s == "NULL")
        return nullptr;

    bool b = false;
    if (YAML::convert<bool>::decode(node, b))
        return b;

    std::int64_t i = 0;
    if (YAML::convert<std::int64_t>::decode(node, i))
        return i;

    double d = 0.0;
    if (YAML::convert<double>::decode(node, d))
        return d;

    return s;
}

ordered_json nodeToJson(const YAML::Node &node, int depth)
{
    if (depth > kMaxDepth)
        throw TooDeep{};

    switch (node.Type()) {
    case YAML::NodeType::Null:
        return nullptr;

    case YAML::NodeType::Scalar:
        return scalarToJson(node);

    case YAML::NodeType::Sequence: {
        ordered_json arr = ordered_json::array();
        for (const auto &child : node)
            arr.push_back(nodeToJson(child, depth + 1));
        return arr;
    }

    case YAML::NodeType::Map: {
        ordered_json obj = ordered_json::object();
        for (const auto &kv : node) {
            // JSON keys must be strings; YAML allows any node as a key.
            if (!kv.first.IsScalar())
                throw YAML::Exception(kv.first.Mark(), "mapping key is not a scalar");
            obj[kv.first.Scalar()] = nodeToJson(kv.second, depth + 1);
        }
        return obj;
    }

    case YAML::NodeType::Undefined:
    default:
        return nullptr;
    }
}

QString tidy(const std::string &what)
{
    QString m = QString::fromStdString(what);
    // yaml-cpp prefixes "yaml-cpp: error at line X, column Y: <detail>".
    const int colon = m.lastIndexOf(QStringLiteral(": "));
    if (colon >= 0 && m.startsWith(QStringLiteral("yaml-cpp")))
        m = m.mid(colon + 2);
    if (!m.isEmpty())
        m[0] = m.at(0).toUpper();
    return m;
}

} // namespace

Result jsonToYaml(const QString &json)
{
    Result r;
    if (json.trimmed().isEmpty()) {
        r.message = QStringLiteral("Document is empty");
        return r;
    }

    ordered_json parsed;
    try {
        parsed = ordered_json::parse(json.toStdString());
    } catch (const ordered_json::parse_error &e) {
        // Reuse the JSON parse error verbatim; the source really is JSON here.
        r.message = QString::fromUtf8(e.what());
        const int colon = r.message.indexOf(QStringLiteral(": "));
        if (colon >= 0)
            r.message = r.message.mid(colon + 2);
        return r;
    }

    try {
        YAML::Emitter out;
        out.SetIndent(2);
        emitJson(out, parsed, 0);
        if (!out.good()) {
            r.message = QStringLiteral("YAML emitter failed: %1").arg(QString::fromStdString(out.GetLastError()));
            return r;
        }
        r.ok = true;
        r.text = QString::fromStdString(out.c_str());
        if (!r.text.endsWith(u'\n'))
            r.text.append(u'\n');
    } catch (const TooDeep &) {
        r.message = QStringLiteral("Document nests deeper than %1 levels").arg(kMaxDepth);
    } catch (const std::exception &e) {
        r.message = QString::fromUtf8(e.what());
    }
    return r;
}

Result yamlToJson(const QString &yaml, int indent)
{
    Result r;
    if (yaml.trimmed().isEmpty()) {
        r.message = QStringLiteral("Document is empty");
        return r;
    }

    try {
        const YAML::Node root = YAML::Load(yaml.toStdString());
        const ordered_json j = nodeToJson(root, 0);
        r.ok = true;
        r.text = QString::fromStdString(indent < 0 ? j.dump() : j.dump(indent));
    } catch (const YAML::ParserException &e) {
        r.line = e.mark.line + 1; // yaml-cpp marks are 0-based
        r.column = e.mark.column + 1;
        r.message = tidy(e.msg);
    } catch (const YAML::Exception &e) {
        r.line = e.mark.line + 1;
        r.column = e.mark.column + 1;
        r.message = tidy(e.msg);
    } catch (const TooDeep &) {
        r.message = QStringLiteral("Document nests deeper than %1 levels").arg(kMaxDepth);
    } catch (const std::exception &e) {
        r.message = QString::fromUtf8(e.what());
    }
    return r;
}

} // namespace yamltools
