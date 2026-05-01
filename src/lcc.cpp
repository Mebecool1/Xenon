// luabasec.cpp — LuaBase transpiler, fully standalone C++
// Build: g++ -o lcc lcc.cpp -O2 -std=c++17
//
// Converted from Python implementation. All features F01-F30 preserved.
// All bug fixes BUG-A through BUG-M preserved.

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ===========================================================================
// TokenType
// ===========================================================================
enum class TT {
  NUMBER,
  STRING,
  IDENTIFIER,
  IF,
  THEN,
  ELSE,
  END,
  WHILE,
  DO,
  FUNCTION,
  RETURN,
  PRINT,
  SCANF,
  INT,
  STR,
  FLOAT,
  FOPEN,
  FWRITE,
  FCLOSE,
  HLT,
  ASSIGN,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  EQ,
  NE,
  LT,
  GT,
  MOD,
  LE,
  GE,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  COMMA,
  TEOF,
  LINK,
  LSBRACKET,
  RSBRACKET,
  LONG,
  DOUBLE,
  M256,
  VOID,
  SHORT,
  M256I,
  TYPE,
  ELSEIF,
  AND,
  OR,
  PTR,
  ADDRESS_OF,
  DOT,
  // F01-F30
  FOR,
  BREAK,
  CONTINUE,
  NOT,
  BITNOT,
  BITOR,
  BITXOR,
  SHL,
  SHR,
  PRINTLN,
  ASSERT,
  SWITCH,
  CASE,
  DEFAULT_KW,
  COLON,
  CONST_KW,
  CAST,
  TYPEOF,
  SIZEOF_KW,
  QUESTION,
  PLUS_ASSIGN,
  MINUS_ASSIGN,
  MUL_ASSIGN,
  DIV_ASSIGN,
  MOD_ASSIGN,
  INCR,
  DECR,
  POW,
  FREAD,
  STRLEN_KW,
  STRCPY_KW,
  STRCAT_KW,
  MEMSET_KW,
  EXIT_KW,
  ENUM_KW,
  BOOL_KW,
  TRUE_KW,
  FALSE_KW,
  PRINTFMT,
  CHAR_KW,
  ARROW,
  CHAR_LIT,
  SEMICOLON,
  NULL_KW,
  INLINE_KW,
  DEFINE_KW,
  U8,
  U32,
  U64,
  TRY_KW,
  EXCEPT_KW,
  THROW_KW,
  GLOBAL_KW,
  LET_KW,
  VAR_KW,
  
};

static std::string tt_name(TT t) {
  switch (t) {
#define C(x)                                                                   \
  case TT::x:                                                                  \
    return #x
    C(NUMBER);
    C(STRING);
    C(IDENTIFIER);
    C(IF);
    C(THEN);
    C(ELSE);
    C(END);
    C(WHILE);
    C(DO);
    C(FUNCTION);
    C(RETURN);
    C(PRINT);
    C(SCANF);
    C(INT);
    C(STR);
    C(FLOAT);
    C(FOPEN);
    C(FWRITE);
    C(FCLOSE);
    C(HLT);
    C(ASSIGN);
    C(PLUS);
    C(MINUS);
    C(MULTIPLY);
    C(DIVIDE);
    C(EQ);
    C(NE);
    C(LT);
    C(GT);
    C(MOD);
    C(LE);
    C(GE);
    C(LPAREN);
    C(RPAREN);
    C(LBRACE);
    C(RBRACE);
    C(COMMA);
    C(TEOF);
    C(LINK);
    C(LSBRACKET);
    C(RSBRACKET);
    C(LONG);
    C(DOUBLE);
    C(M256);
    C(VOID);
    C(SHORT);
    C(M256I);
    C(TYPE);
    C(ELSEIF);
    C(AND);
    C(OR);
    C(PTR);
    C(ADDRESS_OF);
    C(DOT);
    C(FOR);
    C(BREAK);
    C(CONTINUE);
    C(NOT);
    C(BITNOT);
    C(BITOR);
    C(BITXOR);
    C(SHL);
    C(SHR);
    C(PRINTLN);
    C(ASSERT);
    C(SWITCH);
    C(CASE);
    C(DEFAULT_KW);
    C(COLON);
    C(CONST_KW);
    C(CAST);
    C(TYPEOF);
    C(SIZEOF_KW);
    C(QUESTION);
    C(PLUS_ASSIGN);
    C(MINUS_ASSIGN);
    C(MUL_ASSIGN);
    C(DIV_ASSIGN);
    C(MOD_ASSIGN);
    C(INCR);
    C(DECR);
    C(POW);
    C(FREAD);
    C(STRLEN_KW);
    C(STRCPY_KW);
    C(STRCAT_KW);
    C(MEMSET_KW);
    C(EXIT_KW);
    C(ENUM_KW);
    C(BOOL_KW);
    C(TRUE_KW);
    C(FALSE_KW);
    C(PRINTFMT);
    C(CHAR_KW);
    C(ARROW);
    C(CHAR_LIT);
    C(SEMICOLON);
    C(NULL_KW);
    C(INLINE_KW);
    C(DEFINE_KW);
    C(U8);
    C(U32);
    C(U64);
    C(TRY_KW);
    C(EXCEPT_KW);
    C(THROW_KW);
    C(GLOBAL_KW);
    C(LET_KW);
    C(VAR_KW);
    
#undef C
  default:
    return "UNKNOWN";
  }
}

// ===========================================================================
// LuaBaseError
// ===========================================================================
struct LuaBaseError : std::exception {
  std::string kind, msg_full;
  LuaBaseError(std::string kind_, std::string msg, std::optional<int> line = {},
               std::optional<int> col = {}, std::string snippet = "")
      : kind(std::move(kind_)) {
    std::ostringstream ss;
    ss << "[" << kind << "]";
    if (line)
      ss << " (line " << *line << ", col " << *col << ")";
    ss << ": " << msg;
    if (!snippet.empty())
      ss << "\n    " << snippet;
    msg_full = ss.str();
  }
  const char *what() const noexcept override { return msg_full.c_str(); }
};

// ===========================================================================
// Token
// ===========================================================================
struct Token {
  TT type;
  std::string value;
  int line{0}, col{0};
  Token(TT t, std::string v, int l = 0, int c = 0)
      : type(t), value(std::move(v)), line(l), col(c) {}
};

// ===========================================================================
// Lexer
// ===========================================================================
class Lexer {
  std::string src;
  size_t pos{0};
  int line{1}, col{1};

  static const std::map<std::string, TT> &keywords() {
    static std::map<std::string, TT> kw = {
        {"if", TT::IF},
        {"then", TT::THEN},
        {"else", TT::ELSE},
        {"end", TT::END},
        {"while", TT::WHILE},
        {"do", TT::DO},
        {"function", TT::FUNCTION},
        {"return", TT::RETURN},
        {"print", TT::PRINT},
        {"scanf", TT::SCANF},
        {"int", TT::INT},
        {"str", TT::STR},
        {"float", TT::FLOAT},
        {"fopen", TT::FOPEN},
        {"fwrite", TT::FWRITE},
        {"fclose", TT::FCLOSE},
        {"hlt", TT::HLT},
        {"link", TT::LINK},
        {"long", TT::LONG},
        {"double", TT::DOUBLE},
        {"__m256", TT::M256},
        {"void", TT::VOID},
        {"short", TT::SHORT},
        {"__m256i", TT::M256I},
        {"type", TT::TYPE},
        {"elseif", TT::ELSEIF},
        {"ptr", TT::PTR},
        {"and", TT::AND},
        {"or", TT::OR},
        {"for", TT::FOR},
        {"break", TT::BREAK},
        {"continue", TT::CONTINUE},
        {"not", TT::NOT},
        {"println", TT::PRINTLN},
        {"assert", TT::ASSERT},
        {"switch", TT::SWITCH},
        {"case", TT::CASE},
        {"default", TT::DEFAULT_KW},
        {"const", TT::CONST_KW},
        {"cast", TT::CAST},
        {"typeof", TT::TYPEOF},
        {"sizeof", TT::SIZEOF_KW},
        {"exit", TT::EXIT_KW},
        {"enum", TT::ENUM_KW},
        {"bool", TT::BOOL_KW},
        {"true", TT::TRUE_KW},
        {"false", TT::FALSE_KW},
        {"printfmt", TT::PRINTFMT},
        {"char", TT::CHAR_KW},
        {"fread", TT::FREAD},
        {"strlen", TT::STRLEN_KW},
        {"strcpy", TT::STRCPY_KW},
        {"strcat", TT::STRCAT_KW},
        {"memset", TT::MEMSET_KW},
        {"null", TT::NULL_KW},
        {"inline", TT::INLINE_KW},
        {"define", TT::DEFINE_KW},
        {"u8", TT::U8},
        {"u32", TT::U32},
        {"u64", TT::U64},
        {"try", TT::TRY_KW},
        {"except", TT::EXCEPT_KW},
        {"throw", TT::THROW_KW},
        {"global", TT::GLOBAL_KW},
        {"let", TT::LET_KW},
        {"var", TT::VAR_KW},
        
    };
    return kw;
  }

  char advance_char() {
    char ch = src[pos++];
    if (ch == '\n') {
      line++;
      col = 1;
    } else
      col++;
    return ch;
  }

  std::string current_line_text() const {
    size_t start = src.rfind('\n', pos > 0 ? pos - 1 : 0);
    start = (start == std::string::npos) ? 0 : start + 1;
    size_t end = src.find('\n', pos);
    end = (end == std::string::npos) ? src.size() : end;
    return src.substr(start, end - start);
  }

public:
  explicit Lexer(std::string source) : src(std::move(source)) {}

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    while (pos < src.size()) {
      char c = src[pos];

      // whitespace
      if (std::isspace((unsigned char)c)) {
        advance_char();
        continue;
      }

      // # line comment
      if (c == '#') {
        while (pos < src.size() && src[pos] != '\n')
          advance_char();
        continue;
      }

      // // single-line comment
      if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '/') {
        while (pos < src.size() && src[pos] != '\n')
          advance_char();
        continue;
      }

      // /* block comment */
      if (c == '/' && pos + 1 < src.size() && src[pos + 1] == '*') {
        advance_char();
        advance_char();
        while (pos + 1 < src.size()) {
          if (src[pos] == '*' && src[pos + 1] == '/') {
            advance_char();
            advance_char();
            break;
          }
          advance_char();
        }
        continue;
      }

      int tok_line = line, tok_col = col;

      // identifiers / keywords
      if (std::isalpha((unsigned char)c) || c == '_') {
        size_t start = pos;
        while (pos < src.size() &&
               (std::isalnum((unsigned char)src[pos]) || src[pos] == '_'))
          advance_char();
        std::string val = src.substr(start, pos - start);
        auto it = keywords().find(val);
        tokens.emplace_back(it != keywords().end() ? it->second
                                                   : TT::IDENTIFIER,
                            val, tok_line, tok_col);
        continue;
      }

      // numeric literals (decimal, float, hex 0x...)
      if (std::isdigit((unsigned char)c)) {
        size_t start = pos;
        bool dot_seen = false;
        if (c == '0' && pos + 1 < src.size() &&
            (src[pos + 1] == 'x' || src[pos + 1] == 'X')) {
          advance_char();
          advance_char();
          while (pos < src.size() && std::isxdigit((unsigned char)src[pos]))
            advance_char();
        } else {
          while (pos < src.size() && (std::isdigit((unsigned char)src[pos]) ||
                                      (src[pos] == '.' && !dot_seen))) {
            if (src[pos] == '.')
              dot_seen = true;
            advance_char();
          }
        }
        tokens.emplace_back(TT::NUMBER, src.substr(start, pos - start),
                            tok_line, tok_col);
        continue;
      }

      // string literals
      if (c == '"') {
        advance_char();
        size_t start = pos;
        while (pos < src.size() && src[pos] != '"') {
          if (src[pos] == '\n')
            throw LuaBaseError("LexError", "Unterminated string literal",
                               tok_line, tok_col, current_line_text());
          if (src[pos] == '\\' && pos + 1 < src.size())
            advance_char();
          advance_char();
        }
        if (pos >= src.size())
          throw LuaBaseError("LexError",
                             "Unterminated string literal at end of file",
                             tok_line, tok_col);
        std::string val = src.substr(start, pos - start);
        advance_char();
        tokens.emplace_back(TT::STRING, val, tok_line, tok_col);
        continue;
      }

      // char literals 'A'
      if (c == '\'') {
        advance_char();
        size_t start = pos;
        if (pos < src.size() && src[pos] == '\\')
          advance_char();
        if (pos < src.size())
          advance_char();
        std::string val = src.substr(start, pos - start);
        if (pos < src.size() && src[pos] == '\'')
          advance_char();
        tokens.emplace_back(TT::CHAR_LIT, val, tok_line, tok_col);
        continue;
      }

      // two-char operators
      if (pos + 1 < src.size()) {
        std::string d = src.substr(pos, 2);
        static const std::map<std::string, TT> ops2 = {
            {"==", TT::EQ},          {"!=", TT::NE},
            {"<=", TT::LE},          {">=", TT::GE},
            {"&&", TT::AND},         {"||", TT::OR},
            {"<<", TT::SHL},         {">>", TT::SHR},
            {"+=", TT::PLUS_ASSIGN}, {"-=", TT::MINUS_ASSIGN},
            {"*=", TT::MUL_ASSIGN},  {"/=", TT::DIV_ASSIGN},
            {"%=", TT::MOD_ASSIGN},  {"++", TT::INCR},
            {"--", TT::DECR},        {"**", TT::POW},
            {"->", TT::ARROW},
        };
        auto it2 = ops2.find(d);
        if (it2 != ops2.end()) {
          tokens.emplace_back(it2->second, d, tok_line, tok_col);
          advance_char();
          advance_char();
          continue;
        }
      }

      // single-char operators
      {
        static const std::map<char, TT> ops1 = {
            {'+', TT::PLUS},       {'-', TT::MINUS},     {'*', TT::MULTIPLY},
            {'/', TT::DIVIDE},     {'%', TT::MOD},       {'=', TT::ASSIGN},
            {'(', TT::LPAREN},     {')', TT::RPAREN},    {'{', TT::LBRACE},
            {'}', TT::RBRACE},     {',', TT::COMMA},     {'<', TT::LT},
            {'>', TT::GT},         {']', TT::RSBRACKET}, {'[', TT::LSBRACKET},
            {'&', TT::ADDRESS_OF}, {'.', TT::DOT},       {'~', TT::BITNOT},
            {'|', TT::BITOR},      {'^', TT::BITXOR},    {'?', TT::QUESTION},
            {':', TT::COLON},      {';', TT::SEMICOLON},
        };
        auto it1 = ops1.find(c);
        if (it1 != ops1.end()) {
          tokens.emplace_back(it1->second, std::string(1, c), tok_line,
                              tok_col);
          advance_char();
          continue;
        }
      }

      throw LuaBaseError("LexError",
                         std::string("Unexpected character '") + c + "'",
                         tok_line, tok_col, current_line_text());
    }
    tokens.emplace_back(TT::TEOF, "", line, col);
    return tokens;
  }
};

// ===========================================================================
// Helpers
// ===========================================================================
static std::string c_path(const std::string &p) {
  std::string r = p;
  size_t pos = 0;
  while ((pos = r.find('\\', pos)) != std::string::npos) {
    r.replace(pos, 1, "\\\\");
    pos += 2;
  }
  return r;
}

static bool ends_with(const std::string &s, const std::string &suf) {
  return s.size() >= suf.size() &&
         s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

static std::string join(const std::vector<std::string> &v,
                        const std::string &sep) {
  std::string r;
  for (size_t i = 0; i < v.size(); i++) {
    if (i)
      r += sep;
    r += v[i];
  }
  return r;
}

// ===========================================================================
// TypeInfo — rich type representation used by the inference engine
// ===========================================================================
struct TypeInfo {
  std::string base;   // "int", "float", "double", "char*", "bool", "void", etc.
  int ptr_depth{0};   // extra pointer indirections beyond what's in base
  bool is_array{false};
  bool is_const{false};

  static TypeInfo of(std::string b, int pd = 0) {
    TypeInfo t; t.base = std::move(b); t.ptr_depth = pd; return t;
  }
  static TypeInfo unknown() { return TypeInfo::of("int"); }

  // Render to a C declaration type string
  std::string c_type() const {
    std::string r = base;
    for (int i = 0; i < ptr_depth; i++) r += "*";
    return r;
  }

  bool is_ptr()     const { return ptr_depth > 0 || base.size() > 0 && base.back() == '*'; }
  bool is_float()   const { return base == "float" || base == "double"; }
  bool is_integer() const {
    return base == "int" || base == "long" || base == "short" ||
           base == "char" || base == "bool" || base == "uint8_t" ||
           base == "uint32_t" || base == "uint64_t";
  }
  bool is_numeric() const { return is_float() || is_integer(); }
};

// Arithmetic promotion: widest numeric type wins (mirrors C's usual arithmetic conversions)
static TypeInfo promote(const TypeInfo &a, const TypeInfo &b) {
  // pointer arithmetic: ptr ± int → ptr
  if (a.is_ptr() && b.is_integer()) return a;
  if (b.is_ptr() && a.is_integer()) return b;

  // float > double? No — double > float
  if (a.base == "double" || b.base == "double") return TypeInfo::of("double");
  if (a.base == "float"  || b.base == "float")  return TypeInfo::of("float");

  // integer promotion: pick the wider one
  auto rank = [](const std::string &s) -> int {
    if (s == "bool")      return 0;
    if (s == "char")      return 1;
    if (s == "uint8_t")   return 1;
    if (s == "short")     return 2;
    if (s == "int")       return 3;
    if (s == "uint32_t")  return 4;
    if (s == "long")      return 5;
    if (s == "uint64_t")  return 6;
    return 3; // unknown → int
  };
  return rank(a.base) >= rank(b.base) ? a : b;
}

// ===========================================================================
// CTranspiler
// ===========================================================================
class CTranspiler {
  bool isSubTranspiler = false;
  std::vector<Token> tokens;
  size_t pos{0};

  std::vector<std::string> headers;
  std::vector<std::string> functions;
  std::vector<std::string> main_body;
  std::map<std::string, std::string> var_types;
  std::map<std::string, std::string> func_return_types; // fname → raw return type
  // struct_name → {field_name → c_type}
  std::map<std::string, std::map<std::string, std::string>> struct_field_types;
  // fname → vector of param c_types (for argument-position inference)
  std::map<std::string, std::vector<std::string>> func_param_types;
  std::set<std::string> _handle_declared;
  std::set<std::string> _lh_included;
  std::set<std::string> _enum_names;
  std::string _source_file{"<unknown>"};
  std::string _source_dir{"."};
  std::vector<std::string> _include_paths;

  Token &current() { return tokens[pos]; }
  Token advance() { return tokens[pos++]; }

  Token expect(TT ttype, bool shut) {
    Token &tok = current();
    if (tok.type == ttype)
      return advance();
    std::string got =
        tok.value.empty() ? tt_name(tok.type) : ("'" + tok.value + "'");
    if (!shut)
      throw LuaBaseError("SyntaxError",
                         "Expected " + tt_name(ttype) + ", got " + got,
                         tok.line, tok.col);
    return Token(ttype, "", tok.line, tok.col);
  }

  std::string safe_name(const std::string &name) {
    static const std::set<std::string> c_keywords = {
        "double",   "int",      "float",    "char",     "while",    "if",
        "return",   "break",    "FILE",     "long",     "void",     "short",
        "__m256",   "__m256i",  "for",      "do",       "else",     "struct",
        "typedef",  "switch",   "case",     "default",  "const",    "static",
        "unsigned", "signed",   "extern",   "goto",     "sizeof",   "enum",
        "union",    "continue", "register", "volatile", "auto",     "bool",
        "true",     "false",    "NULL",     "uint8_t",  "uint32_t", "uint64_t",
        "inline", "auto"
    };
    return c_keywords.count(name) ? "var_" + name : name;
  }

  std::string line_directive(const Token &tok) {
    if (tok.line == 0)
      return "";
    return "#line " + std::to_string(tok.line) + " \"" + c_path(_source_file) +
           "\"\n";
  }

  bool has_header(const std::string &h) {
    for (auto &hh : headers)
      if (hh.find(h) != std::string::npos)
        return true;
    return false;
  }

  // -----------------------------------------------------------------------
  // enum
  // -----------------------------------------------------------------------
  std::string parse_enum() {
    advance(); // skip 'enum'
    Token name_tok = current();
    std::string enum_name = expect(TT::IDENTIFIER, false).value;
    _enum_names.insert(enum_name);
    expect(TT::LBRACE, false);
    std::vector<std::string> members;
    while (current().type != TT::RBRACE) {
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError",
                           "Unterminated enum '" + enum_name + "'",
                           name_tok.line, name_tok.col);
      std::string m = expect(TT::IDENTIFIER, false).value;
      if (current().type == TT::ASSIGN) {
        advance();
        members.push_back(m + " = " + parse_expr());
      } else {
        members.push_back(m);
      }
      if (current().type == TT::COMMA)
        advance();
    }
    expect(TT::RBRACE, false);
    return "typedef enum {\n    " + join(members, ",\n    ") + "\n} " +
           enum_name + ";\n";
  }

  // -----------------------------------------------------------------------
  // type (struct)
  // -----------------------------------------------------------------------
  std::string parse_type_definition() {
    advance(); // skip 'type'
    Token name_tok = current();
    std::string struct_name = expect(TT::IDENTIFIER, false).value;
    expect(TT::LBRACE, false);
    std::vector<std::string> fields;
    static const std::set<TT> valid_field_types = {
        TT::INT,        TT::FLOAT,   TT::STR,     TT::LONG, TT::SHORT,
        TT::DOUBLE,     TT::VOID,    TT::PTR,     TT::M256, TT::M256I,
        TT::IDENTIFIER, TT::BOOL_KW, TT::CHAR_KW,
    };
    while (current().type != TT::RBRACE) {
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError",
                           "Unterminated type '" + struct_name + "'",
                           name_tok.line, name_tok.col);
      Token ft = current();
      if (!valid_field_types.count(ft.type))
        throw LuaBaseError("SyntaxError",
                           "Expected field type in '" + struct_name +
                               "', got '" + ft.value + "'",
                           ft.line, ft.col);
      std::string f_type_raw = advance().value;
      std::string f_type;
      if (f_type_raw == "ptr") {
        std::string inner = advance().value;
        if (inner == "str")
          inner = "char*";
        f_type = inner + "*";
      } else {
        // BUG-L FIXED
        if (f_type_raw == "void")
          throw LuaBaseError("SyntaxError",
                             "'void' is invalid as a struct field in '" +
                                 struct_name + "'. Use 'ptr void'.",
                             ft.line, ft.col);
        f_type = (f_type_raw == "str") ? "char*" : f_type_raw;
      }
      // BUG-K FIXED
      std::string f_name = safe_name(expect(TT::IDENTIFIER, false).value);
      if (current().type == TT::LSBRACKET) {
        advance();
        std::string sz;
        if (current().type == TT::NUMBER || current().type == TT::IDENTIFIER)
          sz = advance().value;
        else
          sz = expect(TT::NUMBER, false).value; // triggers proper error
        expect(TT::RSBRACKET, false);
        fields.push_back(f_type + " " + f_name + "[" + sz + "];");
      } else {
        fields.push_back(f_type + " " + f_name + ";");
      }
      if (current().type == TT::SEMICOLON)
        advance();
    }
    expect(TT::RBRACE, false);
    var_types[struct_name] = "STRUCT";
    // Register field types for member-access inference
    auto &field_map = struct_field_types[struct_name];
    for (const auto &fd : fields) {
      // fd looks like "int foo;" or "char* bar;" or "float baz[4];"
      // Extract type and name naively: last word before ; or [ is the field name
      std::string fd2 = fd;
      // strip trailing ;
      if (!fd2.empty() && fd2.back() == ';') fd2.pop_back();
      // strip array suffix
      size_t lb = fd2.find('[');
      if (lb != std::string::npos) fd2 = fd2.substr(0, lb);
      // trim
      while (!fd2.empty() && std::isspace((unsigned char)fd2.back())) fd2.pop_back();
      // split at last space
      size_t sp = fd2.rfind(' ');
      if (sp != std::string::npos) {
        std::string fname2 = fd2.substr(sp+1);
        std::string ftype2 = fd2.substr(0, sp);
        while (!ftype2.empty() && std::isspace((unsigned char)ftype2.back())) ftype2.pop_back();
        field_map[fname2] = ftype2;
      }
    }
    return "typedef struct {\n    " + join(fields, "\n    ") + "\n} " +
           struct_name + ";\n";
  }

  // -----------------------------------------------------------------------
  // scanf emit helper — BUG-E/F/G fixed
  // -----------------------------------------------------------------------
  std::string emit_scanf(const std::string &name, const std::string &vt,
                         const Token &tok) {
    if (vt == "str" || vt == "char*")
      return "fgets(" + name + ",sizeof(" + name +
             "),stdin);\n"
             "    " +
             name + "[strcspn(" + name + ",\"\\n\")]='\\0';";
    if (vt == "int")
      return "scanf(\"%d\",&" + name + ");";
    if (vt == "short")
      return "scanf(\"%hd\",&" + name + ");";
    if (vt == "long")
      return "scanf(\"%ld\",&" + name + ");";
    if (vt == "float")
      return "scanf(\"%f\",&" + name + ");";
    if (vt == "double")
      return "scanf(\"%lf\",&" + name + ");";
    if (vt == "char")
      return "scanf(\" %c\",&" + name + ");";
    
    if (vt == "bool")
      return "{int _lb_t;scanf(\"%d\",&_lb_t);" + name + "=(bool)_lb_t;}";
    if (vt == "__m256" || vt == "__m256i")
      return "fprintf(stderr,\"[LuaBase] line " + std::to_string(tok.line) +
             ": cannot scanf into __m256/__m256i '" + name + "'\\n\");exit(1);";
    return "scanf(\"%d\",&" + name + ");/*LuaBase:unknown type for " + name +
           ",defaulted %d*/";
  }

  // -----------------------------------------------------------------------
  // Expression parser
  // -----------------------------------------------------------------------
  std::string parse_expr() { return parse_ternary(); }

  std::string parse_ternary() {
    std::string c = parse_logical();
    if (current().type == TT::QUESTION) {
      advance();
      std::string t = parse_logical();
      expect(TT::COLON, false);
      std::string e = parse_logical();
      return "(" + c + "?" + t + ":" + e + ")";
    }
    return c;
  }

  std::string parse_logical() {
    std::string left = parse_bitwise_or();
    while (current().type == TT::AND || current().type == TT::OR) {
      std::string op = (advance().type == TT::AND) ? "&&" : "||";
      left = "(" + left + " " + op + " " + parse_bitwise_or() + ")";
    }
    return left;
  }

  std::string parse_bitwise_or() {
    std::string left = parse_bitwise_xor();
    while (current().type == TT::BITOR) {
      advance();
      left = "(" + left + "|" + parse_bitwise_xor() + ")";
    }
    return left;
  }

  std::string parse_bitwise_xor() {
    std::string left = parse_bitwise_and();
    while (current().type == TT::BITXOR) {
      advance();
      left = "(" + left + "^" + parse_bitwise_and() + ")";
    }
    return left;
  }

  std::string parse_bitwise_and() {
    std::string left = parse_comparison();
    while (current().type == TT::ADDRESS_OF) {
      advance();
      left = "(" + left + "&" + parse_comparison() + ")";
    }
    return left;
  }

  std::string parse_comparison() {
    std::string left = parse_shift();
    static const std::map<TT, std::string> cmp = {
        {TT::EQ, "=="}, {TT::NE, "!="}, {TT::LT, "<"},
        {TT::GT, ">"},  {TT::LE, "<="}, {TT::GE, ">="}};
    while (cmp.count(current().type)) {
      std::string op = cmp.at(advance().type);
      left = "(" + left + op + parse_shift() + ")";
    }
    return left;
  }

  std::string parse_shift() {
    std::string left = parse_additive();
    while (current().type == TT::SHL || current().type == TT::SHR) {
      std::string op = (advance().type == TT::SHL) ? "<<" : ">>";
      left = "(" + left + op + parse_additive() + ")";
    }
    return left;
  }

  std::string parse_additive() {
    std::string left = parse_multiplicative();
    while (current().type == TT::PLUS || current().type == TT::MINUS) {
      std::string op = advance().value;
      left = "(" + left + op + parse_multiplicative() + ")";
    }
    return left;
  }

  std::string parse_multiplicative() {
    std::string left = parse_unary();
    while (current().type == TT::MULTIPLY || current().type == TT::DIVIDE ||
           current().type == TT::MOD) {
      std::string op = advance().value;
      left = "(" + left + op + parse_unary() + ")";
    }
    return left;
  }

  std::string parse_unary() {
    if (current().type == TT::NOT) {
      advance();
      return "(!" + parse_unary() + ")";
    }
    if (current().type == TT::BITNOT) {
      advance();
      return "(~" + parse_unary() + ")";
    }
    if (current().type == TT::MINUS) {
      advance();
      return "(-" + parse_unary() + ")";
    }
    if (current().type == TT::MULTIPLY) {
      advance();
      return "*" + safe_name(expect(TT::IDENTIFIER, false).value);
    }
    if (current().type == TT::INCR) {
      advance();
      return "(++" + safe_name(expect(TT::IDENTIFIER, false).value) + ")";
    }
    if (current().type == TT::DECR) {
      advance();
      return "(--" + safe_name(expect(TT::IDENTIFIER, false).value) + ")";
    }
    return parse_power();
  }

  std::string parse_power() {
    std::string base = parse_primary();
    if (current().type == TT::POW) {
      advance();
      std::string exp = parse_unary();
      return "pow((double)(" + base + "),(double)(" + exp + "))";
    }
    return base;
  }

  std::string parse_primary() {
    Token &t = current();

    // F15: typeof
    if (t.type == TT::TYPEOF) {
      advance();
      expect(TT::LPAREN, false);
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::RPAREN, false);
      auto it = var_types.find(name);
      return "\"" + (it != var_types.end() ? it->second : "unknown") + "\"";
    }

    // F16: sizeof
    if (t.type == TT::SIZEOF_KW) {
      advance();
      expect(TT::LPAREN, false);
      Token &inner_tok = current();
      static const std::set<TT> type_toks = {
          TT::INT,    TT::FLOAT,   TT::STR,     TT::LONG,      TT::SHORT,
          TT::DOUBLE, TT::BOOL_KW, TT::CHAR_KW, TT::IDENTIFIER};
      if (type_toks.count(inner_tok.type)) {
        std::string n = advance().value;
        if (n == "str")
          n = "char*";
        expect(TT::RPAREN, false);
        return "sizeof(" + n + ")";
      }
      std::string inner = parse_expr();
      expect(TT::RPAREN, false);
      return "sizeof(" + inner + ")";
    }

    // F14: cast
    if (t.type == TT::CAST) {
      advance();
      expect(TT::LPAREN, false);
      std::string tn = advance().value;
      if (tn == "str")
        tn = "char*";
      expect(TT::COMMA, false);
      std::string inner = parse_expr();
      expect(TT::RPAREN, false);
      return "((" + tn + ")(" + inner + "))";
    }

    // F22: strlen
    if (t.type == TT::STRLEN_KW) {
      advance();
      expect(TT::LPAREN, false);
      std::string arg = parse_expr();
      expect(TT::RPAREN, false);
      return "(int)strlen(" + arg + ")";
    }

    // F25: exit in expr context
    if (t.type == TT::EXIT_KW) {
      advance();
      expect(TT::LPAREN, false);
      std::string code = parse_expr();
      expect(TT::RPAREN, false);
      return "(exit(" + code + "),0)";
    }

    if (t.type == TT::LPAREN) {
      advance();
      std::string inner = parse_expr();
      expect(TT::RPAREN, false);
      return "(" + inner + ")";
    }

    if (t.type == TT::ADDRESS_OF) {
      advance();
      return "&" + safe_name(expect(TT::IDENTIFIER, false).value);
    }

    Token tok = advance();
    if (tok.type == TT::NUMBER)
      return tok.value;
    if (tok.type == TT::STRING)
      return "\"" + tok.value + "\"";
    if (tok.type == TT::TRUE_KW)
      return "true";
    if (tok.type == TT::FALSE_KW)
      return "false";
    if (tok.type == TT::CHAR_LIT)
      return "'" + tok.value + "'";
    if (tok.type == TT::NULL_KW)
      return "NULL";

    if (tok.type == TT::IDENTIFIER) {
      std::string name = safe_name(tok.value);
      if (current().type == TT::LPAREN) {
        advance();
        std::vector<std::string> args;
        while (current().type != TT::RPAREN) {
          if (current().type == TT::TEOF)
            throw LuaBaseError("SyntaxError",
                               "Unterminated args in call to '" + name + "'",
                               tok.line, tok.col);
          args.push_back(parse_expr());
          if (current().type == TT::COMMA)
            advance();
        }
        expect(TT::RPAREN, false);
        return name + "(" + join(args, ", ") + ")";
      }
      while (current().type == TT::LSBRACKET || current().type == TT::DOT ||
             current().type == TT::ARROW) {
        if (current().type == TT::LSBRACKET) {
          advance();
          std::string idx = parse_expr();
          expect(TT::RSBRACKET, false);
          name = name + "[(int)(" + idx + ")]";
        } else if (current().type == TT::DOT) {
          advance();
          std::string field = expect(TT::IDENTIFIER, false).value;
          // extract base name
          std::string base = name;
          for (char &ch : base)
            if (ch == '[' || ch == '.' || ch == '-' || ch == '>')
              ch = ' ';
          std::istringstream ss(base);
          std::string first;
          ss >> first;
          std::string op = (var_types.count(first) && var_types[first] == "ptr")
                               ? "->"
                               : ".";
          name = name + op + field;
        } else {
          advance();
          std::string field = expect(TT::IDENTIFIER, false).value;
          name = name + "->" + field;
        }
      }
      if (current().type == TT::INCR) {
        advance();
        return "(" + name + "++)";
      }
      if (current().type == TT::DECR) {
        advance();
        return "(" + name + "--)";
      }
      return name;
    }

    throw LuaBaseError("SyntaxError",
                       "Unexpected token '" + tok.value + "' in expression",
                       tok.line, tok.col);
  }

  // -----------------------------------------------------------------------
  // Type Inference Engine — Rust/TypeScript-grade
  // -----------------------------------------------------------------------

  // Map a raw LuaBase type keyword to its canonical C type string
  std::string raw_to_c(const std::string &raw) const {
    if (raw == "str")      return "char*";
    if (raw == "ptr")      return "void*";
    if (raw == "bool")     return "bool";
    if (raw == "char")     return "char";
    if (raw == "u8")       return "uint8_t";
    if (raw == "u32")      return "uint32_t";
    if (raw == "u64")      return "uint64_t";
    return raw; // int, float, double, long, short, void, custom structs
  }

  // Look up a variable's TypeInfo from var_types
  TypeInfo lookup_var(const std::string &name) const {
    auto it = var_types.find(name);
    if (it == var_types.end()) return TypeInfo::unknown();
    std::string raw = it->second;
    // strip _ARRAY suffix
    bool is_arr = false;
    if (raw.size() > 6 && raw.substr(raw.size()-6) == "_ARRAY") {
      raw = raw.substr(0, raw.size()-6);
      is_arr = true;
    }
    TypeInfo ti;
    // handle "ptr X" stored as "X*" in var_types
    if (!raw.empty() && raw.back() == '*') {
      ti.base = raw.substr(0, raw.size()-1);
      ti.ptr_depth = 1;
    } else {
      ti.base = raw_to_c(raw);
      ti.ptr_depth = 0;
    }
    ti.is_array = is_arr;
    return ti;
  }

  // Look up a function's return TypeInfo
  TypeInfo lookup_func_ret(const std::string &fname) const {
    auto it = func_return_types.find(fname);
    if (it == func_return_types.end()) return TypeInfo::unknown();
    const std::string &raw = it->second;
    TypeInfo ti;
    if (!raw.empty() && raw.back() == '*') {
      ti.base = raw.substr(0, raw.size()-1);
      ti.ptr_depth = 1;
    } else {
      ti.base = raw_to_c(raw);
    }
    return ti;
  }

  // Infer TypeInfo for a NUMBER token value string
  TypeInfo infer_number_literal(const std::string &s) const {
    // hex → int (unsigned if large, but keep simple)
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
      return TypeInfo::of("int");
    // float suffix
    if (!s.empty() && (s.back() == 'f' || s.back() == 'F'))
      return TypeInfo::of("float");
    // double suffix or decimal point or exponent
    if (s.find('.') != std::string::npos ||
        s.find('e') != std::string::npos ||
        s.find('E') != std::string::npos)
      return TypeInfo::of("double");
    // long suffix
    if (!s.empty() && (s.back() == 'l' || s.back() == 'L'))
      return TypeInfo::of("long");
    return TypeInfo::of("int");
  }

  // Core inference: walk token stream starting at `start_pos` (peek, no mutation)
  // and compute the TypeInfo of the expression.
  // This is a *pure lookahead* — it does NOT advance `pos`.
  // We snapshot and restore pos internally.
  TypeInfo infer_expr_type_at(size_t start_pos) {
    size_t saved = pos;
    pos = start_pos;
    TypeInfo result = infer_ti_ternary();
    pos = saved;
    return result;
  }

  TypeInfo infer_ti_ternary() {
    TypeInfo cond = infer_ti_logical();
    if (pos < tokens.size() && tokens[pos].type == TT::QUESTION) {
      pos++;
      TypeInfo t = infer_ti_logical();
      if (pos < tokens.size() && tokens[pos].type == TT::COLON) pos++;
      TypeInfo e = infer_ti_logical();
      // ternary: promote the two branches (like Rust/TS — both arms must unify)
      return promote(t, e);
    }
    return cond;
  }

  TypeInfo infer_ti_logical() {
    TypeInfo left = infer_ti_bitwise_or();
    while (pos < tokens.size() &&
           (tokens[pos].type == TT::AND || tokens[pos].type == TT::OR)) {
      pos++;
      infer_ti_bitwise_or(); // consume, discard — result is always bool
      left = TypeInfo::of("bool");
    }
    return left;
  }

  TypeInfo infer_ti_bitwise_or() {
    TypeInfo left = infer_ti_bitwise_xor();
    while (pos < tokens.size() && tokens[pos].type == TT::BITOR) {
      pos++;
      TypeInfo right = infer_ti_bitwise_xor();
      left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_bitwise_xor() {
    TypeInfo left = infer_ti_bitwise_and();
    while (pos < tokens.size() && tokens[pos].type == TT::BITXOR) {
      pos++;
      TypeInfo right = infer_ti_bitwise_and();
      left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_bitwise_and() {
    TypeInfo left = infer_ti_comparison();
    while (pos < tokens.size() && tokens[pos].type == TT::ADDRESS_OF) {
      pos++;
      TypeInfo right = infer_ti_comparison();
      left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_comparison() {
    TypeInfo left = infer_ti_shift();
    static const std::set<TT> cmp_ops = {
      TT::EQ, TT::NE, TT::LT, TT::GT, TT::LE, TT::GE};
    while (pos < tokens.size() && cmp_ops.count(tokens[pos].type)) {
      pos++;
      infer_ti_shift(); // consume RHS — comparison always yields bool
      left = TypeInfo::of("bool");
    }
    return left;
  }

  TypeInfo infer_ti_shift() {
    TypeInfo left = infer_ti_additive();
    while (pos < tokens.size() &&
           (tokens[pos].type == TT::SHL || tokens[pos].type == TT::SHR)) {
      pos++;
      TypeInfo right = infer_ti_additive();
      left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_additive() {
    TypeInfo left = infer_ti_multiplicative();
    while (pos < tokens.size() &&
           (tokens[pos].type == TT::PLUS || tokens[pos].type == TT::MINUS)) {
      pos++;
      TypeInfo right = infer_ti_multiplicative();
      // pointer arithmetic: ptr + int → ptr (C semantics)
      if (left.is_ptr() && right.is_integer())  { /* left stays ptr */ }
      else if (right.is_ptr() && left.is_integer()) left = right;
      // ptr - ptr → ptrdiff_t (treat as long)
      else if (left.is_ptr() && right.is_ptr()) left = TypeInfo::of("long");
      else left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_multiplicative() {
    TypeInfo left = infer_ti_unary();
    while (pos < tokens.size() &&
           (tokens[pos].type == TT::MULTIPLY ||
            tokens[pos].type == TT::DIVIDE ||
            tokens[pos].type == TT::MOD)) {
      pos++;
      TypeInfo right = infer_ti_unary();
      left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_unary() {
    if (pos >= tokens.size()) return TypeInfo::unknown();
    TT tt = tokens[pos].type;

    // logical NOT → bool
    if (tt == TT::NOT) { pos++; infer_ti_unary(); return TypeInfo::of("bool"); }
    // bitwise NOT → same type as operand
    if (tt == TT::BITNOT) { pos++; return infer_ti_unary(); }
    // unary minus → promote to at least int
    if (tt == TT::MINUS) { pos++; TypeInfo inner = infer_ti_unary(); return inner.is_float() ? inner : promote(inner, TypeInfo::of("int")); }
    // dereference *p → strip one pointer level
    if (tt == TT::MULTIPLY) {
      pos++;
      if (pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER) {
        std::string n = safe_name(tokens[pos++].value);
        TypeInfo ti = lookup_var(n);
        if (ti.ptr_depth > 0) { ti.ptr_depth--; return ti; }
        if (!ti.base.empty() && ti.base.back() == '*') {
          ti.base.pop_back(); return ti;
        }
        return TypeInfo::of("int");
      }
      return TypeInfo::unknown();
    }
    // prefix ++ / -- → same type as operand
    if (tt == TT::INCR || tt == TT::DECR) {
      pos++;
      if (pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER) {
        std::string n = safe_name(tokens[pos++].value);
        return lookup_var(n);
      }
      return TypeInfo::of("int");
    }
    return infer_ti_power();
  }

  TypeInfo infer_ti_power() {
    TypeInfo base = infer_ti_primary();
    if (pos < tokens.size() && tokens[pos].type == TT::POW) {
      pos++;
      infer_ti_unary();
      return TypeInfo::of("double"); // pow() always returns double
    }
    return base;
  }

  // Skip over a balanced paren/bracket group without interpreting
  void skip_balanced(TT open, TT close) {
    if (pos < tokens.size() && tokens[pos].type == open) {
      pos++;
      int depth = 1;
      while (pos < tokens.size() && depth > 0) {
        if (tokens[pos].type == open) depth++;
        else if (tokens[pos].type == close) depth--;
        pos++;
      }
    }
  }

  TypeInfo infer_ti_primary() {
    if (pos >= tokens.size()) return TypeInfo::unknown();
    Token &t = tokens[pos];

    // typeof(x) → char* (it returns a string)
    if (t.type == TT::TYPEOF) {
      pos++;
      skip_balanced(TT::LPAREN, TT::RPAREN);
      return TypeInfo::of("char*");
    }

    // sizeof(...) → size_t (use unsigned long / int)
    if (t.type == TT::SIZEOF_KW) {
      pos++;
      skip_balanced(TT::LPAREN, TT::RPAREN);
      return TypeInfo::of("long");
    }

    // cast(T, expr) → the cast target type
    if (t.type == TT::CAST) {
      pos++;
      if (pos < tokens.size() && tokens[pos].type == TT::LPAREN) {
        pos++;
        std::string tn;
        if (pos < tokens.size()) tn = raw_to_c(tokens[pos++].value);
        // skip comma + expr + rparen
        if (pos < tokens.size() && tokens[pos].type == TT::COMMA) pos++;
        infer_ti_ternary(); // consume inner expr
        if (pos < tokens.size() && tokens[pos].type == TT::RPAREN) pos++;
        // build TypeInfo from tn
        TypeInfo ti;
        if (!tn.empty() && tn.back() == '*') {
          ti.base = tn.substr(0, tn.size()-1); ti.ptr_depth = 1;
        } else { ti.base = tn; }
        return ti;
      }
      return TypeInfo::unknown();
    }

    // strlen → int
    if (t.type == TT::STRLEN_KW) {
      pos++;
      skip_balanced(TT::LPAREN, TT::RPAREN);
      return TypeInfo::of("int");
    }

    // exit → int (expression form returns 0)
    if (t.type == TT::EXIT_KW) {
      pos++;
      skip_balanced(TT::LPAREN, TT::RPAREN);
      return TypeInfo::of("int");
    }

    // parenthesised expression — propagate inner type
    if (t.type == TT::LPAREN) {
      pos++;
      TypeInfo inner = infer_ti_ternary();
      if (pos < tokens.size() && tokens[pos].type == TT::RPAREN) pos++;
      return inner;
    }

    // address-of &x → pointer to x's type
    if (t.type == TT::ADDRESS_OF) {
      pos++;
      if (pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER) {
        std::string n = safe_name(tokens[pos++].value);
        TypeInfo inner = lookup_var(n);
        inner.ptr_depth++;
        return inner;
      }
      return TypeInfo::of("void*");
    }

    Token tok = tokens[pos++];

    // Literals
    if (tok.type == TT::NUMBER)   return infer_number_literal(tok.value);
    if (tok.type == TT::STRING)   return TypeInfo::of("char*");
    if (tok.type == TT::CHAR_LIT) return TypeInfo::of("char");
    if (tok.type == TT::TRUE_KW || tok.type == TT::FALSE_KW)
      return TypeInfo::of("bool");
    if (tok.type == TT::NULL_KW)  return TypeInfo::of("void*");

    if (tok.type == TT::IDENTIFIER) {
      std::string name = safe_name(tok.value);

      // Function call: name(...)
      if (pos < tokens.size() && tokens[pos].type == TT::LPAREN) {
        skip_balanced(TT::LPAREN, TT::RPAREN);
        return lookup_func_ret(name);
      }

      // Base type from var_types
      TypeInfo ti = lookup_var(name);
      std::string struct_base = ti.base; // save for field lookup

      // Postfix chains: [idx], .field, ->field
      while (pos < tokens.size() &&
             (tokens[pos].type == TT::LSBRACKET ||
              tokens[pos].type == TT::DOT ||
              tokens[pos].type == TT::ARROW)) {
        if (tokens[pos].type == TT::LSBRACKET) {
          // array subscript → element type (strip one array level)
          pos++;
          infer_ti_ternary(); // skip index expr
          if (pos < tokens.size() && tokens[pos].type == TT::RSBRACKET) pos++;
          // element type: if we know it was an array, strip _ARRAY tag
          // The element type is the same as the base type stored
          ti.is_array = false;
        } else {
          // .field or ->field
          bool is_arrow = (tokens[pos].type == TT::ARROW);
          pos++;
          if (pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER) {
            std::string field = tokens[pos++].value;
            // Try struct_field_types for precise field type
            auto sit = struct_field_types.find(struct_base);
            if (sit != struct_field_types.end()) {
              auto fit2 = sit->second.find(field);
              if (fit2 != sit->second.end()) {
                std::string ft = fit2->second;
                TypeInfo fti;
                if (!ft.empty() && ft.back() == '*') {
                  fti.base = ft.substr(0, ft.size()-1); fti.ptr_depth = 1;
                } else { fti.base = ft; }
                ti = fti;
                struct_base = ti.base;
                continue;
              }
            }
            ti = TypeInfo::unknown();
          }
        }
      }

      // Postfix ++ / --  → same type
      if (pos < tokens.size() &&
          (tokens[pos].type == TT::INCR || tokens[pos].type == TT::DECR))
        pos++;

      return ti;
    }

    return TypeInfo::unknown();
  }

  // Public entry point: infer the type of the expression whose first token
  // is at `start_pos` in the token stream. Does NOT consume tokens.
  TypeInfo infer_type_at(size_t start_pos) {
    return infer_expr_type_at(start_pos);
  }

  // Legacy compatibility shim used by the let/var handler
  // (kept so the rest of the code compiles unchanged)
  std::string infer_type_from_rhs(const Token &/*expr_tok*/, const std::string &/*expr_str*/,
                                   size_t rhs_start_pos) {
    TypeInfo ti = infer_type_at(rhs_start_pos);
    return ti.c_type();
  }

  // -----------------------------------------------------------------------
  // Statement parser
  // -----------------------------------------------------------------------
  std::string parse_statement() {
    while (current().type == TT::SEMICOLON)
      advance();
    Token &t = current();

    if (t.type == TT::TYPE)
      return parse_type_definition();
    if (t.type == TT::ENUM_KW)
      return parse_enum();

    // HLT
    if (t.type == TT::HLT) {
      advance();
      expect(TT::LPAREN, false);
      std::string ms = parse_expr();
      expect(TT::RPAREN, false);
      return "usleep((" + ms + ")*1000);";
    }

    // throw
    if (t.type == TT::THROW_KW) {
      advance();
      return "_lb_throw(TO_STR(" + parse_expr() + "));";
    }

    // try/except
    if (t.type == TT::TRY_KW) {
      Token try_t = advance();
      expect(TT::LBRACE, false);
      std::vector<std::string> try_body;
      while (current().type != TT::RBRACE && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          try_body.push_back(line_directive(tok2) + "        " + s);
      }
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError", "Unterminated 'try' block",
                           try_t.line, try_t.col);
      expect(TT::RBRACE, false);
      if (current().type != TT::EXCEPT_KW)
        throw LuaBaseError("SyntaxError", "'try' must be followed by 'except'",
                           try_t.line, try_t.col);
      advance();
      expect(TT::LPAREN, false);
      advance(); // exc type (str etc)
      std::string exc_var = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::RPAREN, false);
      expect(TT::LBRACE, false);
      std::vector<std::string> exc_body;
      while (current().type != TT::RBRACE && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          exc_body.push_back(line_directive(tok2) + "        " + s);
      }
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError", "Unterminated 'except' block",
                           try_t.line, try_t.col);
      expect(TT::RBRACE, false);
      int uid =
          std::abs((int)std::hash<std::string>{}(
              std::to_string(try_t.line) + ":" + std::to_string(try_t.col))) %
          99999;
      std::string tb = join(try_body, "\n");
      std::string eb = join(exc_body, "\n");
      return "{ /* try */\n"
             "    static jmp_buf _lb_jmp_" +
             std::to_string(uid) +
             ";\n"
             "    static char _lb_exc_msg_" +
             std::to_string(uid) +
             "[512];\n"
             "    _lb_exc_active = &_lb_jmp_" +
             std::to_string(uid) +
             ";\n"
             "    _lb_exc_msg    = _lb_exc_msg_" +
             std::to_string(uid) +
             ";\n"
             "    if(!setjmp(_lb_jmp_" +
             std::to_string(uid) + ")) {\n" + tb +
             "\n"
             "    } else { /* except */\n"
             "    char " +
             exc_var + "[512]; snprintf(" + exc_var + ",sizeof(" + exc_var +
             "),\"%s\",_lb_exc_msg_" + std::to_string(uid) + ");\n" + eb +
             "\n"
             "    }\n"
             "    _lb_exc_active = NULL;\n"
             "}";
    }

    // GLOBAL
    if (t.type == TT::GLOBAL_KW) {
      advance();
      std::string decl = parse_statement();
      if (!decl.empty())
        headers.push_back(decl + "\n");
      return "";
    }

    // fopen
    if (t.type == TT::FOPEN) {
      advance();
      expect(TT::LPAREN, false);
      std::string hname = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::COMMA, false);
      std::string fname = parse_expr();
      expect(TT::COMMA, false);
      std::string mode = parse_expr();
      expect(TT::RPAREN, false);
      std::string decl = _handle_declared.count(hname) ? "" : "FILE* ";
      _handle_declared.insert(hname);
      return decl + hname + " = fopen(" + fname + "," + mode +
             ");\n"
             "    if(" +
             hname + "==NULL){fprintf(stderr,\"FAIL: %s\\n\"," + fname +
             ");exit(1);}";
    }

    // fwrite
    if (t.type == TT::FWRITE) {
      advance();
      expect(TT::LPAREN, false);
      std::string hname = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::COMMA, false);
      Token ct = current();
      std::string content = parse_expr();
      expect(TT::RPAREN, false);
      std::string vt;
      if (ct.type == TT::IDENTIFIER) {
        auto it = var_types.find(safe_name(ct.value));
        vt = (it != var_types.end()) ? it->second : "";
      }
      if ((vt == "str" || vt == "char*") ||
          (vt.empty() && ct.type == TT::STRING))
        return "if(" + hname + "){fprintf(" + hname + ",\"%s\"," + content +
               ");fflush(" + hname + ");}";
      if (vt == "int" || vt == "short")
        return "if(" + hname + "){fprintf(" + hname + ",\"%d\"," + content +
               ");fflush(" + hname + ");}";
      if (vt == "char")
        return "if(" + hname + "){fprintf(" + hname + ",\"%c\"," + content +
               ");fflush(" + hname + ");}";
      if (vt == "long")
        return "if(" + hname + "){fprintf(" + hname + ",\"%ld\"," + content +
               ");fflush(" + hname + ");}";
      if (vt == "double")
        return "if(" + hname + "){fprintf(" + hname + ",\"%.10g\"," + content +
               ");fflush(" + hname + ");}";
      if (vt == "float")
        return "if(" + hname + "){fprintf(" + hname + ",\"%g\"," + content +
               ");fflush(" + hname + ");}";
      if (vt == "__m256" || vt == "__m256i")
        return "if(" + hname + "){fprintf(" + hname + ",\"%s\",TO_STR(" +
               content + "));fflush(" + hname + ");}";
      return "if(" + hname + "){fprintf(" + hname + ",\"%s\",TO_STR(" +
             content + "));fflush(" + hname + ");}";
    }

    // fclose
    if (t.type == TT::FCLOSE) {
      advance();
      expect(TT::LPAREN, false);
      std::string hname = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::RPAREN, false);
      return "if(" + hname + "){fclose(" + hname + ");" + hname + "=NULL;}";
    }

    // fread
    if (t.type == TT::FREAD) {
      advance();
      expect(TT::LPAREN, false);
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::RPAREN, false);
      return "if(handle)fread(&" + name + ",sizeof(" + name + "),1,handle);";
    }

    // exit
    if (t.type == TT::EXIT_KW) {
      advance();
      expect(TT::LPAREN, false);
      std::string code = parse_expr();
      expect(TT::RPAREN, false);
      return "exit(" + code + ");";
    }

    // define
    if (t.type == TT::DEFINE_KW) {
      advance();
      std::string dname = expect(TT::IDENTIFIER, false).value;
      std::string dval = parse_expr();
      headers.push_back("#define " + dname + " " + dval + "\n");
      return "";
    }

    // inline function
    if (t.type == TT::INLINE_KW) {
      advance();
      if (current().type != TT::FUNCTION)
        throw LuaBaseError("SyntaxError",
                           "'inline' must be followed by 'function'", t.line,
                           t.col);
      advance();
      return parse_function_body(t, true);
    }

    // const
    if (t.type == TT::CONST_KW) {
      advance();
      std::string inner_raw = advance().value;
      std::string inner_c = (inner_raw == "str") ? "char*" : inner_raw;
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::ASSIGN, false);
      std::string val = parse_expr();
      var_types[name] = inner_raw;
      return "const " + inner_c + " " + name + " = " + val + ";";
    }

    // let / var — type inference (same semantics, mutable by default)
    if (t.type == TT::LET_KW || t.type == TT::VAR_KW) {
      advance();
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::ASSIGN, false);
      size_t rhs_start = pos;          // capture RHS start BEFORE parse_expr consumes it
      Token expr_start = current();
      std::string val = parse_expr();

      // Full Rust/TS-style type inference: walk the token stream at rhs_start
      std::string inferred_type = infer_type_from_rhs(expr_start, val, rhs_start);

      // Store normalised form in var_types for downstream inference
      // (strip trailing * for the raw form, keep ptr_depth info via c_type)
      var_types[name] = inferred_type;
      return inferred_type + " " + name + " = " + val + ";";
    }

    // Type declarations (var decl)
    bool is_custom =
        (t.type == TT::IDENTIFIER &&
         (var_types.count(t.value) && var_types[t.value] == "STRUCT" ||
          _enum_names.count(t.value)));
    static const std::set<TT> type_decl_toks = {
        TT::INT,     TT::FLOAT,   TT::STR,  TT::PTR,  TT::LONG,
        TT::SHORT,   TT::DOUBLE,  TT::VOID, TT::M256, TT::M256I,
        TT::BOOL_KW, TT::CHAR_KW, TT::U8,   TT::U32,  TT::U64};
        
    if (type_decl_toks.count(t.type) || is_custom) {
      std::string vtype_raw = advance().value;
      std::string vtype;
      static const std::map<std::string, std::string> umap = {
          {"u8", "uint8_t"}, {"u32", "uint32_t"}, {"u64", "uint64_t"}};
      if (umap.count(vtype_raw)) {
        vtype = umap.at(vtype_raw);
        if (!has_header("#include <stdint.h>"))
          headers[0] = "#include <stdint.h>\n" + headers[0];
      } else if (vtype_raw == "ptr") {
        std::string inner = advance().value;
        if (inner == "str")
          inner = "char*";
        vtype = inner + "*";
      } else if (vtype_raw == "bool") {
        vtype = "bool";
      } else if (vtype_raw == "char") {
        vtype = "char";
      } else {
        vtype = (vtype_raw == "str") ? "char*" : vtype_raw;
      }

      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      var_types[name] = vtype_raw;

      // array
      if (current().type == TT::LSBRACKET) {
        advance();
        // accept NUMBER or IDENTIFIER (e.g. #define'd macro)
        std::string size;
        if (current().type == TT::NUMBER || current().type == TT::IDENTIFIER)
          size = advance().value;
        else
          size = expect(TT::NUMBER, false).value; // triggers proper error
        expect(TT::RSBRACKET, false);
        var_types[name] = vtype_raw + "_ARRAY";
        std::string val;
        if (current().type == TT::ASSIGN) {
          advance();
          expect(TT::LBRACE, false);
          std::vector<std::string> items;
          while (current().type != TT::RBRACE) {
            items.push_back(parse_expr());
            if (current().type == TT::COMMA)
              advance();
          }
          expect(TT::RBRACE, false);
          val = " = {" + join(items, ", ") + "}";
        }
        return vtype + " " + name + "[" + size + "]" + val + ";";
      }
      // with init
      if (current().type == TT::ASSIGN) {
        advance();
        return vtype + " " + name + " = " + parse_expr() + ";";
      }
      // no init — BUG-M FIXED
      if (vtype_raw == "str")
        return "char " + name + "[256];";
      if (vtype_raw == "char")
        return "char " + name + " = '\\0';";
      if (vtype_raw == "bool")
        return "bool " + name + " = false;";
      
      if (umap.count(vtype_raw))
        return vtype + " " + name + " = 0;";
      return vtype + " " + name + ";";
    }

    // print
    if (t.type == TT::PRINT) {
      advance();
      expect(TT::LPAREN, false);
      std::vector<std::string> exprs;
      while (current().type != TT::RPAREN) {
        if (current().type == TT::TEOF)
          throw LuaBaseError("SyntaxError", "Unterminated 'print'", t.line,
                             t.col);
        exprs.push_back(parse_expr());
        if (current().type == TT::COMMA)
          advance();
      }
      expect(TT::RPAREN, false);
      std::string r;
      for (size_t i = 0; i < exprs.size(); i++) {
        if (i)
          r += "; ";
        r += "printf(\"%s\",TO_STR(" + exprs[i] + "))";
      }
      return r + ";";
    }

    // println
    if (t.type == TT::PRINTLN) {
      advance();
      expect(TT::LPAREN, false);
      std::vector<std::string> exprs;
      while (current().type != TT::RPAREN) {
        if (current().type == TT::TEOF)
          throw LuaBaseError("SyntaxError", "Unterminated 'println'", t.line,
                             t.col);
        exprs.push_back(parse_expr());
        if (current().type == TT::COMMA)
          advance();
      }
      expect(TT::RPAREN, false);
      std::string r;
      for (size_t i = 0; i < exprs.size(); i++) {
        if (i)
          r += "; ";
        r += "printf(\"%s\",TO_STR(" + exprs[i] + "))";
      }
      return r + "; printf(\"\\n\");";
    }

    // printfmt
    if (t.type == TT::PRINTFMT) {
      advance();
      expect(TT::LPAREN, false);
      std::vector<std::string> args;
      while (current().type != TT::RPAREN) {
        if (current().type == TT::TEOF)
          throw LuaBaseError("SyntaxError", "Unterminated 'printfmt'", t.line,
                             t.col);
        args.push_back(parse_expr());
        if (current().type == TT::COMMA)
          advance();
      }
      expect(TT::RPAREN, false);
      return "printf(" + join(args, ", ") + ");";
    }

    // assert
    if (t.type == TT::ASSERT) {
      advance();
      expect(TT::LPAREN, false);
      std::string cond = parse_expr();
      expect(TT::RPAREN, false);
      return "do{if(!(" + cond +
             "))"
             "{fprintf(stderr,\"[assert] FAILED line " +
             std::to_string(t.line) + ": assert(" + cond +
             ")\\n\");exit(1);}}"
             "while(0);";
    }

    // if
    if (t.type == TT::IF) {
      advance();
      std::string cond = parse_expr();
      expect(TT::THEN, false);
      std::vector<std::string> body;
      while (current().type != TT::ELSEIF && current().type != TT::ELSE &&
             current().type != TT::END && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          body.push_back(line_directive(tok2) + "    " + s);
      }
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError", "Unterminated 'if'", t.line, t.col);

      std::vector<std::string> elseif_parts;
      while (current().type == TT::ELSEIF) {
        Token ei = advance();
        std::string ce = parse_expr();
        expect(TT::THEN, false);
        std::vector<std::string> be;
        while (current().type != TT::ELSEIF && current().type != TT::ELSE &&
               current().type != TT::END && current().type != TT::TEOF) {
          Token tok2 = current();
          std::string s = parse_statement();
          if (!s.empty())
            be.push_back(line_directive(tok2) + "    " + s);
        }
        if (current().type == TT::TEOF)
          throw LuaBaseError("SyntaxError", "Unterminated 'elseif'", ei.line,
                             ei.col);
        elseif_parts.push_back("} else if (" + ce + ") {\n" + join(be, "\n"));
      }

      std::string else_part;
      if (current().type == TT::ELSE) {
        advance();
        std::vector<std::string> eb;
        while (current().type != TT::END && current().type != TT::TEOF) {
          Token tok2 = current();
          std::string s = parse_statement();
          if (!s.empty())
            eb.push_back(line_directive(tok2) + "    " + s);
        }
        else_part = " else {\n" + join(eb, "\n") + "\n}";
      }
      expect(TT::END, false);

      std::string res = "if (" + cond + ") {\n" + join(body, "\n") + "\n";
      if (!elseif_parts.empty())
        res += join(elseif_parts, "\n") + "\n";
      if (!else_part.empty())
        return res + "}" + else_part;
      if (!elseif_parts.empty())
        return res + "}";
      return res + "}";
    }

    // while
    if (t.type == TT::WHILE) {
      advance();
      std::string cond = parse_expr();
      expect(TT::DO, false);
      std::vector<std::string> body;
      while (current().type != TT::END && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          body.push_back(line_directive(tok2) + s);
      }
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError", "Unterminated 'while'", t.line,
                           t.col);
      expect(TT::END, false);
      return "while (" + cond + ") {\n    " + join(body, "    \n") + "\n}";
    }

    // for
    if (t.type == TT::FOR) {
      advance();
      std::string var = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::ASSIGN, false);
      std::string start = parse_expr();
      expect(TT::COMMA, false);
      std::string limit = parse_expr();
      std::string step = "1";
      if (current().type == TT::COMMA) {
        advance();
        step = parse_expr();
      }
      expect(TT::DO, false);
      std::vector<std::string> body;
      while (current().type != TT::END && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          body.push_back(line_directive(tok2) + s);
      }
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError", "Unterminated 'for'", t.line, t.col);
      expect(TT::END, false);
      var_types[var] = "int";
      std::string inner = body.empty() ? "" : join(body, "\n    ");
      return "for(int " + var + "=(" + start +
             ");"
             "(" +
             step + ")>=0?" + var + "<=(" + limit + "):" + var + ">=(" + limit +
             ");" + var + "+=(" + step +
             "))"
             "{\n    " +
             inner + "\n}";
    }

    if (t.type == TT::BREAK) {
      advance();
      return "break;";
    }
    if (t.type == TT::CONTINUE) {
      advance();
      return "continue;";
    }

    // switch
    if (t.type == TT::SWITCH) {
      advance();
      std::string expr = parse_expr();
      expect(TT::LBRACE, false);
      std::vector<std::string> cases;
      std::vector<std::string> default_stmts;
      while (current().type != TT::RBRACE && current().type != TT::TEOF) {
        if (current().type == TT::CASE) {
          advance();
          std::string val = parse_expr();
          expect(TT::COLON, false);
          std::vector<std::string> stmts;
          while (current().type != TT::CASE &&
                 current().type != TT::DEFAULT_KW &&
                 current().type != TT::RBRACE && current().type != TT::TEOF) {
            std::string s = parse_statement();
            if (!s.empty())
              stmts.push_back("    " + s);
          }
          cases.push_back("case " + val + ":\n" + join(stmts, "\n") +
                          "\n    break;");
        } else if (current().type == TT::DEFAULT_KW) {
          advance();
          if (current().type == TT::COLON)
            advance();
          while (current().type != TT::RBRACE && current().type != TT::TEOF) {
            std::string s = parse_statement();
            if (!s.empty())
              default_stmts.push_back("    " + s);
          }
        } else {
          advance();
        }
      }
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError", "Unterminated 'switch'", t.line,
                           t.col);
      expect(TT::RBRACE, false);
      std::string body = join(cases, "\n");
      if (!default_stmts.empty())
        body += "\ndefault:\n" + join(default_stmts, "\n") + "\n    break;";
      return "switch(" + expr + "){\n" + body + "\n}";
    }

    // strcpy / strcat
    if (t.type == TT::STRCPY_KW) {
      advance();
      expect(TT::LPAREN, false);
      std::string dst = parse_expr();
      expect(TT::COMMA, false);
      std::string src = parse_expr();
      expect(TT::RPAREN, false);
      return "strcpy(" + dst + "," + src + ");";
    }
    if (t.type == TT::STRCAT_KW) {
      advance();
      expect(TT::LPAREN, false);
      std::string dst = parse_expr();
      expect(TT::COMMA, false);
      std::string src = parse_expr();
      expect(TT::RPAREN, false);
      return "strcat(" + dst + "," + src + ");";
    }

    // memset
    if (t.type == TT::MEMSET_KW) {
      advance();
      expect(TT::LPAREN, false);
      std::string ptr = parse_expr();
      expect(TT::COMMA, false);
      std::string val = parse_expr();
      expect(TT::COMMA, false);
      std::string n = parse_expr();
      expect(TT::RPAREN, false);
      return "memset(" + ptr + "," + val + "," + n + ");";
    }

    // pointer deref assign  *name = expr
    if (t.type == TT::MULTIPLY) {
      advance();
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::ASSIGN, false);
      return "*" + name + " = " + parse_expr() + ";";
    }

    // identifier: assign, compound assign, ++/--, call
    if (t.type == TT::IDENTIFIER) {
      std::string name = safe_name(advance().value);
      while (current().type == TT::LSBRACKET || current().type == TT::DOT ||
             current().type == TT::ARROW) {
        if (current().type == TT::LSBRACKET) {
          advance();
          std::string idx = parse_expr();
          expect(TT::RSBRACKET, false);
          name = name + "[(int)(" + idx + ")]";
        } else if (current().type == TT::DOT) {
          advance();
          std::string field = expect(TT::IDENTIFIER, false).value;
          std::string base = name;
          for (char &ch : base)
            if (ch == '[' || ch == '.' || ch == '-' || ch == '>')
              ch = ' ';
          std::istringstream ss(base);
          std::string first;
          ss >> first;
          std::string op = (var_types.count(first) && var_types[first] == "ptr")
                               ? "->"
                               : ".";
          name = name + op + field;
        } else {
          advance();
          name = name + "->" + expect(TT::IDENTIFIER, false).value;
        }
      }
      if (current().type == TT::ASSIGN) {
        advance();
        return name + " = " + parse_expr() + ";";
      }
      static const std::map<TT, std::string> compound = {
          {TT::PLUS_ASSIGN, "+="}, {TT::MINUS_ASSIGN, "-="},
          {TT::MUL_ASSIGN, "*="},  {TT::DIV_ASSIGN, "/="},
          {TT::MOD_ASSIGN, "%="},
      };
      if (compound.count(current().type)) {
        std::string cop = compound.at(advance().type);
        return name + " " + cop + " " + parse_expr() + ";";
      }
      if (current().type == TT::INCR) {
        advance();
        return name + "++;";
      }
      if (current().type == TT::DECR) {
        advance();
        return name + "--;";
      }
      if (current().type == TT::LPAREN) {
        advance();
        std::vector<std::string> args;
        while (current().type != TT::RPAREN) {
          args.push_back(parse_expr());
          if (current().type == TT::COMMA)
            advance();
        }
        expect(TT::RPAREN, false);
        return name + "(" + join(args, ", ") + ");";
      }
      return "";
    }

    // scanf
    if (t.type == TT::SCANF) {
      advance();
      expect(TT::LPAREN, false);
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::RPAREN, false);
      auto it = var_types.find(name);
      std::string vt = (it != var_types.end()) ? it->second : "";
      return emit_scanf(name, vt, t);
    }

    // return
    if (t.type == TT::RETURN) {
      advance();
      if (current().type == TT::END || current().type == TT::RBRACE ||
          current().type == TT::TEOF || current().type == TT::SEMICOLON)
        return "return;";
      return "return " + parse_expr() + ";";
    }

    // prefix ++ / --
    if (t.type == TT::INCR) {
      advance();
      return "++" + safe_name(expect(TT::IDENTIFIER, false).value) + ";";
    }
    if (t.type == TT::DECR) {
      advance();
      return "--" + safe_name(expect(TT::IDENTIFIER, false).value) + ";";
    }

    advance();
    return "";
  }

  // -----------------------------------------------------------------------
  // Function parsing
  // -----------------------------------------------------------------------
  std::string parse_function_body(const Token &fn_tok, bool inl) {
    static const std::set<TT> valid_ret = {
        TT::INT,     TT::FLOAT,   TT::STR,  TT::LONG,  TT::SHORT,
        TT::DOUBLE,  TT::VOID,    TT::M256, TT::M256I, TT::IDENTIFIER,
        TT::BOOL_KW, TT::CHAR_KW, TT::PTR,
    };
    Token &rt = current();
    if (!valid_ret.count(rt.type))
      throw LuaBaseError("SyntaxError",
                         "Expected return type after 'function', got '" +
                             rt.value + "'",
                         rt.line, rt.col);
    std::string raw_ret = advance().value;
    std::string ret_type;
    if (raw_ret == "ptr") {
      std::string inner = advance().value;
      if (inner == "str")
        inner = "char*";
      ret_type = inner + "*";
    } else if (raw_ret == "str")
      ret_type = "char*";
    else if (raw_ret == "bool")
      ret_type = "bool";
    else if (raw_ret == "char")
      ret_type = "char";
    else
      ret_type = raw_ret;
    if (inl)
      ret_type = "static inline " + ret_type;

    std::string fname = safe_name(expect(TT::IDENTIFIER, false).value);
    // register for type inference on call sites
    func_return_types[fname] = raw_ret;
    expect(TT::LPAREN, false);

    auto saved_var_types = var_types;
    std::vector<std::string> params;
    static const std::set<TT> valid_p = {
        TT::INT,        TT::FLOAT,   TT::STR,    TT::LONG, TT::SHORT,
        TT::DOUBLE,     TT::VOID,    TT::PTR,    TT::M256, TT::M256I,
        TT::IDENTIFIER, TT::BOOL_KW, TT::CHAR_KW};
    while (current().type != TT::RPAREN) {
      if (current().type == TT::TEOF)
        throw LuaBaseError("SyntaxError",
                           "Unterminated params in function '" + fname + "'",
                           fn_tok.line, fn_tok.col);
      Token pt = current();
      if (!valid_p.count(pt.type))
        throw LuaBaseError("SyntaxError",
                           "Expected param type in '" + fname + "', got '" +
                               pt.value + "'",
                           pt.line, pt.col);
      std::string p_type_raw = advance().value;
      std::string p_type;
      if (p_type_raw == "ptr") {
        std::string inner = advance().value;
        if (inner == "str")
          inner = "char*";
        p_type = inner + "*";
      } else if (p_type_raw == "str")
        p_type = "char*";
      else if (p_type_raw == "bool")
        p_type = "bool";
      else if (p_type_raw == "char")
        p_type = "char";
      else
        p_type = p_type_raw;
      std::string p_name = safe_name(expect(TT::IDENTIFIER, false).value);
      var_types[p_name] = p_type_raw;
      params.push_back(p_type + " " + p_name);
      if (current().type == TT::COMMA)
        advance();
    }
    expect(TT::RPAREN, false);
    // Register param types for call-site argument inference
    {
      std::vector<std::string> ptypes;
      for (const auto &p : params) {
        // p is "type name" — extract type (everything before last space)
        size_t sp = p.rfind(' ');
        if (sp != std::string::npos)
          ptypes.push_back(p.substr(0, sp));
        else
          ptypes.push_back("int");
      }
      func_param_types[fname] = ptypes;
    }
    expect(TT::LBRACE, false);

    std::vector<std::string> body;
    while (current().type != TT::RBRACE && current().type != TT::TEOF) {
      Token tok2 = current();
      std::string s = parse_statement();
      if (!s.empty())
        body.push_back(line_directive(tok2) + "    " + s);
    }
    if (current().type == TT::TEOF) {
      var_types = saved_var_types;
      throw LuaBaseError("SyntaxError", "Unterminated function '" + fname + "'",
                         fn_tok.line, fn_tok.col);
    }
    expect(TT::RBRACE, false);
    var_types = saved_var_types;
    return ret_type + " " + fname + "(" + join(params, ", ") + ") {\n" +
           join(body, "\n") + "\n}\n";
  }

  // -----------------------------------------------------------------------
  // Scan a .h file for function signatures via clang -ast-dump=json
  // Extracts every FunctionDecl: name + return type, no hardcoding.
  // Falls back silently if clang not available or file unreadable.
  // -----------------------------------------------------------------------
  void scan_h_for_funcs(const std::string &path) {
    if (!fs::exists(path)) return;

    // build clang command — dump AST as JSON, suppress system header noise
    std::string cmd = "clang -Xclang -ast-dump=json -fsyntax-only "
                      "-fno-color-diagnostics \"" + path + "\" 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return;

    // read all output
    std::string json;
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe))
      json += buf;
    pclose(pipe);

    if (json.empty()) return;

    // -------------------------------------------------------------------
    // Minimal JSON walk — no external lib needed.
    // We look for the pattern:
    //   "kind": "FunctionDecl"   (anywhere in an object)
    //   "name": "<fname>"        (in same object)
    //   "type": { "qualType": "<rettype> (<params>)" }
    //
    // Strategy: scan for "kind":"FunctionDecl", then within the same
    // brace-scope grab "name" and "qualType".
    // -------------------------------------------------------------------
    auto json_str_val = [&](const std::string &src, size_t from,
                             const std::string &key) -> std::string {
      // find "key" : "value" starting at from, return value or ""
      std::string needle = "\"" + key + "\"";
      size_t k = src.find(needle, from);
      if (k == std::string::npos) return "";
      size_t colon = src.find(':', k + needle.size());
      if (colon == std::string::npos) return "";
      size_t q1 = src.find('"', colon + 1);
      if (q1 == std::string::npos) return "";
      size_t q2 = q1 + 1;
      while (q2 < src.size() && !(src[q2] == '"' && src[q2-1] != '\\'))
        q2++;
      return src.substr(q1 + 1, q2 - q1 - 1);
    };

    // find matching closing brace for object starting at `open` (which is '{')
    auto find_close = [&](const std::string &src, size_t open) -> size_t {
      int depth = 0;
      bool in_str = false;
      for (size_t i = open; i < src.size(); i++) {
        if (in_str) {
          if (src[i] == '\\') { i++; continue; }
          if (src[i] == '"') in_str = false;
        } else {
          if (src[i] == '"') { in_str = true; continue; }
          if (src[i] == '{') depth++;
          else if (src[i] == '}') { if (--depth == 0) return i; }
        }
      }
      return std::string::npos;
    };

    const std::string kind_needle = "\"kind\":\"FunctionDecl\"";
    // normalised search: clang may emit spaces around colon
    // so also try with spaces
    size_t search = 0;
    while (search < json.size()) {
      // find next FunctionDecl
      size_t kpos = json.find("\"FunctionDecl\"", search);
      if (kpos == std::string::npos) break;

      // walk back to find the opening '{' of this object
      size_t obj_start = json.rfind('{', kpos);
      if (obj_start == std::string::npos) { search = kpos + 1; continue; }

      size_t obj_end = find_close(json, obj_start);
      if (obj_end == std::string::npos) { search = kpos + 1; continue; }

      std::string obj = json.substr(obj_start, obj_end - obj_start + 1);

      // extract name
      std::string fname = json_str_val(obj, 0, "name");
      // extract qualType — looks like "vec3 (float, float, float)"
      std::string qual_type = json_str_val(obj, 0, "qualType");

      if (!fname.empty() && !qual_type.empty()) {
        // return type is everything before the first '('
        size_t paren = qual_type.find('(');
        if (paren != std::string::npos) {
          std::string ret = qual_type.substr(0, paren);
          // trim trailing spaces and pointer stars into clean base type
          while (!ret.empty() && (ret.back() == ' ' || ret.back() == '\t'))
            ret.pop_back();
          // keep pointer stars as part of type (e.g. "char *" → "char*")
          // normalise: remove spaces before *
          std::string norm;
          for (size_t i = 0; i < ret.size(); i++) {
            if (ret[i] == ' ' && i + 1 < ret.size() && ret[i+1] == '*')
              continue;
            norm += ret[i];
          }
          if (!norm.empty() && fname != "operator")
            func_return_types[fname] = norm;
        }
      }

      search = obj_end + 1;
    }
  }

  // -----------------------------------------------------------------------
  // .lh include
  // -----------------------------------------------------------------------
  void transpile_lh(const std::string &fname, const Token &tok) {
    std::string canonical;

    // first try absolute path or relative to source dir
    if (fs::path(fname).is_absolute()) {
      canonical = fs::weakly_canonical(fname).string();
      if (fs::exists(canonical)) {
        if (_lh_included.count(canonical))
          return;
        _lh_included.insert(canonical);
      } else {
        canonical = "";
      }
    } else {
      std::string path = (fs::path(_source_dir) / fname).string();
      canonical = fs::weakly_canonical(path).string();
      if (fs::exists(canonical)) {
        if (_lh_included.count(canonical))
          return;
        _lh_included.insert(canonical);
      } else {
        canonical = "";
      }
    }

    // if not found, search include paths
    if (canonical.empty()) {
      for (const auto &ipath : _include_paths) {
        std::string candidate = (fs::path(ipath) / fname).string();
        std::string cand_canonical = fs::weakly_canonical(candidate).string();
        if (fs::exists(cand_canonical)) {
          canonical = cand_canonical;
          if (_lh_included.count(canonical))
            return;
          _lh_included.insert(canonical);
          break;
        }
      }
    }

    // if still not found, search standard library paths with /lb subfolder
    if (canonical.empty()) {
      std::vector<std::string> stdlib_paths = {
          "/usr/local/lib/lb", "/usr/include/lb", "/usr/local/lib",
          "/usr/include", "/opt/local/lib"};
      for (const auto &spath : stdlib_paths) {
        std::string candidate = (fs::path(spath) / fname).string();
        std::string cand_canonical = fs::weakly_canonical(candidate).string();
        if (fs::exists(cand_canonical)) {
          canonical = cand_canonical;
          if (_lh_included.count(canonical))
            return;
          _lh_included.insert(canonical);
          break;
        }
      }
    }

    if (canonical.empty())
      throw LuaBaseError("LinkError",
                         "Cannot find .lh file '" + fname + "' (looked in '" +
                             _source_dir +
                             "', include paths, and standard locations)",
                         tok.line, tok.col);
    std::ifstream f(canonical);
    if (!f)
      throw LuaBaseError("LinkError", "Could not read '" + fname + "'",
                         tok.line, tok.col);
    std::string lh_source((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    std::vector<Token> lh_toks = Lexer(lh_source).tokenize();

    CTranspiler lh(lh_toks);
    lh.isSubTranspiler = true;
    lh._source_dir = fs::path(canonical).parent_path().string();
    lh._source_file = canonical;
    lh._lh_included = _lh_included;
    lh.var_types = var_types;
    lh.func_return_types = func_return_types;
    lh.struct_field_types = struct_field_types;
    lh.func_param_types = func_param_types;
    lh._handle_declared = _handle_declared;

    while (lh.current().type != TT::TEOF) {
      Token &lt = lh.current();
      if (lt.type == TT::SEMICOLON) {
        lh.advance();
        continue;
      }
      if (lt.type == TT::FUNCTION) {
        lh.advance();
        functions.push_back(lh.line_directive(lt) +
                            lh.parse_function_body(lt, false));
      } else if (lt.type == TT::INLINE_KW) {
        lh.advance();
        if (lh.current().type != TT::FUNCTION)
          throw LuaBaseError("SyntaxError",
                             "'inline' must be followed by 'function'", lt.line,
                             lt.col);
        lh.advance();
        functions.push_back(lh.line_directive(lt) +
                            lh.parse_function_body(lt, true));
      } else if (lt.type == TT::TYPE) {
        headers.push_back(lh.line_directive(lt) + lh.parse_type_definition());
      } else if (lt.type == TT::ENUM_KW) {
        headers.push_back(lh.line_directive(lt) + lh.parse_enum());
      } else if (lt.type == TT::LINK) {
        lh.advance();
        Token nt = lh.expect(TT::STRING, true);
        if (!nt.value.empty()) {
          if (ends_with(nt.value, ".lh"))
            transpile_lh(nt.value, nt);
          else {
            headers.push_back("#include \"" + nt.value + "\"");
            // scan for type inference
            for (const auto &ipath : _include_paths) {
              std::string candidate = (fs::path(ipath) / nt.value).string();
              if (fs::exists(candidate)) { scan_h_for_funcs(candidate); break; }
            }
            std::string local = (fs::path(_source_dir) / nt.value).string();
            if (fs::exists(local)) scan_h_for_funcs(local);
          }
        }
      } else {
        lh.parse_statement();
      }
    }
    for (const auto &h : lh.headers)
      this->headers.push_back(h);
    for (const auto &f : lh.functions)
      this->functions.push_back(f);

    for (auto const &[name, type_str] : lh.var_types) {
      this->var_types[name] = type_str;
    }
    for (auto const &[name, type_str] : lh.func_return_types) {
      this->func_return_types[name] = type_str;
    }

    this->_lh_included = lh._lh_included;
  }

public:
  explicit CTranspiler(std::vector<Token> toks) : tokens(std::move(toks)) {
    // build default headers block
    if (!isSubTranspiler) {
      headers.push_back("#include <stdio.h>\n"
                        "#include <stdbool.h>\n"
                        "#include <stdlib.h>\n"
                        "#include <math.h>\n"
                        "#include <immintrin.h>\n"
                        "#include <string.h>\n"
                        "#include <unistd.h>\n"
                        "#include <setjmp.h>\n");
      headers.push_back(
          "#ifndef __LUABASE_RUNTIME__\n"
          "#define __LUABASE_RUNTIME__\n"
          "char _lb_buf[512];\n"
          "char _lb_buf2[512];\n"
          "static inline char* _lb_s(char* x)        { return x; }\n"
          "static inline char* _lb_cs(const char* x) { return (char*)x; }\n"
          "static inline char* _lb_f(float x)        { "
          "snprintf(_lb_buf,sizeof(_lb_buf),\"%g\",x); return _lb_buf; }\n"
          "static inline char* _lb_d(double x)       { "
          "snprintf(_lb_buf,sizeof(_lb_buf),\"%.10g\",x); return _lb_buf; }\n"
          "static inline char* _lb_i(int x)          { "
          "snprintf(_lb_buf,sizeof(_lb_buf),\"%d\",x); return _lb_buf; }\n"
          "static inline char* _lb_l(long x)         { "
          "snprintf(_lb_buf,sizeof(_lb_buf),\"%ld\",x); return _lb_buf; }\n"
          "static inline char* _lb_u(short x)        { "
          "snprintf(_lb_buf,sizeof(_lb_buf),\"%d\",(int)x); return _lb_buf; }\n"
          "static inline char* _lb_b(int x)          { return x ? \"true\" : "
          "\"false\"; }\n"
          "static inline char* _lb_c(char x)         { _lb_buf[0]=x; "
          "_lb_buf[1]='\\0'; return _lb_buf; }\n"
          "static inline char* _lb_m(__m256 v)  { float f[8]; "
          "_mm256_storeu_ps(f,v); "
          "snprintf(_lb_buf,sizeof(_lb_buf),\"[%g,%g,%g,%g,%g,%g,%g,%g]\","
          "f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7]); return _lb_buf; }\n"
          "static inline char* _lb_mi(__m256i v) { union{__m256i v;int "
          "i[8];}u; u.v=v; "
          "snprintf(_lb_buf,sizeof(_lb_buf),\"[%d,%d,%d,%d,%d,%d,%d,%d]\","
          "u.i[0],u.i[1],u.i[2],u.i[3],u.i[4],u.i[5],u.i[6],u.i[7]); return "
          "_lb_buf; }\n"
          "#define TO_STR(x) _Generic((x),"
          "char*:_lb_s,const char*:_lb_cs,"
          "__m256:_lb_m,__m256i:_lb_mi,"
          "float:_lb_f,double:_lb_d,"
          "int:_lb_i,long:_lb_l,short:_lb_u,"
          "bool:_lb_b,char:_lb_c,"
          "default:_lb_i)(x)\n"
          "static jmp_buf* _lb_exc_active = NULL;\n"
          "static char*    _lb_exc_msg    = NULL;\n"
          "static inline void _lb_throw(const char* msg) {\n"
          "    if(_lb_exc_active) {\n"
          "        if(_lb_exc_msg) snprintf(_lb_exc_msg,512,\"%s\",msg);\n"
          "        longjmp(*_lb_exc_active,1);\n"
          "    } else {\n"
          "        fprintf(stderr,\"[throw] unhandled: %s\\n\",msg); exit(1);\n"
          "    }\n"
          "}\n"
          "#endif /* __LUABASE_RUNTIME__ */\n");
    }
  }

  std::string transpile(const std::string &source_dir,
                        const std::string &source_file, bool manual_main,
                        const std::vector<std::string> &include_paths = {}) {
    _source_dir = source_dir;
    _source_file = source_file;
    _include_paths = include_paths;

    while (current().type != TT::TEOF) {
      Token tok = current();
      try {
        if (tok.type == TT::SEMICOLON) {
          advance();
          continue;
        }
        if (tok.type == TT::TYPE) {
          headers.push_back(line_directive(tok) + parse_type_definition());
        } else if (tok.type == TT::ENUM_KW) {
          headers.push_back(line_directive(tok) + parse_enum());
        } else if (tok.type == TT::FUNCTION) {
          advance();
          functions.push_back(line_directive(tok) +
                              parse_function_body(tok, false));
        } else if (tok.type == TT::INLINE_KW) {
          advance();
          if (current().type != TT::FUNCTION)
            throw LuaBaseError("SyntaxError",
                               "'inline' must be followed by 'function'",
                               tok.line, tok.col);
          advance();
          functions.push_back(line_directive(tok) +
                              parse_function_body(tok, true));
        } else if (tok.type == TT::LINK) {
          advance();
          Token ft = expect(TT::STRING, true);
          if (ft.value.empty())
            throw LuaBaseError("SyntaxError",
                               "'link' requires a string filename", tok.line,
                               tok.col);
          if (ends_with(ft.value, ".lh"))
            transpile_lh(ft.value, ft);
          else {
            headers.push_back("#include \"" + ft.value + "\"");
            // scan for type inference
            std::string local = (fs::path(_source_dir) / ft.value).string();
            if (fs::exists(local)) scan_h_for_funcs(local);
            for (const auto &ipath : _include_paths) {
              std::string candidate = (fs::path(ipath) / ft.value).string();
              if (fs::exists(candidate)) { scan_h_for_funcs(candidate); break; }
            }
          }
        } else {
          tok = current();
          std::string stmt = parse_statement();
          if (!stmt.empty())
            main_body.push_back(line_directive(tok) + stmt);
        }
      } catch (LuaBaseError &) {
        throw;
      } catch (std::exception &e) {
        throw LuaBaseError("InternalError",
                           std::string("Unexpected error near '") + tok.value +
                               "': " + e.what(),
                           tok.line, tok.col);
      }
    }

    std::string body_str = join(main_body, "\n    ");
    std::string res = join(headers, "\n") + "\n";
    res += join(functions, "\n") + "\n";
    if (manual_main) {
      if (!main_body.empty())
        res += "/* [LuaBase] -main mode: top-level statements outside "
               "functions are ignored */\n";
    } else {
      res += "int main(int argc, char** argv) {\n    ";
      res += body_str;
      res += "\n    return 0;\n}";
    }
    return res;
  }
};

// ===========================================================================
// Helpers
// ===========================================================================
static std::string read_file(const std::string &path) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("Could not open: " + path);
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

static void write_file(const std::string &path, const std::string &data) {
  std::ofstream f(path);
  if (!f)
    throw std::runtime_error("Could not write: " + path);
  f << data;
}

static std::string random_str(int n) {
  static const char alpha[] = "abcdefghijklmnopqrstuvwxyz";
  std::string r;
  r.reserve(n);
  for (int i = 0; i < n; i++)
    r += alpha[rand() % 26];
  return r;
}

// ===========================================================================
// main
// ===========================================================================
int main(int argc, char **argv) {
  bool shut = false;
  bool debug = false;
  bool getasm = false;
  bool manual_main = false;
  bool priCMD = false;
  bool dbuild = false;
  auto log = [&](const std::string &s) {
    if (!shut)
      std::cout << s << "\n";
  };
  auto die = [](const std::string &s, int code = -1) -> int {
    std::cerr << "[-] " << s << "\n";
    exit(code);
    return code;
  };

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--shut" || a == "-s")
      shut = true;
    if (a == "--asm" || a == "-asm")
      getasm = true;
    if (a == "--main" || a == "-main")
      manual_main = true;
    if (a == "-sCMD")
      priCMD = true;
  }

  if (argc < 3) {
    log("luabase version 2.9.1 forked from Mebecool1");
    die("Usage: luabasec <in.lb> <out> [extra.c] [-lPATH] [-gLIB] [-wLIBDIR] "
        "[-c] [-s] [--asm] [--main]",
        1);
  }

  std::string inf = argv[1];
  if (inf == "-v") {
    std::cout << "luabase++ compiler version 2.9.1, forked from Mebecool1.";
    return 0;
  }
  std::string out_bin = argv[2];

  if (!fs::exists(inf))
    die("Input file not found: '" + inf + "'", 1);
  if (!ends_with(inf, ".lb"))
    log("[!] Warning: '" + inf + "' does not have a .lb extension");

  std::string base = fs::path(inf).stem().string();

  std::vector<std::string> custom_includes;
  std::vector<std::string> extra_c_files;
  std::vector<std::string> include_paths;

  for (int i = 3; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.size() > 2 && arg.substr(0, 2) == "-l") {
      std::string path = arg.substr(2);
      std::string folder = fs::path(path).parent_path().string();
      if (!folder.empty()) {
        custom_includes.push_back("-I" + folder);
        include_paths.push_back(folder);
      }
    }
    if (arg.size() > 2 && arg.substr(0, 2) == "-g")
      custom_includes.push_back("-l" + arg.substr(2));
    if ((ends_with(arg, ".c") || ends_with(arg, ".o"))) {
      if (fs::exists(arg))
        extra_c_files.push_back(arg);
      else
        log("[-] Warning: Extra source file '" + arg + "' not found.");
    }
    if (arg == "-c")
      debug = true;
    if (arg.size() > 2 && arg.substr(0, 2) == "-w")
      custom_includes.push_back("-L" + arg.substr(2));
    if (arg == "-debug")
      dbuild = true;
  }

  std::string source;
  try {
    source = read_file(inf);
  } catch (std::exception &e) {
    die(std::string("Could not read input file: ") + e.what());
  }

  log("[*] Tokenizing...");
  std::vector<Token> tokens;
  try {
    tokens = Lexer(source).tokenize();
  } catch (LuaBaseError &e) {
    die(e.what());
  } catch (std::exception &e) {
    die(std::string("Unexpected error during tokenization: ") + e.what());
  }

  std::string source_file = fs::weakly_canonical(inf).string();
  std::string source_dir = fs::path(source_file).parent_path().string();
  if (source_dir.empty())
    source_dir = ".";

  log("[*] Compiling...");
  std::string c_code;
  try {
    CTranspiler tr(tokens);
    c_code = tr.transpile(source_dir, source_file, manual_main, include_paths);
  } catch (LuaBaseError &e) {
    die(e.what());
  } catch (std::exception &e) {
    die(std::string("Unexpected error during transpilation: ") + e.what());
  }

  // determine C file path
  std::string c_file;
  if (debug) {
    c_file = inf + ".c";
  } else {
    srand((unsigned)time(NULL));
    auto tmp = fs::temp_directory_path();
    c_file = (tmp / (base + "-" + random_str(6) + ".c")).string();
  }

  try {
    write_file(c_file, c_code);
  } catch (std::exception &e) {
    die(std::string("Could not write C file: ") + e.what());
  }

  // find clang
  auto find_clang = []() -> std::string {
    // try which/where
    for (const char *name :
         {"clang", "clang-18", "clang-17", "clang-16", "clang-15"}) {
      std::string cmd = std::string("which ") + name + " 2>/dev/null";
      FILE *p = popen(cmd.c_str(), "r");
      if (!p)
        continue;
      char buf[512]{};
      fgets(buf, sizeof(buf), p);
      pclose(p);
      std::string r(buf);
      while (!r.empty() &&
             (r.back() == '\n' || r.back() == '\r' || r.back() == ' '))
        r.pop_back();
      if (!r.empty())
        return r;
    }
    return "";
  };

  std::string clang_bin = find_clang();
  if (clang_bin.empty()) {
    std::cerr << "[-] clang is not installed.\n";
    std::cerr << "    Install clang now? [y/N] ";
    std::string ans;
    std::getline(std::cin, ans);
    if (ans == "y" || ans == "Y") {
      // try apt
      if (system("which apt>/dev/null 2>&1") == 0)
        system("sudo apt install -y clang lld");
      else if (system("which dnf>/dev/null 2>&1") == 0)
        system("sudo dnf install -y clang lld");
      else if (system("which pacman>/dev/null 2>&1") == 0)
        system("sudo pacman -S --noconfirm clang lld");
      else
        die("No supported package manager. Install clang manually.");
      clang_bin = find_clang();
      if (clang_bin.empty())
        die("clang still not found on PATH after install.");
    } else {
      die("clang is required.");
    }
  }

  // build clang command
  std::vector<std::string> clang_args;
  clang_args.push_back(clang_bin);
  clang_args.push_back(c_file);
  for (auto &e : extra_c_files)
    clang_args.push_back(e);
  clang_args.push_back("-o");
  clang_args.push_back(out_bin);
  clang_args.push_back("-lm");
  for (auto &ci : custom_includes)
    clang_args.push_back(ci);
  if (!dbuild) {
    clang_args.push_back("-ffast-math");
    clang_args.push_back("-march=native");
    clang_args.push_back("-w");
    clang_args.push_back("-O3");
    clang_args.push_back("-fuse-ld=lld");
    clang_args.push_back("-mavx2");
    clang_args.push_back("-funroll-loops");
    
    clang_args.push_back("-fvectorize");
    

    if (!getasm)
      clang_args.push_back("-flto");
  }

  if (getasm)
    clang_args.push_back("-S");
  clang_args.push_back("-I" + source_dir);
  clang_args.push_back("-g");
  // build shell command string
  std::string cmd;
  for (size_t i = 0; i < clang_args.size(); i++) {
    if (i)
      cmd += " ";
    // quote args with spaces
    if (clang_args[i].find(' ') != std::string::npos)
      cmd += "\"" + clang_args[i] + "\"";
    else
      cmd += clang_args[i];
  }

  log("[*] Compiling C code...");
  if (debug) {
    log("Written to debug file.");
  } else if (priCMD) {
    log("[*] Running: " + cmd);
  } else {
    log("[*] Running clang to produce ./" + out_bin + "...");
  }

  if (!debug) {
    // redirect stderr to a temp file to capture it
    std::string err_file = c_file + ".err";
    std::string full_cmd = cmd + " 2>" + err_file;
    int ret = system(full_cmd.c_str());
    if (ret == 0) {
      log("[*] Made ./" + out_bin);
    } else {
      // copy c_file to local debug
      std::string local_debug = base + "_debug.c";
      std::ifstream src(c_file, std::ios::binary);
      std::ofstream dst(local_debug, std::ios::binary);
      dst << src.rdbuf();
      std::cerr << "[-] Clang error (exit " << ret << "). Debug C -> ./"
                << local_debug << "\n";
      // print captured stderr
      std::ifstream ef(err_file);
      if (ef)
        std::cerr << ef.rdbuf();
      fs::remove(err_file);
      fs::remove(c_file);
      return -2;
    }
    fs::remove(err_file);
    // remove temp c file
    try {
      fs::remove(c_file);
    } catch (...) {
    }
  }

  return 0;
}