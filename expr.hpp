/*

\x.x
e:=x
e e

0 := λf.λx.x
1 := λf.λx.f x
2 := λf.λx.f (f x)
3 := λf.λx.f (f (f x))

*/

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
    static std::unique_ptr<Expr> var(std::string_view id);
    static std::unique_ptr<Expr> fn(std::string_view id, std::unique_ptr<Expr> body);
    static std::unique_ptr<Expr> app(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs);
    Expr(const Expr& e);
    Expr(Expr&& e);
    Expr& operator=(const Expr& e);
    Expr& operator=(Expr&& e);
    ~Expr();
    
    ExprType get_type() const;
    std::string to_string() const;

private:
    Expr();
    void _output(std::stringstream& ss) const;

    ExprType _type;
    union {
        const char* _var;
        struct { const char* id; Expr* body; } _fn;
        struct { Expr* lhs; Expr* rhs; } _app;
    };

};
