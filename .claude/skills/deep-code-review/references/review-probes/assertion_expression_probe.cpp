#include <SQLiteCpp/Assertion.h>
#include <cassert>
#include <iostream>
static int handlerCalls = 0;
namespace SQLite
{
void assertion_failed(const char *, const int, const char *, const char *, const char *)
{
    ++handlerCalls;
}
} // namespace SQLite
int main()
{
    bool outerElse = false;
    if (false)
        SQLITECPP_ASSERT(true, "probe");
    else
        outerElse = true;
    int actual = 0;
    SQLITECPP_ASSERT(actual = 7, "assignment must keep its value");
    int standard = 0;
    assert(standard = 7);
    int expressionCalls = 0;
    SQLITECPP_ASSERT(++expressionCalls, "single evaluation");
    int messageCalls = 0;
    SQLITECPP_ASSERT(true, (++messageCalls, "message"));
#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER
    SQLITECPP_ASSERT(false, (++messageCalls, "failure message"));
#endif
    std::cout << "outerElse=" << outerElse << " actualAssignment=" << actual << " standardAssignment=" << standard
              << " expressionCalls=" << expressionCalls << " messageCalls=" << messageCalls
              << " handlerCalls=" << handlerCalls << '\n';
}
