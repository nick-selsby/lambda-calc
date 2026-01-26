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

Expr Expr::var(char id) {
    char* str = (char*)malloc(2);
    str[0] = id;
    str[1] = '\0';

    Expr e;
    e._type = ExprType::Var;
    e._var = str;
    return e;
}

Expr Expr::var(std::string_view id) {
    Expr e;
    e._type = ExprType::Var;
    e._var = _cp_sv(id);
    return e;
}

Expr Expr::fn(char id, const Expr& body) {
    char* str = (char*)malloc(2);
    str[0] = id;
    str[1] = '\0';

    Expr e;
    e._type = ExprType::Fn;
    e._fn.id = str;
    e._fn.body = body.clone();
    return e;
}

Expr Expr::fn(std::string_view id, const Expr& body) {
    Expr e;
    e._type = ExprType::Fn;
    e._fn.id = _cp_sv(id);
    e._fn.body = body.clone();
    return e;
}

Expr Expr::app(const Expr& lhs, const Expr& rhs) {
    Expr e;
    e._type = ExprType::App;
    e._app.lhs = lhs.clone();
    e._app.rhs = rhs.clone();
    return e;
}

Expr::Expr(const Expr& e) {
    *this = e;
}

Expr::Expr(Expr&& e) {
    *this = std::move(e);
}

Expr& Expr::operator=(const Expr& e) {
    this->~Expr();
    _type = e._type;
    switch (_type) {
        default:
        case ExprType::Empty:
            break;
        case ExprType::Var:
            _var = strdup(e._var);
            break;
        case ExprType::Fn:
            _fn.id = strdup(e._fn.id);
            _fn.body = new Expr(*e._fn.body);
            break;
        case ExprType::App:
            _app.lhs = new Expr(*e._app.lhs);
            _app.rhs = new Expr(*e._app.rhs);
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

Expr* Expr::clone() const {
    return new Expr(*this);
}

ExprType Expr::get_type() const {
    return _type;
}

std::string Expr::to_string() const {
    std::stringstream ss;
    _output(ss);
    return ss.str();
}

struct _SubsEntry {
    const char* id;
    Expr* value;
};

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