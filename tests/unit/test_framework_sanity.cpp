#include "tests/test_framework.hpp"
#include "minic/common/Diagnostic.hpp"

TEST(SanityTest, BasicAssertions) {
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST(DiagnosticTest, ReportAndCount) {
    minic::DiagnosticEngine diag("int x = 10;", "test.mc");
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 0u);

    diag.error({ "test.mc", 1, 5 }, "Syntax error");
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1u);
}

int main() {
    return minic::test::TestRegistry::instance().runAll();
}
