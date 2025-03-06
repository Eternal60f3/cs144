#pragma once

#include <stdexcept>
#include <iostream>
#include <string>

inline std::string GetFileName(std::string file_path) {
    std::string file_name;
    int pos;
    if ((pos = file_path.rfind("/")) != static_cast<int>(std::string::npos) or
        (pos = file_path.rfind(R"(\)")) != static_cast<int>(std::string::npos)) {
        file_name = file_path.substr(pos + 1);
    } else {
        file_name = file_path;
    }
    return file_name;
}


#define ASSERT_D(condition, message)                                                                                  \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            std::cerr << "Assertion failed: " << message << " (" << GetFileName( __FILE__ ) << ":" << __LINE__ << ")" << std::endl;   \
            throw std::runtime_error("assertion error");                                                                                              \
        }                                                                                                              \
    } while (0)
