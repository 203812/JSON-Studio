#pragma once

#include <QString>

namespace jsontools {

struct Result {
    bool ok = false;
    QString text;    // formatted output when ok
    QString message; // human-readable error when !ok
    int line = -1;   // 1-based
    int column = -1; // 1-based
};

// Pretty-print with the given indent. Key order is preserved.
Result format(const QString &input, int indent);

// Single-line output, no spaces.
Result minify(const QString &input);

// Parse only; Result::text is left empty.
Result validate(const QString &input);

} // namespace jsontools
