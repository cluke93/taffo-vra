#include <cmath>
#include <limits>
#include <algorithm>

#include "RangeHandler.hpp"

constexpr float Range::INF;
constexpr float Range::NEG_INF;
const Range Range::BOTTOM = { Range::INF,  Range::NEG_INF };
const Range Range::TOP    = { Range::NEG_INF, Range::INF  };

Range RangeHandler::Merge(const Range& a, const Range& b) {
    
    // if one of them is bottom return the other
    if (a.isBottom()) return b;
    if (b.isBottom()) return a;
    
    return Range {
        std::min(a.min, b.min),
        std::max(a.max, b.max)
    };
}

Range RangeHandler::Add(Range r1, Range r2) {
    if (r1.isBottom() || r2.isBottom()) return Range::BOTTOM;

    float min = r1.min + r2.min;
    float max = r1.max + r2.max;
    
    return Range{ min, max };
}

Range RangeHandler::Sub(Range r1, Range r2) {
    return Range::TOP;
}

Range RangeHandler::Mul(Range r1, Range r2) {
    return Range::TOP;
}

Range RangeHandler::Div(Range r1, Range r2) {
    return Range::TOP;
}