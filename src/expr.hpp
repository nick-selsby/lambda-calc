#pragma once

#include <string_view>
#include <sstream>

enum class ExprType {
    Empty, //nothing allocated
    Var,
    Fn,
    App
};

struct Expr {
    Expr(); // does not initialize anything!
    static Expr var(char id);
    static Expr var(std::string_view id);
    static Expr fn(char arg, const Expr& body);
    static Expr fn(std::string_view arg, const Expr& body);
    static Expr app(const Expr& lhs, const Expr& rhs);
    Expr(const Expr& e);
    Expr(Expr&& e);
    Expr& operator=(const Expr& e);
    Expr& operator=(Expr&& e);
    ~Expr();
    
    Expr* clone() const;
    ExprType get_type() const;
    std::string to_string() const;

    ExprType _type;
    union {
        const char* _var;
        struct { const char* id; Expr* body; } _fn;
        struct { Expr* lhs; Expr* rhs; } _app;
    };

private:
    void _output(std::stringstream& ss) const;
    void _internal_swap(Expr* inner);
};