#include "Check.h"

int main()
{
    runJsonToolsTests();
    runYamlToolsTests();
    runVersionTests();

    std::fprintf(stderr, "%d checks, %d failures\n", check::g_checks, check::g_failures);
    return check::g_failures == 0 ? 0 : 1;
}
