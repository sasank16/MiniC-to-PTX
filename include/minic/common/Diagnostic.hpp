#pragma once

#include "minic/common/SourceLocation.hpp"
#include <string>
#include <vector>
#include <iostream>

namespace minic {

enum class DiagnosticLevel {
    Note,
    Warning,
    Error
};

struct Diagnostic {
    DiagnosticLevel level;
    SourceLocation location;
    std::string message;
};

class DiagnosticEngine {
public:
    explicit DiagnosticEngine(std::string sourceCode = "", std::string filename = "")
        : sourceCode_(std::move(sourceCode)), filename_(std::move(filename)) {}

    void setSource(std::string sourceCode, std::string filename = "") {
        sourceCode_ = std::move(sourceCode);
        filename_ = std::move(filename);
    }

    void report(DiagnosticLevel level, const SourceLocation& loc, const std::string& msg);
    void error(const SourceLocation& loc, const std::string& msg);
    void warning(const SourceLocation& loc, const std::string& msg);
    void note(const SourceLocation& loc, const std::string& msg);

    bool hasErrors() const { return errorCount_ > 0; }
    size_t errorCount() const { return errorCount_; }
    size_t warningCount() const { return warningCount_; }
    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }

    void printAll(std::ostream& os = std::cerr) const;
    void clear();

private:
    std::string getLineSnippet(int line) const;

    std::string sourceCode_;
    std::string filename_;
    std::vector<Diagnostic> diagnostics_;
    size_t errorCount_{0};
    size_t warningCount_{0};
};

} // namespace minic
