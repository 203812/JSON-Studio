#include "core/Version.h"

#include <QStringList>

#ifndef JSONSTUDIO_VERSION
#define JSONSTUDIO_VERSION "0.0.0"
#endif

namespace version {
namespace {

// Splits the numeric core ("1.2.3") from any pre-release suffix ("-beta").
// Returns the list of numeric components; the suffix presence is reported out.
QList<int> numericParts(const QString &raw, bool *hasPreRelease)
{
    QString s = raw.trimmed();
    if (s.startsWith(u'v') || s.startsWith(u'V'))
        s = s.mid(1);

    // Drop build metadata ("+abc") and pre-release ("-beta.1") for the core.
    int cut = s.size();
    const int dash = s.indexOf(u'-');
    const int plus = s.indexOf(u'+');
    if (dash >= 0)
        cut = qMin(cut, dash);
    if (plus >= 0)
        cut = qMin(cut, plus);

    if (hasPreRelease)
        *hasPreRelease = (dash >= 0 && dash < (plus >= 0 ? plus : s.size()));

    const QString core = s.left(cut);
    QList<int> parts;
    const QStringList tokens = core.split(u'.', Qt::SkipEmptyParts);
    for (const QString &t : tokens) {
        bool ok = false;
        const int n = t.toInt(&ok);
        parts.append(ok ? n : 0);
    }
    return parts;
}

} // namespace

QString current()
{
    return QStringLiteral(JSONSTUDIO_VERSION);
}

int compare(const QString &a, const QString &b)
{
    bool preA = false;
    bool preB = false;
    const QList<int> pa = numericParts(a, &preA);
    const QList<int> pb = numericParts(b, &preB);

    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int va = i < pa.size() ? pa.at(i) : 0;
        const int vb = i < pb.size() ? pb.at(i) : 0;
        if (va != vb)
            return va < vb ? -1 : 1;
    }

    // Equal numeric core: a pre-release ("1.2.0-beta") is older than the final
    // ("1.2.0").
    if (preA != preB)
        return preA ? -1 : 1;
    return 0;
}

bool isNewer(const QString &latest, const QString &currentVersion)
{
    return compare(latest, currentVersion) > 0;
}

} // namespace version
