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

struct Const {
    std::string name;
    Range range;

    Const(const std::string& name, float val) : name(name) {
        range = Range(val, val, true);
    }
};

struct Var {
    std::string name;
    Range range;

    Var(const std::string& name, Range range, bool isFixed = false) : name(name), range(range) { }
};

struct InputVal {
    std::string name;
    Range range;

    InputVal(const std::string& name, std::optional<std::pair<float, float>> customRange = std::nullopt, bool isFixed = false) : name(name),
            range(customRange.has_value() ? 
                Range(customRange->first, customRange->second) : 
                Range(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), isFixed)) {}

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
    void addVar(const std::string& name, Range range);

    /// @brief Add a new var if not exists or update if (checking if range should be expanded)
    /// @param var object to add, already instantiated
    void addVar(std::unique_ptr<Var> var);

    /// @brief get the lists of all variable in the scope, without consider parent scope
    /// @return all variable in this level of scope
    std::vector<Var*> getVariables();

    // template<size_t N>
    // void addFixVector(const std::string& name, std::array<float, N> vals) {
    //     variables.emplace_back(std::make_unique<FixVector>(name, vals));
    // }

    /// @brief Search a variable recursively on parents in bottom-up
    /// @param name symbol to search
    /// @return object if found, nullptr otherwise
    Var* lookup(const std::string& name);

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

    void enlargeVarRange(Var& var, const Range& r);

private:

    /**
     * Scope parent for lookups
     */
    Scope* parent;

    /**
     * List of variables in this scope
     */
    std::vector<std::unique_ptr<Var>> variables;

};

#endif