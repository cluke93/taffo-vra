#include <cmath>
#include <limits>
#include <algorithm>

#include "RangeHandler.hpp"

Range RangeHandler::Merge(Range r1, Range r2) {
    return Range(r1.min < r2.min ? r1.min : r2.min, r1.max > r2.max ? r1.max : r2.max);
}

Range RangeHandler::Add(Range r1, Range r2, int minIter, int maxIter) {
    return Range(r1.min + minIter * r2.min, r1.max + maxIter * r2.max);
}

Range RangeHandler::Sub(Range r1, Range r2, int minIter, int maxIter) {
    return Range(r1.min - maxIter * r2.max, r1.max - minIter * r2.min);
}

Range RangeHandler::Mul(Range r1, Range r2) {
    float a = r1.min, b = r1.max;
    float c = r2.min, d = r2.max;
    
    return Range (
        std::min({a * c, a * d, b * c, b * d}),
        std::max({a * c, a * d, b * c, b * d})
    );
}

Range RangeHandler::MulOnLoop(Range r1, Range r2, int minIter, int maxIter) {
    // Helper: elevamento a potenza di un interval
    auto powInterval = [&](const Range &r, int exp) {
        // exp >= 0
        if (exp == 0) return Range{1.0f, 1.0f};
        
        float v1 = static_cast<float>(std::pow(r.min, exp));
        float v2 = static_cast<float>(std::pow(r.max, exp));
        float lo, hi;
        // Se l'intervallo attraversa zero e l'esponente è pari
        if (r.min < 0.0f && r.max > 0.0f && (exp % 2 == 0)) {
            lo = 0.0f;            
            hi = std::max(v1, v2);
        } else {
            lo = std::min(v1, v2);
            hi = std::max(v1, v2);
        }
        return Range{lo, hi};
    };

    // campionamento delle quattro iterazioni principali
    std::vector<int> iters;
    iters.push_back(minIter);
    if (minIter + 1 <= maxIter)       iters.push_back(minIter + 1);
    if (maxIter - 1 >= minIter)       iters.push_back(maxIter - 1);
    if (maxIter != minIter)           iters.push_back(maxIter);

    float globalMin = std::numeric_limits<float>::infinity();
    float globalMax = -std::numeric_limits<float>::infinity();

    for (int i : iters) {
        // calcola r2^i
        Range kpow = powInterval(r2, i);
        // applica a = a0 * (k^i)
        Range ri = Mul(r1, kpow);
        globalMin = std::min(globalMin, ri.min);
        globalMax = std::max(globalMax, ri.max);
    }

    return Range{globalMin, globalMax};
}

Range RangeHandler::Div(Range r1, Range r2) {
    
    if (r2.min <= 0 && r2.max >= 0) {
        errs() << "  [VRA] Divisione per zero potenziale, salto\n";
        return Range();
    } else {
        float a = r1.min, b = r1.max;
        float c = r2.min, d = r2.max;
        return Range(
            std::min({a / c, a / d, b / c, b / d}),
            std::max({a / c, a / d, b / c, b / d})
        );
    }
}