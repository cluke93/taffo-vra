#ifndef SCOPE_HANDLER_H
#define SCOPE_HANDLER_H

#include <iostream>
#include <iomanip>
#include <stack>
#include <sstream>
#include <cmath>
#include <string>
#include <vector>
#include <memory>

#include "Utils.hpp"
#include "RangeHandler.hpp"

using namespace llvm;

enum class VarType { Local, Argument, Constant, ArgumentRef, Return };


struct Operand {

    /// @brief name of this operand
    std::string name;
    
    /// @brief current solved range
    std::unique_ptr<Range> range;

    /// @brief pointers to dependences, pair in case of binary op
    std::vector<Operand*> dependencies;

    /// @brief function to call as soon as all dependences are resolved
    std::function<Range(const std::vector<Range>&)> call;
    
    /// @brief if the range has been already solved, this is the initial range to to it. If a new input range expand this, then new computation must be done.
    std::unique_ptr<Range> resolvedWith;

    VarType type;

    Operand(const std::string& name, std::vector<Operand*> dependencies, std::function<Range(const std::vector<Range>&)> callFn, VarType type) : 
        name(name), type(type), call(std::move(callFn)), dependencies(dependencies), resolvedWith(nullptr) {  }

    /**
     * @brief Constructor for concrete operands with a known initial range
     * @param name         Name of the operand
     * @param initialRange Known range (by value)
     * @param type         Variable type (Local or Argument)
     */
    Operand(const std::string& name, const Range& initialRange, VarType type) : name(name), range(std::make_unique<Range>(initialRange)),
        call(nullptr), resolvedWith(std::make_unique<Range>(initialRange)), type(type) {}

    bool isResolvable() {
        return range != nullptr;
    }

    Range* getRange() {
        if (!range) return new Range(NEG_INF, POS_INF);
        return range.get();
    }

    void addDepencendy(Operand* op);

    bool tryResolution();

    void forceResolution();

    /// Deep copy constructor
    Operand(const Operand& other)
      : name(other.name),
        range(other.range ? std::make_unique<Range>(*other.range) : nullptr),
        dependencies(other.dependencies), call(other.call), 
        resolvedWith(other.resolvedWith ? std::make_unique<Range>(*other.resolvedWith) : nullptr),
        type(other.type) {}

    /// Ritorna un unique_ptr a un clone deep di *this
    std::unique_ptr<Operand> clone() const {
        return std::make_unique<Operand>(*this);
    }
};


template <size_t N>
struct FixVector {
    std::string name;
    std::array<float, N> val;
    Range range;

    FixVector(const std::string& name, const std::array<float, N>& val)
        : name(name), val(val) {
        float min = *std::min_element(val.begin(), val.end());
        float max = *std::max_element(val.begin(), val.end());
        range = Range(min, max);
    }
};

template <size_t N>
struct InputFixVector {
    std::string name;
    std::array<float, N> val;
    Range range;

    InputFixVector(const std::string& name,
                   const std::array<float, N>& val,
                   std::optional<std::pair<float, float>> customRange = std::nullopt)
        : name(name), val(val) {
        if (customRange) {
            range = Range(customRange->first, customRange->second);
        } else {
            float min = *std::min_element(val.begin(), val.end());
            float max = *std::max_element(val.begin(), val.end());
            range = Range(min, max);
        }
    }
};


class Scope {

public:

    std::string formatFloatSmart(float val) const;

    /// @brief Create and append a new var (you have to ensure this is never created before)
    /// @param name name of this var
    /// @param range range of this var
    void addOperand(std::unique_ptr<Operand> op);

    /// @brief get the lists of all variable in the scope, without consider parent scope
    /// @return all variable in this level of scope
    std::vector<Operand*> getOperands();

    // template<size_t N>
    // void addFixVector(const std::string& name, std::array<float, N> vals) {
    //     variables.emplace_back(std::make_unique<FixVector>(name, vals));
    // }

    /// @brief Search a variable recursively on parents in bottom-up
    /// @param name symbol to search
    /// @return object if found, nullptr otherwise
    Operand* lookup(const std::string& name);

    /**
     * Merge this scope by adding new variables and enlarging range of existings
     */
    void mergeWith(Scope* otherScope);

    /// Stampa in console la struttura dello scope in modo ordinato
    void prettyPrint(int depth = 0) const;

    /// Ritorna una stringa JSON che descrive questo scope (e ricorsivamente i genitori)
    std::string toJson() const;

    /// Stampa in console la rappresentazione JSON di questo scope
    void printJson() const;

    Scope(Scope* parent): parent(parent) {}

protected:

    //void enlargeVarRange(Operand& var, const Range& r);

private:

    /**
     * Scope parent for lookups
     */
    Scope* parent;

    /**
     * List of variables in this scope
     */
    std::vector<std::unique_ptr<Operand>> operands;

};

#endif