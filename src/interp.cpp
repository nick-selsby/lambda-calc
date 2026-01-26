#include "interp.hpp"
#include "expr.hpp"

#include <cctype>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstring>

bool is_id_char(char c) {
    switch (c) {
        case '\\':
        case '.':
            return false;
        default:
            return true;
    }

    //if (c >= 'a' && c <= 'z') return true;
    //if (c >= 'A' && c <= 'Z') return true;
    //if (c == '_') return true;
}

enum Token {
    TOKEN_ID,
    TOKEN_ASSIGN,
    TOKEN_FN_LAMBDA,
    TOKEN_FN_PERIOD,
    TOKEN_PARENL,
    TOKEN_PARENR,
    TOKEN_EOL,
};

std::vector<Token> t_tokens;
std::vector<std::string> t_ids;
std::vector<int> t_columns;
std::string_view t_expression_string;
bool t_concat;
const char* t_current_char;

size_t p_token;

std::string error;
bool has_error = false;

static std::string_view token_to_string(Token t) {
    using namespace std::string_view_literals;

    switch (t) {
        case TOKEN_ID: return "ID"sv;
        case TOKEN_ASSIGN: return "assign operator"sv;
        case TOKEN_FN_LAMBDA: return "lambda operator"sv;
        case TOKEN_FN_PERIOD: return "lambda body"sv;
        case TOKEN_PARENL: return "open parenthesis"sv;
        case TOKEN_PARENR: return "close parenthesis"sv;
        case TOKEN_EOL: return "EOL"sv;
    }

    return "?"sv;
}

static char* char_str(char x) {
    char* buf = new char[2];
    buf[0] = x;
    buf[1] = '\0';
    return buf;
}

static void push_token(Token token) {
    t_tokens.push_back(token);
    t_ids.emplace_back();
    t_columns.push_back(t_current_char - t_expression_string.begin() + 1);
    t_concat = false;
}

static void push_id_token(char c) {
    if (t_concat && !t_tokens.empty() && t_tokens.back() == TOKEN_ID) {
        t_ids.back() += c;
        return;
    }

    t_tokens.push_back(TOKEN_ID);
    t_ids.emplace_back() = c;
    t_columns.push_back(t_current_char - t_expression_string.begin() + 1);
    t_concat = true;
}

std::string get_error_text() {
    if (!has_error) return {};
    return error;
}

static void token_err(const std::string& text) {
    using namespace std::string_literals;
    error = "column "s + std::to_string(t_current_char - t_expression_string.begin()) + ": "s + text;
    has_error = true;
}

static void parse_err(const std::string& text) {
    using namespace std::string_literals;
    error = "column "s + std::to_string(t_columns[p_token]) + ": "s + text;
    has_error = true;
}

static void parse_bad_token_err() {
    using namespace std::string_literals;
    error = "column "s + std::to_string(t_columns[p_token]) + ": unexpected "s + std::string(token_to_string(t_tokens[p_token]));
    has_error = true;
}

static void tokenize() {
    using namespace std::string_literals;

    t_current_char = t_expression_string.begin();
    t_tokens.clear();
    t_ids.clear();
    t_columns.clear();
    t_concat = false;
    has_error = false;

    while (true) {
        if (t_current_char == t_expression_string.end()) break;
        char c = *t_current_char;
        
        if (std::isspace(c)) { t_concat = false; }
        else if (c == '=') { push_token(TOKEN_ASSIGN); }
        else if (c == '(') { push_token(TOKEN_PARENL); }
        else if (c == ')') { push_token(TOKEN_PARENR); }
        else if (c == '\\') { push_token(TOKEN_FN_LAMBDA); }
        else if (c == '.') { push_token(TOKEN_FN_PERIOD); }
        //else if (is_id_char(c)) { push_token(TOKEN_ID, c); }
        //else { token_err("unexpected character \""s + c + '\"'); return; }
        else { push_id_token(c); }

        t_current_char++;
    }
    push_token(TOKEN_EOL);
}

static std::unique_ptr<Expr> parse_expr();

static std::unique_ptr<Expr> parse_expr_id() {
    const std::string& id = t_ids[p_token];
    p_token++;
    std::unique_ptr<Expr> new_expr(new Expr);
    new_expr->_type = ExprType::Var;
    new_expr->_var = strdup(id.c_str());
    return new_expr;
}

static std::unique_ptr<Expr> parse_expr_lambda() {
    if (t_tokens[p_token] != TOKEN_FN_LAMBDA) {
        parse_bad_token_err();
        return nullptr;
    }
    p_token++;
    if (t_tokens[p_token] != TOKEN_ID) {
        parse_bad_token_err();
        return nullptr;
    }
    std::string& id = t_ids[p_token];
    p_token++;
    if (t_tokens[p_token] != TOKEN_FN_PERIOD) {
        parse_bad_token_err();
        return nullptr;
    }
    p_token++;
    auto expr = parse_expr();
    if (!expr) return nullptr;

    std::unique_ptr<Expr> new_expr(new Expr);
    new_expr->_type = ExprType::Fn;
    new_expr->_fn.id = strdup(id.c_str());
    new_expr->_fn.body = expr.release();
    return new_expr;
}

static std::unique_ptr<Expr> parse_expr_paren() {
    if (t_tokens[p_token] != TOKEN_PARENL) {
        parse_bad_token_err();
        return nullptr;
    }
    p_token++;
    auto expr = parse_expr();
    if (!expr) return nullptr;

    if (t_tokens[p_token] != TOKEN_PARENR) {
        parse_bad_token_err();
        return nullptr;
    }
    p_token++;

    return expr;
}

static std::unique_ptr<Expr> parse_expr() {
    std::unique_ptr<Expr> expr;

    while (true) {
        std::unique_ptr<Expr> next;

        switch (t_tokens[p_token]) {
            case TOKEN_ID:              next = parse_expr_id(); break;
            case TOKEN_FN_LAMBDA:       next = parse_expr_lambda(); break;
            case TOKEN_PARENL:          next = parse_expr_paren(); break;
            default:                    goto PARSE_EXPR_EOL;
        }

        if (!next) return nullptr;

        if (!expr) {
            expr = std::move(next);
        } else {
            std::unique_ptr<Expr> new_expr(new Expr);
            new_expr->_type = ExprType::App;
            new_expr->_app.lhs = expr.release();
            new_expr->_app.rhs = next.release();
            expr = std::move(new_expr);
        }
    }

    PARSE_EXPR_EOL:
    if (!expr) {
        parse_bad_token_err();
        return nullptr;
    }

    return expr;
}

static std::optional<Expr> _reset_and_parse_expr() {
    has_error = false;
    p_token = 0;

    std::unique_ptr<Expr> expr = parse_expr();
    if (!expr) return {};

    if (t_tokens[p_token] != TOKEN_EOL) {
        parse_bad_token_err();
        return {};
    }

    auto op = std::make_optional<Expr>();
    *op = std::move(*expr);
    expr.reset();
    return op;
}

static std::optional<Instruction> parse() {
    has_error = false;
    p_token = 0;

    std::optional<Instruction> inst = std::make_optional<Instruction>();

    if (t_tokens.size() >= 3 && t_tokens[0] == TOKEN_ID && t_tokens[1] == TOKEN_ASSIGN) {
        p_token = 2;
        auto expr = parse_expr();
        if (!expr) return std::nullopt;
        inst->assign_to = t_ids[0];
        inst->expr = std::move(expr);
    } else {
        auto expr = parse_expr();
        if (!expr) return std::nullopt;
        inst->expr = std::move(expr);
    }

    if (t_tokens[p_token] != TOKEN_EOL) {
        parse_bad_token_err();
        return std::nullopt;
    }
    
    return inst;
}

std::optional<Instruction> interpret_expression(std::string_view expr_str) {
    using namespace std::string_literals;

    t_expression_string = expr_str;
    tokenize();
    //for (int i = 0; i < t_tokens.size(); i++) {
    //    printf("%d ", t_tokens[i]);
    //}
    if (has_error) return std::nullopt;
    std::optional<Instruction> inst = parse();
    if (has_error) return std::nullopt;

    return inst;
}

std::optional<Expr> parse_expression(std::string_view expr_str) {
    t_expression_string = expr_str;
    tokenize();
    //for (int i = 0; i < t_tokens.size(); i++) {
    //    printf("%d ", t_tokens[i]);
    //}
    if (has_error) return std::nullopt;
    std::optional<Expr> expr = _reset_and_parse_expr();
    return expr;
}

/*
struct VariableMapHash {
    size_t operator()(const std::string& key) const { return std::hash<std::string_view>()(key); }
    size_t operator()(std::string_view key) const { return std::hash<std::string_view>()(key); }
    size_t operator()(const char* key) const { return std::hash<std::string_view>()(key); }
};

std::unordered_map<std::string, Expr*, VariableMapHash> variables;
struct StoredValue {
    std::string key;
    Expr* original_value;
};
std::vector<StoredValue> binding_stack;

static void _push_binding(const char* id, Expr* val) {
    auto& sv = binding_stack.emplace_back();
    sv.key.assign(id);
    auto it = variables.find(sv.key);
    if (it == variables.end()) {
        sv.original_value = nullptr;
        variables.emplace(sv.key, val);
    } else {
        sv.original_value = it->second;
        it->second = val;
    }
}

static void _pop_binding() {
    auto& sv = binding_stack.back();
    variables[sv.key] = sv.original_value;
    binding_stack.pop_back();
}
*/
struct _VariableDef {
    std::unique_ptr<char[]> id;
    std::unique_ptr<Expr> value;
};

static std::vector<_VariableDef> _variables;

static std::vector<_VariableDef>::iterator _find_var(const char* id) {
    for (size_t i = 0; i < _variables.size(); i++) {
        if (strcmp(_variables[i].id.get(), id) == 0) {
            return _variables.begin() + i;
        }
    }

    return _variables.end();
}

void clear_variables() {
    _variables.clear();
}

bool clear_variable(const char* id) {
    using namespace std::string_literals;
    auto it = _find_var(id);
    if (it == _variables.end()) {
        has_error = true;
        error = "variable "s + id + " is not assigned";
        return false;
    }
    
    _variables.erase(it);
    has_error = false;
    return true;
}

bool set_variable(const char* id, const Expr& expr) {
    using namespace std::string_literals;
    auto it = _find_var(id);
    _VariableDef* def;
    if (it == _variables.end()) {
        def = &_variables.emplace_back();
    } else {
        def = &*it;
    }

    def->id.reset(strdup(id));
    def->value.reset(expr.clone());
    has_error = false;
    return true;
}

bool set_variable(const char* id, const char* raw_expr) {
    std::optional<Expr> expr = parse_expression(raw_expr);
    if (!expr) return false;
    return set_variable(id, *expr);
}

Expr* get_variable(const char* id) {
    using namespace std::string_literals;
    auto it = _find_var(id);
    if (it == _variables.end()) {
        has_error = true;
        error = "variable "s + id + " is not assigned";
        return nullptr;
    }

    has_error = false;
    return it->value.get();
}

struct _Binding {
    std::unique_ptr<char[]> id;
    std::unique_ptr<Expr> expr;
};

using _Bindings = std::vector<_Binding>;

static bool _bind_get(const _Bindings& bindings, const char* id, Expr** bind_result) {
    for (auto it = bindings.rbegin(); it != bindings.rend(); it++) {
        if (strcmp(it->id.get(), id) == 0) {
            *bind_result = it->expr.get();
            return true;
        }
    }
    return false;
}

Expr* _reduce_expression(Expr* expr, _Bindings& bindings) {
    switch (expr->_type) {
    default:
    case ExprType::Empty:
        has_error = true;
        error = "corrupted expression passed to function";
        return nullptr;
    case ExprType::Var: {
        Expr* bind = nullptr;
        bool has_bind = _bind_get(bindings, expr->_var, &bind);
        if (!has_bind) {
            Expr* value = get_variable(expr->_var);
            if (value == nullptr) return nullptr; //error propagates
            return _reduce_expression(value, bindings);
        }
        else if (bind == nullptr) {
            has_error = false;
            return expr->clone();
        }
        else {
            return _reduce_expression(bind, bindings);
        }
    }
    case ExprType::Fn: {
        auto& binding = bindings.emplace_back();
        binding.id.reset(strdup(expr->_fn.id));
        Expr* reduced_inner = _reduce_expression(expr->_fn.body, bindings);
        bindings.pop_back();
        if (reduced_inner == nullptr) return nullptr; //error propagates
        Expr* reduced = new Expr;
        reduced->_type = ExprType::Fn;
        reduced->_fn.id = strdup(expr->_fn.id);
        reduced->_fn.body = reduced_inner;
        has_error = false;
        return reduced;
    }
    case ExprType::App: {
        Expr* reduced_lhs = _reduce_expression(expr->_app.lhs, bindings);
        if (reduced_lhs == nullptr) return nullptr; //error propagates
        Expr* reduced_rhs = _reduce_expression(expr->_app.rhs, bindings);
        if (reduced_rhs == nullptr) {
            delete reduced_lhs;
            return nullptr;
        }
        
        auto& binding = bindings.emplace_back();
        binding.id.reset(strdup(expr->_fn.id));
        binding.expr.reset(reduced_rhs);
        Expr* reduced = _reduce_expression(reduced_lhs->_fn.body, bindings); //error propagates
        bindings.pop_back();

        delete reduced_lhs;
        return reduced;
    }
    }
}

Expr* reduce_expression(Expr* expr) {
    _Bindings bindings;
    return _reduce_expression(expr, bindings);
}