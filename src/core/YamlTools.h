#pragma once

#include <QString>

namespace yamltools {

struct Result {
    bool ok = false;
    QString text;
    QString message;
    int line = -1;   // 1-based, -1 when the error has no position
    int column = -1; // 1-based
};

// JSON -> YAML. Key order is preserved.
Result jsonToYaml(const QString &json);

// YAML -> JSON. Negative indent means compact, matching jsontools::minify.
Result yamlToJson(const QString &yaml, int indent);

} // namespace yamltools
