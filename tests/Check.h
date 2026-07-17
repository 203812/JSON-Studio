#pragma once

#include <QString>

#include <cstdio>

// Minimal assertion helpers shared by the core test files. Deliberately not a
// framework: these tests are pure functions in, values out.
namespace check {

inline int g_failures = 0;
inline int g_checks = 0;

inline void that(bool cond, const char *what, const QString &detail = {})
{
    ++g_checks;
    if (cond)
        return;
    ++g_failures;
    std::fprintf(stderr, "FAIL: %s%s%s\n", what, detail.isEmpty() ? "" : " | ",
                 detail.isEmpty() ? "" : qPrintable(detail));
}

inline void eq(const QString &got, const QString &want, const char *what)
{
    that(got == want, what, QStringLiteral("got [%1] want [%2]").arg(got, want));
}

inline void eq(int got, int want, const char *what)
{
    that(got == want, what, QStringLiteral("got %1 want %2").arg(got).arg(want));
}

} // namespace check

void runJsonToolsTests();
void runYamlToolsTests();
void runVersionTests();
