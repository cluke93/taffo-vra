#ifndef SCOPE_HANDLER_H
#define SCOPE_HANDLER_H

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "RangeHandler.hpp"  // per Range

using namespace llvm;

enum class VarType { Local, Argument };

/**
 * @brief Rappresenta una variabile con nome, intervallo di valori e tipo
 */
class Var {
public:
    std::string name;  ///< Nome della variabile
    Range range;       ///< Intervallo di valori della variabile
    VarType type;      ///< Tipo: locale o argomento

    /**
     * @brief Costruisce una variabile
     * @param name Nome
     * @param range Intervallo iniziale
     * @param type Tipo della variabile
     */
    Var(std::string name, const Range& range, VarType type)
        : name(std::move(name)), range(range), type(type) {}
};

/**
 * @brief Scope gerarchico per variabili con parent chaining
 */
class Scope {
public:
    /**
     * @brief Costruttore
     * @param parent Parent scope (nullptr per global)
     */
    explicit Scope(Scope* parent = nullptr) : ParentScope(parent) {}

    /**
     * @brief Inserisce una variabile con nome e range
     * @param name  Nome della variabile
     * @param range Range iniziale
     */
    void addVar(const std::string& name, const Range& range, VarType type = VarType::Local);

    /**
     * @brief Look up di una variabile, ricerca ricorsiva sul parent
     * @param name Nome della variabile
     * @return Puntatore a Var o nullptr
     */
    Var* lookup(const std::string& name) const;

    /**
     * @brief Stampa su errs() tutte le variabili nello scope corrente
     */
    void dump() const;

private:
    Scope* ParentScope;
    std::unordered_map<std::string, std::unique_ptr<Var>> Variables;
};

#endif