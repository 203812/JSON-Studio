#pragma once

#include <QString>

namespace version {

// The version this build reports, from CMake's project() via a compile define.
QString current();

// Semantic-ish comparison of "1.2.3" style strings. A leading 'v' and any
// pre-release suffix ("-beta.1") are tolerated. Missing components count as 0,
// so "1.2" == "1.2.0". Returns <0 if a<b, 0 if equal, >0 if a>b.
int compare(const QString &a, const QString &b);

// True when latest is strictly newer than current.
bool isNewer(const QString &latest, const QString &currentVersion);

} // namespace version
