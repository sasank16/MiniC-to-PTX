#pragma once

#include <string>
#include <sstream>

namespace minic {

struct SourceLocation {
    std::string filename;
    int line{1};
    int column{1};

    SourceLocation() = default;
    SourceLocation(std::string file, int l, int col)
        : filename(std::move(file)), line(l), column(col) {}

    std::string toString() const {
        std::ostringstream oss;
        if (!filename.empty()) {
            oss << filename << ":";
        }
        oss << line << ":" << column;
        return oss.str();
    }
};

} // namespace minic
