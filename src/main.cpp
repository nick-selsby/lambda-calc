#include "expr.hpp"
#include "interp.hpp"
#include <cstdio>
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
    for (int i = 0; i < 10; i++) {
        char name[32];
        sprintf(name, "%d", i);
        Expr e = Expr::fn("x", Expr::var("x"));
        for (int j = 0; j < i; j++) {
            e = Expr::fn("x", e);
        }
        set_variable(name, e);
    }

    std::string line; 
    while (true)
    {
        printf(">");
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) break;
        run_and_output(line.c_str());
    }
}