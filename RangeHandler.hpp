#ifndef RANGE_HANDLER_H
#define RANGE_HANDLER_H

#include <cmath>
#include <limits>
#include <algorithm>
#include <set>

#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

using namespace llvm;



struct Range {
    float min;
    float max;
    bool isFixed = false;

    static constexpr float INF  = std::numeric_limits<float>::infinity();
    static constexpr float NEG_INF = -std::numeric_limits<float>::infinity();

    static const Range BOTTOM;
    static const Range TOP; 

    bool isBottom() const { 
        return min == INF && max == NEG_INF; 
    }
    bool isTop() const { 
        return min == NEG_INF && max == INF;
    }
    bool isEmpty() const {
        return isBottom();  // min > max
    }
    bool containsInfinity() const {
        return std::isinf(min) || std::isinf(max);
    }

    Range() : min(+std::numeric_limits<float>::infinity()), max(-std::numeric_limits<float>::infinity()) {}

    Range (float min, float max): min(min), max(max) {}

    Range (float min, float max, bool isFixed): min(min), max(max), isFixed(isFixed) {}

};

class RangeHandler {

    public:

    /**
     * Create a new range merging two ranges, no matter fixed property
     */
    static Range Merge(const Range& a, const Range& b);

    /**
     * Create new merge as result of sum between two ranges done inside loop
     */
    static Range Add(Range r1, Range r2);

    /**
     * Create new merge as result of diff between two ranges done inside loop
     */
    static Range Sub(Range r1, Range r2);

    /**
     * Create new merge as result of multiplication between two ranges
     */
    static Range Mul(Range r1, Range r2);

    /**
     * Create new merge as result of division between two ranges
     */
    static Range Div(Range r1, Range r2);

};


#endif