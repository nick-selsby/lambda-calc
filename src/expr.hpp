#pragma once

#include <memory>
#include <string_view>
#include <sstream>

enum class ExprType {
    Empty, //nothing allocated
    Var,
    Fn,
    App
};

struct Expr {
    static std::unique_ptr<Expr> var(char id);
    static std::unique_ptr<Expr> var(std::string_view id);
    static std::unique_ptr<Expr> fn(char arg, std::unique_ptr<Expr> body);
    static std::unique_ptr<Expr> fn(std::string_view arg, std::unique_ptr<Expr> body);
    static std::unique_ptr<Expr> app(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs);
    std::unique_ptr<Expr> clone() const;
    Expr(const Expr& e);
    Expr(Expr&& e);
    Expr& operator=(const Expr& e);
    Expr& operator=(Expr&& e);
    ~Expr();
    
    ExprType get_type() const;
    std::string to_string() const;
    void apply(Expr* expr);

    ExprType _type;
    union {
        const char* _var;
        struct { const char* id; Expr* body; } _fn;
        struct { Expr* lhs; Expr* rhs; } _app;
    };

private:
    Expr();
    void _output(std::stringstream& ss) const;
    void _internal_swap(Expr* inner);
    void apply(const char* id, Expr* expr);
};
