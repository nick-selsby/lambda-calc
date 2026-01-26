#include "expr.hpp"
#include "interp.hpp"
#include <iostream>
#include <string>

void run_and_output(const char* s) {
    auto instr = interpret_expression(s);
    if (!instr) {
        std::cout << "ERROR: " << get_error_text() << std::endl;
        return;
    }
    
    std::cout << instr->expr->to_string();
    if (!instr->assign_to.empty())
        std::cout << " [ASSIGNS TO '" << instr->assign_to << "']";
    std::cout << std::endl;

    if (!instr->assign_to.empty()) {
        bool success = set_variable(instr->assign_to.c_str(), *instr->expr);
        if (!success) {
            std::cout << "ERROR: " << get_error_text() << std::endl;
        }
    } else {
        Expr* reduced = reduce_expression(instr->expr.get());
        if (!reduced) {
            std::cout << "ERROR: " << get_error_text() << std::endl;
        } else {
            std::cout << "REDUCED: " << reduced->to_string() << std::endl;
        }
        delete reduced;
    }
}

int main() {
    std::string line; 
    while (true)
    {
        printf(">");
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) break;
        run_and_output(line.c_str());
    }
}