#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

namespace minic::test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(std::string name, std::function<void()> func) {
        tests_.push_back({std::move(name), std::move(func)});
    }

    int runAll() {
        int passed = 0;
        int failed = 0;
        std::cout << "[==========] Running " << tests_.size() << " tests.\n";

        for (const auto& test : tests_) {
            std::cout << "[ RUN      ] " << test.name << "\n";
            try {
                test.func();
                std::cout << "[       OK ] " << test.name << "\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.name << "\n";
                std::cout << "             Exception: " << e.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "[  FAILED  ] " << test.name << "\n";
                std::cout << "             Unknown exception caught.\n";
                failed++;
            }
        }

        std::cout << "[==========] " << tests_.size() << " tests ran.\n";
        std::cout << "[  PASSED  ] " << passed << " tests.\n";
        if (failed > 0) {
            std::cout << "[  FAILED  ] " << failed << " tests.\n";
            return 1;
        }
        return 0;
    }

private:
    std::vector<TestCase> tests_;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRegistry::instance().addTest(name, std::move(func));
    }
};

class AssertionFailure : public std::runtime_error {
public:
    explicit AssertionFailure(const std::string& msg) : std::runtime_error(msg) {}
};

inline void assertTrue(bool cond, const char* condStr, const char* file, int line, const std::string& msg = "") {
    if (!cond) {
        std::ostringstream oss;
        oss << file << ":" << line << ": Assertion failed: (" << condStr << ")";
        if (!msg.empty()) {
            oss << " - " << msg;
        }
        throw AssertionFailure(oss.str());
    }
}

template <typename T, typename U>
inline void assertEqual(const T& a, const U& b, const char* aStr, const char* bStr, const char* file, int line) {
    if (!(a == b)) {
        std::ostringstream oss;
        oss << file << ":" << line << ": Equality assertion failed: (" << aStr << " == " << bStr << ") evaluated to (" << a << " == " << b << ")";
        throw AssertionFailure(oss.str());
    }
}

} // namespace minic::test

#define TEST(suite, name) \
    static void suite##_##name##_impl(); \
    static ::minic::test::TestRegistrar suite##_##name##_reg(#suite "." #name, suite##_##name##_impl); \
    static void suite##_##name##_impl()

#define EXPECT_TRUE(cond) ::minic::test::assertTrue((cond), #cond, __FILE__, __LINE__)
#define EXPECT_FALSE(cond) ::minic::test::assertTrue(!(cond), "!(" #cond ")", __FILE__, __LINE__)
#define EXPECT_EQ(a, b) ::minic::test::assertEqual((a), (b), #a, #b, __FILE__, __LINE__)
#define ASSERT_TRUE(cond) EXPECT_TRUE(cond)
#define ASSERT_EQ(a, b) EXPECT_EQ(a, b)
