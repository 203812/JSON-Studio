// Version comparison drives the update check; an off-by-one here means either
// nagging users who are current, or hiding a real update. So it is tested hard.

#include "Check.h"

#include "core/Version.h"

namespace {

void testOrdering()
{
    check::that(version::compare(QStringLiteral("1.0.0"), QStringLiteral("1.0.1")) < 0, "1.0.0 < 1.0.1");
    check::that(version::compare(QStringLiteral("1.0.1"), QStringLiteral("1.0.0")) > 0, "1.0.1 > 1.0.0");
    check::that(version::compare(QStringLiteral("1.2.0"), QStringLiteral("1.2.0")) == 0, "equal");
    check::that(version::compare(QStringLiteral("2.0.0"), QStringLiteral("1.9.9")) > 0, "major dominates");
    check::that(version::compare(QStringLiteral("0.10.0"), QStringLiteral("0.9.0")) > 0, "10 > 9 numerically");
}

void testTolerantParsing()
{
    // A leading 'v' (GitHub tag style) must not matter.
    check::that(version::compare(QStringLiteral("v1.2.3"), QStringLiteral("1.2.3")) == 0, "v-prefix ignored");
    // Missing components are zero.
    check::that(version::compare(QStringLiteral("1.2"), QStringLiteral("1.2.0")) == 0, "1.2 == 1.2.0");
    check::that(version::compare(QStringLiteral("1"), QStringLiteral("1.0.0")) == 0, "1 == 1.0.0");
    // Whitespace tolerated.
    check::that(version::compare(QStringLiteral("  1.2.3 "), QStringLiteral("1.2.3")) == 0, "trim");
}

void testPreRelease()
{
    // A pre-release is older than its final release.
    check::that(version::compare(QStringLiteral("1.2.0-beta.1"), QStringLiteral("1.2.0")) < 0,
                "beta < final");
    check::that(version::compare(QStringLiteral("1.2.0"), QStringLiteral("1.2.0-rc.2")) > 0,
                "final > rc");
    // Build metadata is ignored for ordering.
    check::that(version::compare(QStringLiteral("1.2.0+build9"), QStringLiteral("1.2.0")) == 0,
                "build metadata ignored");
}

void testIsNewer()
{
    check::that(version::isNewer(QStringLiteral("v0.2.0"), QStringLiteral("0.1.0")), "0.2.0 newer than 0.1.0");
    check::that(!version::isNewer(QStringLiteral("v0.1.0"), QStringLiteral("0.1.0")), "same is not newer");
    check::that(!version::isNewer(QStringLiteral("v0.1.0"), QStringLiteral("0.2.0")),
                "older release is not newer");
    // Garbage must not read as an update (avoid false nags).
    check::that(!version::isNewer(QStringLiteral("not-a-version"), QStringLiteral("0.1.0")),
                "garbage tag is not newer");
}

} // namespace

void runVersionTests()
{
    testOrdering();
    testTolerantParsing();
    testPreRelease();
    testIsNewer();
}
