#include "minic/common/Diagnostic.hpp"
#include <iomanip>
#include <sstream>

namespace minic {

void DiagnosticEngine::report(DiagnosticLevel level, const SourceLocation& loc, const std::string& msg) {
    diagnostics_.push_back({level, loc, msg});
    if (level == DiagnosticLevel::Error) {
        errorCount_++;
    } else if (level == DiagnosticLevel::Warning) {
        warningCount_++;
    }
}

void DiagnosticEngine::error(const SourceLocation& loc, const std::string& msg) {
    report(DiagnosticLevel::Error, loc, msg);
}

void DiagnosticEngine::warning(const SourceLocation& loc, const std::string& msg) {
    report(DiagnosticLevel::Warning, loc, msg);
}

void DiagnosticEngine::note(const SourceLocation& loc, const std::string& msg) {
    report(DiagnosticLevel::Note, loc, msg);
}

std::string DiagnosticEngine::getLineSnippet(int targetLine) const {
    if (sourceCode_.empty() || targetLine < 1) {
        return "";
    }
    std::istringstream stream(sourceCode_);
    std::string line;
    int currentLine = 1;
    while (std::getline(stream, line)) {
        if (currentLine == targetLine) {
            // Strip any trailing \r
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        currentLine++;
    }
    return "";
}

void DiagnosticEngine::printAll(std::ostream& os) const {
    for (const auto& diag : diagnostics_) {
        // Location string
        std::string locStr = diag.location.toString();
        if (locStr.empty()) {
            locStr = filename_.empty() ? "<input>" : filename_;
        }

        const char* levelStr = "unknown";
        switch (diag.level) {
            case DiagnosticLevel::Error:   levelStr = "error"; break;
            case DiagnosticLevel::Warning: levelStr = "warning"; break;
            case DiagnosticLevel::Note:    levelStr = "note"; break;
        }

        os << locStr << ": " << levelStr << ": " << diag.message << "\n";

        // If we have line content, print source snippet and caret
        std::string lineSnippet = getLineSnippet(diag.location.line);
        if (!lineSnippet.empty()) {
            std::string lineNumStr = std::to_string(diag.location.line);
            std::string padding(lineNumStr.length(), ' ');

            os << " " << lineNumStr << " | " << lineSnippet << "\n";
            os << " " << padding    << " | ";

            int col = std::max(1, diag.location.column);
            for (int i = 1; i < col; ++i) {
                if (i <= static_cast<int>(lineSnippet.size()) && lineSnippet[i - 1] == '\t') {
                    os << "\t";
                } else {
                    os << " ";
                }
            }
            os << "^\n";
        }
    }
}

void DiagnosticEngine::clear() {
    diagnostics_.clear();
    errorCount_ = 0;
    warningCount_ = 0;
}

} // namespace minic
