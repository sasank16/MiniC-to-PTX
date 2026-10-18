#include "tests/test_framework.hpp"

int main() {
    return minic::test::TestRegistry::instance().runAll();
}
