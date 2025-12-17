#include "expr.hpp"
#include "interp.hpp"
#include <iostream>

void output(const char* s) {
    auto instr = interpret_expression(s);
    if (!instr) {
        std::cout << "ERROR: " << get_error_text() << std::endl;
        return;
    }

    std::cout << instr->expr->to_string();
    if (!instr->assign_to.empty())
        std::cout << " [ASSIGNS TO '" << instr->assign_to << "']";
    std::cout << std::endl;
}

int main() {
    output("\\x.\\x.\\x.\\x.\\x.\\x.\\x.\\x.\\x.y");
}