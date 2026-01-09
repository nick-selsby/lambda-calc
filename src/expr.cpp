#include "expr.hpp"

#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <utility>

static const char* _cp_sv(std::string_view sv) {
    char* ptr = (char*)malloc(sv.size() + 1);
    memcpy(ptr, sv.data(), sv.size());
    ptr[sv.size()] = '\0';
    return ptr;
}

Expr::Expr() {
    //not safe! only call from static builder methods
}

std::unique_ptr<Expr> Expr::var(char id) {
    char* str = (char*)malloc(2);
    str[0] = id;
    str[1] = '\0';

    std::unique_ptr<Expr> e(new Expr());
    e->_type = ExprType::Var;
    e->_var = str;
    return e;
}

std::unique_ptr<Expr> Expr::var(std::string_view id) {
    std::unique_ptr<Expr> e(new Expr());
    e->_type = ExprType::Var;
    e->_var = _cp_sv(id);
    return e;
}

std::unique_ptr<Expr> Expr::fn(char id, std::unique_ptr<Expr> body) {
    char* str = (char*)malloc(2);
    str[0] = id;
    str[1] = '\0';

    std::unique_ptr<Expr> e(new Expr());
    e->_type = ExprType::Fn;
    e->_fn.id = str;
    e->_fn.body = body.release();
    return e;
}

std::unique_ptr<Expr> Expr::fn(std::string_view id, std::unique_ptr<Expr> body) {
    std::unique_ptr<Expr> e(new Expr());
    e->_type = ExprType::Fn;
    e->_fn.id = _cp_sv(id);
    e->_fn.body = body.release();
    return e;
}

std::unique_ptr<Expr> Expr::app(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs) {
    std::unique_ptr<Expr> e(new Expr());
    e->_type = ExprType::App;
    e->_app.lhs = lhs.release();
    e->_app.rhs = rhs.release();
    return e;
}

std::unique_ptr<Expr> Expr::clone() const {
    return std::make_unique<Expr>(*this);
}

Expr::Expr(const Expr& e) {
    *this = e;
}

Expr::Expr(Expr&& e) {
    *this = std::move(e);
}

Expr& Expr::operator=(const Expr& e) {
    this->~Expr();
    switch (_type) {
        default:
        case ExprType::Empty:
            break;
        case ExprType::Var:
            _var = strdup(e._var);
            break;
        case ExprType::Fn:
            _fn.id = strdup(_fn.id);
            _fn.body = new Expr(*this);
            break;
        case ExprType::App:
            _app.lhs = new Expr(*this);
            _app.rhs = new Expr(*this);
            break;
    }

    return *this;
}

Expr& Expr::operator=(Expr&& e) {
    this->~Expr();
    memcpy(this, &e, sizeof(*this));
    e._type = ExprType::Empty;
    return *this;
}

Expr::~Expr() {
    switch (_type) {
        default:
        case ExprType::Empty:
            break;
        case ExprType::Var:
            free((void*)_var);
            break;
        case ExprType::Fn:
            free((void*)_fn.id);
            delete _fn.body;
            break;
        case ExprType::App:
            delete _app.lhs;
            delete _app.rhs;
            break;
    }
}

ExprType Expr::get_type() const {
    return _type;
}

std::string Expr::to_string() const {
    std::stringstream ss;
    _output(ss);
    return ss.str();
}

void Expr::apply(Expr* expr) {
    apply(nullptr, expr);
}

void Expr::apply(const char* id, Expr* expr) {
    switch (_type) {
        default:
        case ExprType::Empty:
            break;
        case ExprType::Var: {
            if (strcmp(_var, id) == 0) {
                *this = *expr;
            }
            break;
        }
        case ExprType::Fn: {
            if (id == nullptr) {
                char* clone_id = strdup(_fn.id);
                _internal_swap(_fn.body);
                apply(clone_id, expr);
                free(clone_id);
            } else {
                if (strcmp(_fn.id, id) == 0) break;
                apply(id, expr);
            }

            break;
        }
        case ExprType::App: {
            apply(id, _app.lhs);
            apply(id, _app.rhs);
        }
    }
}

void Expr::_output(std::stringstream& ss) const {
    switch (_type) {
        default:
        case ExprType::Empty:
            break;
        case ExprType::Var:
            ss << _var;
            break;
        case ExprType::Fn:
            ss << '(' << '\\' << _var << '.';
            _fn.body->_output(ss);
            ss << ')';
            break;
        case ExprType::App:
            ss << '(';
            _app.lhs->_output(ss);
            ss << ' ';
            _app.rhs->_output(ss);
            ss << ')';
            break;
    }
}

void Expr::_internal_swap(Expr* inner) {
    Expr expr = std::move(*inner);
    *this = std::move(expr);
}