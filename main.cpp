#include "expr.hpp"
#include <iostream>

int main() {
    auto expr = Expr::var("x");

    for (int i = 0; i < 100; i++) {
        expr = Expr::fn("x", std::move(expr));
    }

    std::cout << expr->to_string() << std::endl;
}

/*
^x.x
x:=
x x
*/