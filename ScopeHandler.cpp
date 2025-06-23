#include "ScopeHandler.hpp"
#include <algorithm>

std::string Scope::formatFloatSmart(float val) const {
    std::ostringstream oss;

    // Evita notazione scientifica per valori "normali"
    if (std::abs(val) < 1e6 && std::abs(val) > 1e-4) {
        if (std::floor(val) == val) {
            // Valore intero, stampa come intero
            oss << static_cast<int>(val);
        } else if (std::abs(val) < 1.0f) {
            // Valore piccolo, usa 4 decimali
            oss << std::fixed << std::setprecision(4) << val;
        } else {
            // Valore normale, usa 2 decimali
            oss << std::fixed << std::setprecision(2) << val;
        }
    } else {
        // Valore molto grande o molto piccolo: usa notazione scientifica
        oss << std::scientific << std::setprecision(2) << val;
    }

    return oss.str();
}

void Scope::addVar(const std::string& name, Range range) {
    variables.emplace_back(std::make_unique<Var>(name, range));
}

void Scope::addVar(std::unique_ptr<Var> var) {
    if (!var) return;
    
    for (auto& existingPtr : variables) {
        Var& existing = *existingPtr;
        if (existing.name == var->name) {
            
            if (!existing.range.isFixed) {
                enlargeVarRange(existing, var->range);
            }
            return;
        }
    }
    variables.emplace_back(std::move(var));
}

Var* Scope::lookup(const std::string& name) {
    for (const auto& var : variables) {
        if (var->name == name)
            return var.get();
    }
    if (parent)
        return parent->lookup(name);
    return nullptr;
}

std::vector<Var*> Scope::getVariables() {
    std::vector<Var*> result;
    for (auto& v : variables) {
        result.push_back(v.get());
    }
    return result;
}

void Scope::mergeWith(Scope* otherScope) {
    if (!otherScope) return;

    // Prendiamo tutte le Var* dichiarate (solo locali)
    auto otherVars = otherScope->getVariables();
    for (Var* v : otherVars) {
        // Cerco in this->variables
        auto it = std::find_if(
            variables.begin(), variables.end(),
            [&](const std::unique_ptr<Var>& varPtr){
                return varPtr->name == v->name;
            }
        );

        if (it != variables.end()) {
            // existing var: check if enlargin is needed

            Var* existing = it->get();
            if (!existing->range.isFixed) {
                existing->range.min = std::min(existing->range.min, v->range.min);
                existing->range.max = std::max(existing->range.max, v->range.max);
            }
        } else {
            // new var: add it

            variables.emplace_back(std::make_unique<Var>(v->name, v->range));
        }
    }
}

void Scope::enlargeVarRange(Var& var, const Range& r) {
    if (var.range.min > r.min) var.range.min = r.min;
    if (var.range.max < r.max) var.range.max = r.max;
}

void Scope::prettyPrint(int depth) const {
    // indentazione con due spazi per livello
    std::string indent(depth * 2, ' ');
    errs() << indent << "Scope (level " << depth << ") @" << this
           << " — vars: " << variables.size() << "\n";

    for (auto const& vp : variables) {
        const Var& v = *vp;
        errs() << indent << "  • " << v.name
               << " [" << formatFloatSmart(v.range.min)
               << ", "  << formatFloatSmart(v.range.max) << "]";
        if (v.range.isFixed) errs() << " (fixed)";
        errs() << "\n";
    }

    if (parent) {
        errs() << indent << "  ↑ parent:\n";
        parent->prettyPrint(depth + 1);
    }
}

std::string Scope::toJson() const {
    std::ostringstream oss;
    oss << "{";
    // variabili
    oss << "\"vars\":[";
    for (size_t i = 0; i < variables.size(); ++i) {
        const Var& v = *variables[i];
        oss << "{"
            << "\"name\":\""  << v.name << "\","
            << "\"min\":"    << v.range.min << ","
            << "\"max\":"    << v.range.max << ","
            << "\"fixed\":"  << (v.range.isFixed ? "true" : "false")
            << "}";
        if (i + 1 < variables.size()) oss << ",";
    }
    oss << "]";

    // parent
    if (parent) {
        oss << ",\"parent\":" << parent->toJson();
    } else {
        oss << ",\"parent\":null";
    }

    oss << "}";
    return oss.str();
}

void Scope::printJson() const {
    errs() << toJson() << "\n";
}