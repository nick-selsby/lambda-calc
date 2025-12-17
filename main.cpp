#include "expr.hpp"
#include <iostream>
#include <vector>

int main() {
    std::vector<std::unique_ptr<Expr>> v;

    auto expr = Expr::fn("x", Expr::fn("x", Expr::fn("x", Expr::var("x"))));

    std::cout << expr->to_string() << std::endl;
}

/*
^x.x
x:=
x x
*/