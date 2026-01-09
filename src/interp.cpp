#include "interp.hpp"
#include "expr.hpp"

#include <cctype>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

bool is_id_char(char c) {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c == '_') return true;

    return false;
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
std::vector<char> t_ids;
std::vector<int> t_columns;
std::string_view t_expression_string;
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

static void push_token(Token token, char c = '\0') {
    t_tokens.push_back(token);
    t_ids.push_back(c);
    t_columns.push_back(t_current_char - t_expression_string.begin() + 1);
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
    has_error = false;

    while (true) {
        if (t_current_char == t_expression_string.end()) break;
        char c = *t_current_char;
        
        if (std::isspace(c)) {}
        else if (is_id_char(c)) { push_token(TOKEN_ID, c); }
        else if (c == '=') { push_token(TOKEN_ASSIGN); }
        else if (c == '(') { push_token(TOKEN_PARENL); }
        else if (c == ')') { push_token(TOKEN_PARENR); }
        else if (c == '\\') { push_token(TOKEN_FN_LAMBDA); }
        else if (c == '.') { push_token(TOKEN_FN_PERIOD); }
        else { token_err("unexpected character \""s + c + '\"'); return; }

        t_current_char++;
    }
    push_token(TOKEN_EOL);
}

static std::unique_ptr<Expr> parse_expr();

static std::unique_ptr<Expr> parse_expr_id() {
    auto expr = Expr::var(t_ids[p_token]);
    p_token++;
    return expr;
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
    char id = t_ids[p_token];
    p_token++;
    if (t_tokens[p_token] != TOKEN_FN_PERIOD) {
        parse_bad_token_err();
        return nullptr;
    }
    p_token++;
    auto expr = parse_expr();
    if (!expr) return nullptr;

    return Expr::fn(id, std::move(expr));
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
            expr = Expr::app(std::move(expr), std::move(next));
        }
    }

    PARSE_EXPR_EOL:
    if (!expr) {
        parse_bad_token_err();
        return nullptr;
    }

    return expr;
}

static std::optional<Instruction> parse() {
    has_error = false;
    p_token = 0;

    std::optional<Instruction> inst = std::make_optional<Instruction>();

    if (t_tokens.size() >= 3 && t_tokens[0] == TOKEN_ID && t_tokens[1] == TOKEN_ASSIGN) {
        p_token = 2;
        auto expr = parse_expr();
        if (!expr) return std::nullopt;
        std::optional<Instruction> inst = std::make_optional<Instruction>();
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
    if (has_error) return std::nullopt;
    std::optional<Instruction> inst = parse();
    if (has_error) return std::nullopt;

    return inst;
}

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

void clear_variables() {
    for (auto& [_, v] : variables) {
        delete v;
    }
    variables.clear();
}

bool clear_variable(const char* id) {
    using namespace std::string_literals;
    auto it = variables.find(id);
    if (it == variables.end()) {
        has_error = true;
        error = "variable "s + id + " is not assigned";
        return false;
    }
    
    delete it->second;
    variables.erase(it);
    has_error = false;
    return true;
}

Expr* get_variable(const char* id) {
    using namespace std::string_literals;
    auto it = variables.find(id);
    if (it == variables.end()) {
        has_error = true;
        error = "variable "s + id + " is not assigned";
        return nullptr;
    }

    has_error = false;
    return it->second;
}

//Returns an expr that is GUARANTEED to be a function on success, or nullptr on err.
//Don't leave owned unitialized! It is read and written to!
static bool _reduce_expression(Expr* expr, Expr** output, bool* owned) {
    while (true) {
        if (expr->get_type() == ExprType::Fn) {
            *output = expr;
            return true;
        }
        else if (expr->get_type() == ExprType::Var) {
            expr = get_variable(expr->_var);
        } 
        else {
            bool expr1_owned = false;
            Expr* expr1;
            bool success = _reduce_expression(expr->_app.lhs, &expr1, &expr1_owned);
            if (success) {
                bool expr2_owned = false;
                Expr* expr2;
                success = _reduce_expression(expr->_app.rhs, &expr2, &expr2_owned);
                if (success) {
                    _push_binding(expr1->_fn.id, expr2);
                    bool expr3_owned = true;
                    auto expr3 = expr1->_fn.body->clone();
                    _pop_binding();
                }
            }
        }

        if (has_error) {
            *output = expr;
            return false;
        }
    }
}

std::unique_ptr<Expr> reduce_expression(std::unique_ptr<Expr> expr) {
    while (true) {
        if (expr->get_type() == ExprType::Fn) {
            return expr;
        }
        else if (expr->get_type() == ExprType::Var) {
            //expr = get_variable(expr->_var);
        } 
        else {
            //auto expr_1 = reduce_expression(expr->_app.lhs);
        }

        if (has_error) return nullptr;
    }
}