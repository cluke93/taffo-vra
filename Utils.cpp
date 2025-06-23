#include "Utils.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

std::string Utils::formatFloatSmart(float val) {
    std::ostringstream oss;

    if (std::abs(val) < 1e6 && std::abs(val) > 1e-4) {
        if (std::floor(val) == val) {
            oss << static_cast<int>(val);
        } else if (std::abs(val) < 1.0f) {
            oss << std::fixed << std::setprecision(4) << val;
        } else {
            oss << std::fixed << std::setprecision(2) << val;
        }
    } else {
        oss << std::scientific << std::setprecision(2) << val;
    }

    return oss.str();
}
