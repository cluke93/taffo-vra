#ifndef RANGE_HANDLER_H
#define RANGE_HANDLER_H

#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include <algorithm>
#include <set>

using namespace llvm;

static constexpr float NEG_INF = -std::numeric_limits<float>::infinity();
static constexpr float POS_INF =  std::numeric_limits<float>::infinity();

struct Range {
    float min;
    float max;
    bool isFixed = false;

    /**
     * Constructor that ensure always smallest value as min and greater as max
     */
    Range(float minVal = 0.0f, float maxVal = 0.0f, bool fixed = false) : isFixed(fixed) {
        if (minVal <= maxVal) {
            min = minVal;
            max = maxVal;
        } else {
            max = minVal;
            min = maxVal;
        }
    }

    /**
     * Enlarge the range after an operation if necessary.
     */
    bool tryRangeEnlarging(Range r2) {
        if (isFixed) return false;

        if (r2.min < min) min = r2.min;
        if (r2.max > max) max = r2.max;

        return true;
    }
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
    static Range Add(Range r1, Range r2, int minIter, int maxIter);

    /**
     * Create new merge as result of diff between two ranges done inside loop
     */
    static Range Sub(Range r1, Range r2, int minIter, int maxIter);

    /**
     * Create new merge as result of multiplication between two ranges
     */
    static Range Mul(Range r1, Range r2);

    static Range MulOnLoop(Range r1, Range r2, int minIter, int maxIter);

    /**
     * Create new merge as result of division between two ranges
     */
    static Range Div(Range r1, Range r2);

};


#endif