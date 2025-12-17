#include "expr.hpp"
#include "interp.hpp"
#include <iostream>
#include <string>

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
    std::string line; 
    while (std::getline(std::cin, line))
    {
        if (line.empty()) break;
        output(line.c_str());
    }
}