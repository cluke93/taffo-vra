
#include "ScopeHandler.hpp"
#include "llvm/Support/raw_ostream.h"  // errs()

using namespace llvm;

void Scope::addVar(const std::string& name, const Range& range, VarType type) {
    auto var = std::make_unique<Var>(name, range, type);
    Variables.emplace(name, std::move(var));
}

Var* Scope::lookup(const std::string& name) const {
    auto it = Variables.find(name);
    if (it != Variables.end())
        return it->second.get();
    if (ParentScope)
        return ParentScope->lookup(name);
    return nullptr;
}

void Scope::dump() const {
    for (auto const& p : Variables) {
        const Var* v = p.second.get();
        errs() << "Var '" << p.first << "': ["
               << v->range.min << ", " << v->range.max << "]\n";
    }
}