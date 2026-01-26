#pragma once

#include "expr.hpp"
#include <memory>
#include <string_view>
#include <string>
#include <optional>

struct Instruction {
    std::string assign_to; //blank if this is an output instruction
    std::unique_ptr<Expr> expr;
};

std::optional<Instruction> interpret_expression(std::string_view expr_str);
std::optional<Expr> parse_expression(std::string_view expr_str);
std::string get_error_text();

void clear_variables();
bool clear_variable(const char* id);
bool set_variable(const char* id, const Expr& expr);
bool set_variable(const char* id, const char* raw_expr);
Expr* get_variable(const char* id);

Expr* reduce_expression(Expr* expr);
//Expr* apply_expression(Expr* expr, Expr* value);