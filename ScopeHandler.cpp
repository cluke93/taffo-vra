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

void Scope::addOperand(std::unique_ptr<Operand> op) {
    operands.push_back(std::move(op));
}

Operand* Scope::lookup(const std::string& name) {
    for (const auto& op : operands) {
        if (op->name == name)
            return op.get();
    }
    if (parent)
        return parent->lookup(name);
    return nullptr;
}

std::vector<Operand*> Scope::getOperands() {
    std::vector<Operand*> result;
    for (auto& v : operands) {
        result.push_back(v.get());
    }
    return result;
}

//TODO: da controllare
void Scope::mergeWith(Scope* otherScope) {
    if (!otherScope) return;

    // Prendiamo tutte le Var* dichiarate (solo locali)
    // auto otherOperands = otherScope->getOperands();
    // for (Operand* v : otherOperands) {
    //     // Cerco in this->variables
    //     auto it = std::find_if(
    //         operands.begin(), operands.end(),
    //         [&](const std::unique_ptr<Operand>& varPtr){
    //             return varPtr->name == v->name;
    //         }
    //     );

    //     if (it != operands.end()) {
    //         // existing var: check if enlargin is needed

    //         Operand* existing = it->get();
    //         if (!existing->getRange()->isFixed) {
    //             existing->getRange()->min = std::min(existing->getRange()->min, v->getRange()->min);
    //             existing->getRange()->max = std::max(existing->getRange()->max, v->getRange()->max);
    //         }
    //     } else {
    //         // new var: add it

    //         operands.emplace_back(std::make_unique<Operand>(v->name, v->range, VarType::Local));
    //     }
    // }
}

void Scope::printJson() const {
    errs() << toJson() << "\n";
}

bool Operand::tryResolution() {
    if (isResolvable()) return true;

    std::vector<Range> args;
    for (Operand* dep : dependencies) {
        if (!dep->tryResolution())
            return false;            // se anche una sola fallisce, non risolvo
        args.push_back(*dep->getRange());
    }

    // Chiamo la funzione simbolica
    Range result = call(args);

    // Alloco il risultato nello heap e lo assegno al unique_ptr
    range = std::make_unique<Range>(result);

    return true;
}

void Operand::forceResolution() {
    if (isResolvable()) return;

    std::vector<Range> args;
    for (Operand* dep : dependencies) {
        dep->forceResolution();
        args.push_back(*dep->getRange());
    }

    // Chiamo la funzione simbolica
    Range result = call(args);

    // Alloco il risultato nello heap e lo assegno al unique_ptr
    range = std::make_unique<Range>(result);
}

void Operand::addDepencendy(Operand* op) {
    dependencies.push_back(op);
}