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
std::string get_error_text();

void clear_variables();
bool clear_variable(const char* id);
bool set_variable(const char* id, std::unique_ptr<Expr> expr);
Expr* get_variable(const char* id);
bool output_expression(Expr* expr);
std::unique_ptr<Expr> reduce_expression(std::unique_ptr<Expr> expr);