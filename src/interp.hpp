#pragma once

#include "expr.hpp"
#include <string_view>
#include <string>
#include <optional>

struct Instruction {
    std::string assign_to; //blank if this is an output instruction
    std::unique_ptr<Expr> expr;
};

std::optional<Instruction> interpret_expression(std::string_view expr_str);
std::string get_error_text();