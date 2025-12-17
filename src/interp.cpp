#include "interp.hpp"

#include <cctype>
#include <cstdio>
#include <memory>
#include <optional>
#include <pthread.h>
#include <string>
#include <string_view>
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
    t_columns.push_back(t_current_char - t_expression_string.begin());
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
            case TOKEN_EOL:             goto PARSE_EXPR_EOL;
            default:                    parse_bad_token_err(); return nullptr;
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

    if (t_tokens.size() >= 3 && t_tokens[0] == TOKEN_ID && t_tokens[1] == TOKEN_ASSIGN) {
        p_token = 2;
        auto expr = parse_expr();
        if (!expr) return std::nullopt;
        std::optional<Instruction> inst = std::make_optional<Instruction>();
        inst->assign_to = t_ids[0];
        inst->expr = std::move(expr);
        return inst;
    }

    auto expr = parse_expr();
    if (!expr) return std::nullopt;
    std::optional<Instruction> inst = std::make_optional<Instruction>();
    inst->expr = std::move(expr);
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