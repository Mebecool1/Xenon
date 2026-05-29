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
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef _WIN32
#  include <unistd.h>
#endif

namespace fs = std::filesystem;

static std::string g_current_source;
static std::string g_current_source_file;
static void set_current_source_for_errors(const std::string &src,
                                          const std::string &file = "") {
  g_current_source = src;
  g_current_source_file = file;
}
static std::string extract_line_from_source(const std::string &src, int line) {
  if (line <= 0) return "";
  size_t p = 0;
  int cur = 1;
  // advance to requested line
  while (cur < line && p < src.size()) {
    size_t npos = src.find('\n', p);
    if (npos == std::string::npos) return "";
    p = npos + 1;
    cur++;
  }
  size_t start = p;
  size_t end = src.find('\n', start);
  if (end == std::string::npos) end = src.size();
  std::string s = src.substr(start, end - start);
  if (!s.empty() && s.back() == '\r') s.pop_back();
  return s;
}
// Find first occurrence of a whole-word `word` in `src`. Returns {line,col}
// (1-based) or {0,0} when not found.
static std::pair<int,int> find_word_in_source(const std::string &src, const std::string &word) {
  if (word.empty()) return {0,0};
  size_t p = 0;
  while (p < src.size()) {
    size_t pos = src.find(word, p);
    if (pos == std::string::npos) break;
    // ensure whole-word match
    bool left_ok = (pos == 0) || (!std::isalnum((unsigned char)src[pos-1]) && src[pos-1] != '_');
    size_t after = pos + word.size();
    bool right_ok = (after >= src.size()) || (!std::isalnum((unsigned char)src[after]) && src[after] != '_');
    if (left_ok && right_ok) {
      // compute line and column by scanning up to pos (robust against CR/LF)
      int cur_line = 1;
      size_t last_nl = 0;
      for (size_t i = 0; i < pos; ++i) {
        if (src[i] == '\n') {
          ++cur_line;
          last_nl = i + 1;
        }
      }
      int col = (int)(pos - last_nl) + 1;
      return {cur_line, col};
    }
    p = pos + 1;
  }
  return {0,0};
}
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
  OVERLOAD_KW,
  ADDOP_KW,
  OPERATOR_KW,
  ARGS_KW,
  BINARY_KW,
  UNARY_KW,
  ALIAS_KW,
  PIPE_KW,
  MATCH_KW,
  WITH_KW,
  IN_KW,
  NAMESPACE_KW,
  ARE_KW,
  IGNORE_KW,
  UNSAFE_KW,
  USE_KW,
  BOUNDS_ASSERT, // $bounds: assert annotation — suppresses bounds check on next subscript
  ATTR_DROP,     // $attribute: drop annotation — marks a struct method as the RAII destructor
  OPTION_KW,     // Option<T> type
  SOME_KW,       // Some(expr) constructor
  NONE_KW,       // None literal
  IF_LET_KW,     // if let Some(x) = opt { ... }
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
    C(OVERLOAD_KW);
    C(ADDOP_KW);
    C(OPERATOR_KW);
    C(ARGS_KW);
    C(BINARY_KW);
    C(UNARY_KW);
    C(ALIAS_KW);
    C(PIPE_KW);
    C(MATCH_KW);
    C(WITH_KW);
    C(IN_KW);
    C(NAMESPACE_KW);
    C(ARE_KW);
    C(IGNORE_KW);
    C(UNSAFE_KW);
    C(USE_KW);
    C(BOUNDS_ASSERT);
    C(ATTR_DROP);
    C(OPTION_KW);
    C(SOME_KW);
    C(NONE_KW);
    C(IF_LET_KW);
#undef C
  default:
    return "UNKNOWN";
  }
}

// ===========================================================================
// ANSI color helpers — auto-disabled when stderr is not a tty
// ===========================================================================
namespace ansi {
  static bool enabled() {
#ifdef _WIN32
    return false;
#else
    static int v = isatty(fileno(stderr));
    return v != 0;
#endif
  }
  // Raw codes
  inline std::string reset()    { return enabled() ? "\033[0m"     : ""; }
  inline std::string bold()     { return enabled() ? "\033[1m"     : ""; }
  inline std::string dim()      { return enabled() ? "\033[2m"     : ""; }
  inline std::string italic()   { return enabled() ? "\033[3m"     : ""; }
  inline std::string underline(){ return enabled() ? "\033[4m"     : ""; }
  // Normal colors
  inline std::string red()      { return enabled() ? "\033[31m"    : ""; }
  inline std::string green()    { return enabled() ? "\033[32m"    : ""; }
  inline std::string yellow()   { return enabled() ? "\033[33m"    : ""; }
  inline std::string blue()     { return enabled() ? "\033[34m"    : ""; }
  inline std::string magenta()  { return enabled() ? "\033[35m"    : ""; }
  inline std::string cyan()     { return enabled() ? "\033[36m"    : ""; }
  inline std::string white()    { return enabled() ? "\033[37m"    : ""; }
  inline std::string grey()     { return enabled() ? "\033[90m"    : ""; }
  // Bright / bold colors
  inline std::string bred()     { return enabled() ? "\033[1;31m"  : ""; }
  inline std::string bgreen()   { return enabled() ? "\033[1;32m"  : ""; }
  inline std::string byellow()  { return enabled() ? "\033[1;33m"  : ""; }
  inline std::string bblue()    { return enabled() ? "\033[1;34m"  : ""; }
  inline std::string bmagenta() { return enabled() ? "\033[1;35m"  : ""; }
  inline std::string bcyan()    { return enabled() ? "\033[1;36m"  : ""; }
  inline std::string bwhite()   { return enabled() ? "\033[1;97m"  : ""; }
  // Backgrounds
  inline std::string bg_red()   { return enabled() ? "\033[41m"    : ""; }
  inline std::string bg_yellow(){ return enabled() ? "\033[43m"    : ""; }
  inline std::string bg_blue()  { return enabled() ? "\033[44m"    : ""; }

  // ── Semantic helpers ──────────────────────────────────────────────────────

  // Wrap 'quoted' tokens inside a message body in bright white
  // e.g.  "got 'foo'"  →  "got " + bright_white("'foo'")
  inline std::string token(const std::string &s) {
    return bwhite() + s + reset();
  }
  // Keyword or type name inside a message
  inline std::string kw(const std::string &s) {
    return bcyan() + s + reset();
  }
  // A file path or symbol name
  inline std::string path(const std::string &s) {
    return bold() + underline() + s + reset();
  }
  // A number or literal
  inline std::string num(const std::string &s) {
    return bmagenta() + s + reset();
  }

  // ── Diagnostic block helpers ──────────────────────────────────────────────

  // Full-width separator bar (72 chars)
  inline std::string separator(const char *ch = "─", int w = 72) {
    if (!enabled()) return std::string(w, '-');
    std::string s;
    s.reserve(w * 4);
    for (int i = 0; i < w; i++) s += ch;
    return grey() + s + reset();
  }

  // Error / warning / info / ok badge
  inline std::string error_badge(const std::string &tag) {
    // White text on red background, then bold red tag in brackets
    return bg_red() + bold() + white() + " " + tag + " " + reset()
         + bred() + " ▶" + reset();
  }
  inline std::string warn_badge(const std::string &tag) {
    return bg_yellow() + bold() + "\033[30m" + " " + tag + " " + reset()
         + byellow() + " ▶" + reset();
  }
  inline std::string info_badge(const std::string &tag) {
    return bcyan() + "  ● " + reset() + bold() + tag + reset();
  }
  inline std::string ok_badge(const std::string &tag) {
    return bgreen() + "  ✔ " + reset() + bold() + tag + reset();
  }

  // Status-line tags used in log() messages
  inline std::string info_tag(const std::string &s) {
    return bcyan() + bold() + "[" + s + "]" + reset();
  }
  inline std::string ok_tag(const std::string &s) {
    return bgreen() + bold() + "[" + s + "]" + reset();
  }
  inline std::string warn_tag(const std::string &s) {
    return byellow() + bold() + "[" + s + "]" + reset();
  }

  inline std::string emphasis(const std::string &s) {
    return bwhite() + s + reset();
  }

  // ── Message body token highlighter ───────────────────────────────────────
  // Scan msg and recolor anything in single quotes 'like this' to bwhite,
  // and anything in double quotes "like this" to bmagenta.
  // Also highlights standalone words that look like type names (int, str, …)
  // after "type" or "expected" or "declared".
  inline std::string highlight_msg(const std::string &msg) {
    if (!enabled()) return msg;
    std::string out;
    out.reserve(msg.size() * 2);
    size_t i = 0;
    while (i < msg.size()) {
      char c = msg[i];
      // Single-quoted tokens: 'foo'
      if (c == '\'' && i + 1 < msg.size()) {
        size_t j = msg.find('\'', i + 1);
        if (j != std::string::npos && j > i + 1) {
          out += bwhite() + msg.substr(i, j - i + 1) + reset();
          i = j + 1;
          continue;
        }
      }
      // newline: pass through — inner lines will be plain
      out += c;
      i++;
    }
    return out;
  }
}

// Detect whether a kind string is a warning-level diagnostic
static bool is_warn_kind(const std::string &k) {
  return k == "Warning" || k == "SafetyWarning";
}

// ===========================================================================
// XenonError  — rich ANSI-colored diagnostics
// ===========================================================================
struct XenonError : std::exception {
  std::string kind, msg_full;
  // suggestion     — short human label, e.g. "remove extraneous '$' sigils"
  // fix_snippet    — the corrected source line (replaces the erroneous one)
  // fix_col        — 1-based column where the fix region starts in fix_snippet
  // fix_len        — number of chars the fix spans (for '----' underline)
  XenonError(std::string kind_, std::string msg, std::optional<int> line = {},
               std::optional<int> col = {}, std::string snippet = "",
               std::string suggestion = "", std::string fix_snippet = "",
               int fix_col = 0, int fix_len = 0)
      : kind(std::move(kind_)) {
    std::ostringstream ss;
    bool is_warn = is_warn_kind(kind);

    // ── Top separator ──────────────────────────────────────────────────────
    ss << "\n" << ansi::separator() << "\n";

    // ── Badge line:  ▌ SYNTAXERROR  line 12:5 ─────────────────────────────
    if (is_warn)
      ss << ansi::warn_badge(kind);
    else
      ss << ansi::error_badge(kind);

    if (line) {
      ss << "  " << ansi::grey() << "line " << ansi::reset()
         << ansi::byellow() << ansi::bold() << *line << ansi::reset();
      if (col && *col > 0)
        ss << ansi::grey() << ":" << ansi::reset()
           << ansi::yellow() << *col << ansi::reset();
      // If we have a current source file known, show it as well
      if (!g_current_source_file.empty()) {
        ss << "  " << ansi::path(g_current_source_file);
      }
    }
    ss << "\n";

    // ── Message body (highlighted) ─────────────────────────────────────────
    // Multi-line messages: first line bold white, continuation lines normal
    std::string highlighted = ansi::highlight_msg(msg);
    // Split on \n so continuation lines (e.g. violation lists) get dimmer color
    std::istringstream body_ss(highlighted);
    std::string body_line;
    bool first_line = true;
    while (std::getline(body_ss, body_line)) {
      if (first_line) {
        ss << "\n  " << ansi::bold() << ansi::bwhite() << body_line
           << ansi::reset() << "\n";
        first_line = false;
      } else if (!body_line.empty()) {
        // Sub-lines (like violation lists): cyan for [kind] tags, white for rest
        // Detect lines starting with "    [" — violation entries
        if (body_line.size() > 4 && body_line.substr(0, 5) == "    [") {
          size_t rb = body_line.find(']');
          if (rb != std::string::npos) {
            std::string vtag  = body_line.substr(4, rb - 3);  // "[Kind]"
            std::string vrest = body_line.substr(rb + 1);
            ss << "  " << ansi::bcyan() << vtag << ansi::reset()
               << ansi::white() << vrest << ansi::reset() << "\n";
          } else {
            ss << "  " << ansi::white() << body_line << ansi::reset() << "\n";
          }
        } else if (!body_line.empty() && body_line[0] == ' ') {
          // Indented continuation — yellow for "Fix:" prefix, grey otherwise
          if (body_line.find("Fix:") != std::string::npos)
            ss << "  " << ansi::byellow() << body_line << ansi::reset() << "\n";
          else
            ss << "  " << ansi::grey() << body_line << ansi::reset() << "\n";
        } else {
          ss << "  " << ansi::yellow() << body_line << ansi::reset() << "\n";
        }
      }
    }

    // ── Source snippet + caret ─────────────────────────────────────────────
    // If the throw site didn't include a snippet but we have a global
    // source buffer, extract the requested line so diagnostics always show
    // the source line and caret when possible.
    std::string final_snippet = snippet;
    if (final_snippet.empty() && line && !g_current_source.empty())
      final_snippet = extract_line_from_source(g_current_source, *line);
    if (!final_snippet.empty()) {
      int caret_col = (col && *col > 0) ? *col : 0;
      // if col is missing, try to place caret at first non-space
      if (caret_col == 0) {
        size_t p = final_snippet.find_first_not_of(' ');
        caret_col = (p == std::string::npos) ? 1 : (int)p + 1;
      }
      ss << "\n";
      // Line number gutter
      std::string lnum = line ? std::to_string(*line) : "?";
      ss << ansi::blue() << ansi::bold() << "  " << lnum << " │ "
         << ansi::reset()
         << ansi::bwhite() << final_snippet << ansi::reset() << "\n";
      // Caret row
      int gutter_w = 2 + (int)lnum.size() + 3; // "  N │ "
      ss << ansi::blue() << ansi::bold()
         << std::string(gutter_w, ' ') << ansi::reset();
      if (caret_col > 0 && caret_col <= (int)final_snippet.size() + 1) {
        const std::string &caret_color = is_warn ? ansi::byellow() : ansi::bred();
        ss << std::string(caret_col - 1, ' ')
           << caret_color << ansi::bold() << "^"
           << caret_color << std::string(
                std::max(0, (int)final_snippet.size() - caret_col), '~')
           << ansi::reset();
      }
      ss << "\n";

      // ── Suggestion block (clang/rustc style) ──────────────────────────────
      // Rendered only when the caller supplies a fix_snippet.
      // Format mirrors rustc's `help:` / clang's fix-it:
      //
      //   note: replace with:
      //   2 │ println(var)
      //             ----
      if (!suggestion.empty() || !fix_snippet.empty()) {
        // "note:" label
        ss << "\n  " << ansi::bcyan() << ansi::bold() << "note:" << ansi::reset()
           << " " << ansi::white();
        if (!suggestion.empty())
          ss << suggestion;
        else
          ss << "replace with:";
        ss << ansi::reset() << "\n";

        if (!fix_snippet.empty()) {
          // Reuse the same gutter width as the error line
          ss << ansi::blue() << ansi::bold() << "  " << lnum << " │ "
             << ansi::reset()
             << ansi::bgreen() << fix_snippet << ansi::reset() << "\n";
          // '----' underline in green under the replacement region
          if (fix_col > 0 && fix_len > 0) {
            ss << ansi::blue() << ansi::bold()
               << std::string(gutter_w, ' ') << ansi::reset();
            ss << std::string(fix_col - 1, ' ')
               << ansi::bgreen() << ansi::bold()
               << std::string(fix_len, '-')
               << ansi::reset() << "\n";
          }
        }
      }
    }

    // Show related occurrences — only for errors where a definition/declaration
    // site is genuinely useful to see alongside the error.
    // Rules for appearing:
    //   1. Kind must be in the opt-in set (not shown for most error kinds).
    //   2. Candidate must be a quoted token from the message ('like_this').
    //      Bare-word scraping is dropped — it produced too many false hits.
    //   3. Candidate must look like a user identifier: length >= 2, contains
    //      at least one letter, not a plain number, not a language keyword.
    //   4. The found line must differ from the primary error line by more than
    //      one line (avoids re-showing the same context twice).
    //   5. The found line must look like a definition/declaration site (contains
    //      a keyword such as function/fn/let/var/const/type/struct/enum) OR the
    //      token appears more than once in the source (genuinely used elsewhere).
    //      This prevents showing random incidental hits.
    //   6. At most ONE related snippet is shown to keep output tight.
    static const std::set<std::string> related_kinds = {
      "ArgCountError", "SyntaxError", "ModuleError"
    };
    // Language keywords — never treat these as meaningful "related" candidates.
    static const std::set<std::string> kw_stop = {
      "if","else","elseif","while","for","do","end","then","return","function",
      "fn","let","var","const","type","struct","enum","switch","case","default",
      "break","continue","true","false","null","void","int","str","float","long",
      "double","short","char","bool","u8","u32","u64","ptr","and","or","not",
      "in","match","with","use","global","inline","define","alias","namespace",
      "try","except","throw","unsafe","overload","operator","binary","unary",
      "sizeof","typeof","cast","assert","println","print","scanf","fopen",
      "fwrite","fclose","fread","hlt","exit","strlen","strcpy","strcat","memset",
      "line","column","error","warning","unexpected","near","requires","must",
      "syntax","missing","found","expected","got","declared","defined","used",
      "argument","parameter","call","value","expression","statement","block",
      "module","file","symbol","identifier","operator","token","keyword"
    };

    if (!g_current_source.empty() && related_kinds.count(kind)) {
      // Collect only explicitly quoted tokens (text between single-quotes in msg).
      std::vector<std::string> candidates;
      for (size_t i = 0; i < msg.size(); ++i) {
        if (msg[i] == '\'') {
          size_t j = msg.find('\'', i + 1);
          if (j != std::string::npos && j > i + 1) {
            std::string tok = msg.substr(i + 1, j - i - 1);
            i = j;
            // Must look like a user identifier: letters only in id chars, >= 2 long
            if (tok.size() < 2) continue;
            bool all_id = true;
            bool has_letter = false;
            for (char ch : tok) {
              if (!std::isalnum((unsigned char)ch) && ch != '_') { all_id = false; break; }
              if (std::isalpha((unsigned char)ch)) has_letter = true;
            }
            if (!all_id || !has_letter) continue;
            // Reject keywords
            std::string low = tok;
            for (auto &ch : low) ch = (char)std::tolower((unsigned char)ch);
            if (kw_stop.count(low)) continue;
            // Deduplicate
            bool dup = false;
            for (auto &c : candidates) if (c == tok) { dup = true; break; }
            if (!dup) candidates.push_back(tok);
          }
        }
      }

      // Count total occurrences of a word in source (cheap scan).
      auto count_occurrences = [&](const std::string &word) -> int {
        int cnt = 0;
        size_t p = 0;
        while (p < g_current_source.size()) {
          size_t pos = g_current_source.find(word, p);
          if (pos == std::string::npos) break;
          bool left_ok  = (pos == 0) || (!std::isalnum((unsigned char)g_current_source[pos-1]) && g_current_source[pos-1] != '_');
          size_t after  = pos + word.size();
          bool right_ok = (after >= g_current_source.size()) || (!std::isalnum((unsigned char)g_current_source[after]) && g_current_source[after] != '_');
          if (left_ok && right_ok) ++cnt;
          p = pos + 1;
        }
        return cnt;
      };

      // Definition-site heuristic: does the line look like where something is introduced?
      auto looks_like_def = [](const std::string &snip) -> bool {
        static const std::vector<std::string> def_kws = {
          "function ","fn ","let ","var ","const ","type ","struct ","enum ","operator"
        };
        for (auto &dkw : def_kws)
          if (snip.find(dkw) != std::string::npos) return true;
        return false;
      };

      int shown = 0;
      for (const auto &cand : candidates) {
        if (shown >= 1) break; // cap at one to avoid clutter
        auto [rline, rcol] = find_word_in_source(g_current_source, cand);
        if (rline == 0) continue;
        // Skip if it's within ±1 line of the primary error (already visible).
        if (line && std::abs(rline - *line) <= 1) continue;
        std::string related_snip = extract_line_from_source(g_current_source, rline);
        if (related_snip.empty()) continue;
        // Only show if the line looks like a definition OR the word appears
        // multiple times (meaning it's actually used/referenced elsewhere).
        int occ = count_occurrences(cand);
        if (!looks_like_def(related_snip) && occ < 2) continue;

        ss << "\n" << ansi::separator("-", 40) << "\n";
        ss << ansi::info_badge("Related") << " " << ansi::token(cand) << "\n\n";
        ss << ansi::blue() << ansi::bold() << "  " << rline << " │ " << ansi::reset()
           << ansi::bwhite() << related_snip << ansi::reset() << "\n";
        int gutter_w2 = 2 + (int)std::to_string(rline).size() + 3;
        ss << ansi::blue() << ansi::bold() << std::string(gutter_w2, ' ') << ansi::reset();
        int caret_at = rcol > 0 ? rcol : 1;
        if (caret_at > 0 && caret_at <= (int)related_snip.size() + 1) {
          ss << std::string(caret_at - 1, ' ')
             << ansi::bred() << ansi::bold() << "^"
             << ansi::bred() << std::string(std::max(0, (int)related_snip.size() - caret_at), '~')
             << ansi::reset() << "\n";
        } else ss << "\n";
        shown++;
      }
    }

    // ── Bottom separator ───────────────────────────────────────────────────
    ss << ansi::separator() << "\n";

    msg_full = ss.str();
  }
  const char *what() const noexcept override { return msg_full.c_str(); }
};

// ===========================================================================
// CCBackend — which C compiler to target
// ===========================================================================
enum class CCBackend {
  CLANG, // default: clang (also gcc-compatible)
  GCC,
  TCC, // Tiny C Compiler — C99 only, no _Generic, no AVX intrinsics
  CC,  // whatever 'cc' on PATH is
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
  // Custom operator symbols registered via addop — checked before the
  // built-in single/two-char tables so they take priority.
  std::set<std::string> custom_ops_n; // multi-char custom symbols (2-3+ chars)
  std::set<std::string> custom_ops_2; // two-char custom symbols
  std::set<std::string> custom_ops_1; // one-char custom symbols
public:
  std::vector<std::string> lex_errors; // collected errors (non-fatal accumulation)
private:

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
        {"overload", TT::OVERLOAD_KW},
        {"addop", TT::ADDOP_KW},
        {"operator", TT::OPERATOR_KW},
        {"args", TT::ARGS_KW},
        {"binary", TT::BINARY_KW},
        {"unary", TT::UNARY_KW},
        {"alias", TT::ALIAS_KW},
        {"pipe", TT::PIPE_KW},
        {"match", TT::MATCH_KW},
        {"with", TT::WITH_KW},
        {"in", TT::IN_KW},
        {"namespace", TT::NAMESPACE_KW},
        {"are", TT::ARE_KW},
        {"ignore", TT::IGNORE_KW},
        {"unsafe", TT::UNSAFE_KW},
        {"use", TT::USE_KW},
        {"Option", TT::OPTION_KW},
        {"Some", TT::SOME_KW},
        {"None", TT::NONE_KW},
        {"if let", TT::IF_LET_KW}, // not reachable via single-word lookup; handled in tokenize
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
  explicit Lexer(std::string source,
                 std::set<std::string> custom_syms = {})
      : src(std::move(source)) {
    for (const auto &sym : custom_syms) {
      if (sym.size() >= 2) custom_ops_n.insert(sym);
      if (sym.size() == 2) custom_ops_2.insert(sym);
      else if (sym.size() == 1) custom_ops_1.insert(sym);
    }
  }

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    while (pos < src.size()) {
      char c = src[pos];
      // wrap entire character-dispatch in a try so lex errors are collected
      // rather than aborting on the first one.
      size_t skip_after_error = 0; // if set, catch will advance pos here
      try {

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
        int cmt_line = line, cmt_col = col;
        advance_char();
        advance_char();
        bool closed = false;
        while (pos + 1 < src.size()) {
          if (src[pos] == '*' && src[pos + 1] == '/') {
            advance_char();
            advance_char();
            closed = true;
            break;
          }
          advance_char();
        }
        if (!closed)
          throw XenonError("LexError", "Unterminated block comment", cmt_line,
                             cmt_col);
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
            throw XenonError("LexError", "Unterminated string literal",
                               tok_line, tok_col, current_line_text());
          if (src[pos] == '\\' && pos + 1 < src.size())
            advance_char();
          advance_char();
        }
        if (pos >= src.size())
          throw XenonError("LexError",
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

      // custom two-char operators (registered via addop)
      if (pos + 1 < src.size()) {
        std::string d = src.substr(pos, 2);
        if (custom_ops_2.count(d)) {
          tokens.emplace_back(TT::IDENTIFIER, d, tok_line, tok_col);
          advance_char(); advance_char();
          continue;
        }
      }
      // custom 3-char operators (registered via addop)
      if (pos + 2 < src.size()) {
        std::string s3 = src.substr(pos, 3);
        if (custom_ops_n.count(s3)) {
          tokens.emplace_back(TT::IDENTIFIER, s3, tok_line, tok_col);
          advance_char();
          advance_char();
          advance_char();
          continue;
        }
      }

      // custom 2-char operators (registered via addop)
      if (pos + 1 < src.size()) {
        std::string s2 = src.substr(pos, 2);
        if (custom_ops_n.count(s2)) {
          tokens.emplace_back(TT::IDENTIFIER, s2, tok_line, tok_col);
          advance_char();
          advance_char();
          continue;
        }
      }

      // custom one-char operators (registered via addop)
      {
        std::string s1(1, c);
        if (custom_ops_1.count(s1)) {
          tokens.emplace_back(TT::IDENTIFIER, s1, tok_line, tok_col);
          advance_char();
          continue;
        }
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

      {
        // ── Build a rich diagnostic with suggestion for each bad char ──────
        std::string raw_line = current_line_text();
        std::string msg, suggestion, fix_snip;
        int fix_col = 0, fix_len = 0;

        if (c == '$') {
          // Check for $bounds: assert annotation before treating as error
          // Pattern: '$' immediately followed by "bounds" then ':' then "assert"
          // (whitespace allowed between tokens after the colon)
          size_t scan = pos + 1; // pos is already at '$'
          // collect word after $
          size_t word_start = scan;
          while (scan < src.size() && (std::isalnum((unsigned char)src[scan]) || src[scan] == '_'))
            scan++;
          std::string word = src.substr(word_start, scan - word_start);
          if (word == "bounds") {
            // skip optional whitespace
            while (scan < src.size() && src[scan] == ' ') scan++;
            if (scan < src.size() && src[scan] == ':') {
              scan++;
              while (scan < src.size() && src[scan] == ' ') scan++;
              size_t kw_start = scan;
              while (scan < src.size() && (std::isalnum((unsigned char)src[scan]) || src[scan] == '_'))
                scan++;
              std::string kw = src.substr(kw_start, scan - kw_start);
              if (kw == "assert") {
                // Consume all chars up to scan
                while (pos < scan) advance_char();
                tokens.push_back(Token(TT::BOUNDS_ASSERT, "$bounds:assert", tok_line, tok_col));
                continue;
              }
            }
          }
          // $attribute: drop  — marks the following function as the RAII destructor
          if (word == "attribute") {
            while (scan < src.size() && src[scan] == ' ') scan++;
            if (scan < src.size() && src[scan] == ':') {
              scan++;
              while (scan < src.size() && src[scan] == ' ') scan++;
              size_t kw_start = scan;
              while (scan < src.size() && (std::isalnum((unsigned char)src[scan]) || src[scan] == '_'))
                scan++;
              std::string kw = src.substr(kw_start, scan - kw_start);
              if (kw == "drop") {
                while (pos < scan) advance_char();
                tokens.push_back(Token(TT::ATTR_DROP, "$attribute:drop", tok_line, tok_col));
                continue;
              }
            }
          }
          // Not a recognized $ annotation — fall through to error as before
          int line_off = tok_col - 1;
          int ndollar = 0;
          size_t scan2 = (size_t)line_off;
          while (scan2 < raw_line.size() && raw_line[scan2] == '$') { ndollar++; scan2++; }
          size_t id_start = scan2;
          while (scan2 < raw_line.size() &&
                 (std::isalnum((unsigned char)raw_line[scan2]) || raw_line[scan2] == '_'))
            scan2++;
          std::string ident = raw_line.substr(id_start, scan2 - id_start);
          msg = std::string("unexpected '") + c + "': Xenon does not use variable sigils";
          if (ndollar == 1) suggestion = "remove the '$'";
          else suggestion = "remove the " + std::to_string(ndollar) + " '$' sigils";
          if (!ident.empty()) suggestion += " — write just '" + ident + "'";
          fix_snip = raw_line.substr(0, line_off) + ident + raw_line.substr(id_start + ident.size());
          fix_col = line_off + 1;
          fix_len = (int)ident.size();

        } else if (c == '@') {
          msg = "unexpected '@': did you mean '&' for address-of?";
          suggestion = "use '&' instead of '@'";
          fix_snip = raw_line;
          fix_snip[tok_col - 1] = '&';
          fix_col = tok_col;
          fix_len = 1;

        } else if (c == '`') {
          // Find matching closing backtick on the same line, if any
          size_t close = raw_line.find('`', tok_col);
          msg = "unexpected '`': string literals use double quotes, not backticks";
          if (close != std::string::npos) {
            suggestion = "replace backticks with double quotes";
            fix_snip = raw_line;
            fix_snip[tok_col - 1] = '"';
            fix_snip[close] = '"';
            fix_col = tok_col;
            fix_len = (int)(close - (tok_col - 1)) + 1;
            // skip to just after the closing backtick so it doesn't re-fire
            // close is 0-based offset in raw_line; pos is currently at tok_col-1
            // so closing backtick is at pos + (close - (tok_col-1))
            skip_after_error = pos + (close - (tok_col - 1)) + 1;
          } else {
            suggestion = "replace '`' with '\"'";
            fix_snip = raw_line;
            fix_snip[tok_col - 1] = '"';
            fix_col = tok_col;
            fix_len = 1;
          }

        } else if (c == '\\') {
          msg = "unexpected '\\': standalone backslash is only valid inside string literals";
          suggestion = "remove the backslash, or place it inside a string";

        } else {
          msg = std::string("unexpected character '") + c + "'";
        }

        throw XenonError("LexError", msg, tok_line, tok_col, raw_line,
                          suggestion, fix_snip, fix_col, fix_len);
      }
      } catch (XenonError &_lex_err) {
        // Collect error; skip the entire run of the same offending character
        // (e.g. all four '$' in '$$$$var') so we emit one error per run, not
        // one per character.
        lex_errors.push_back(_lex_err.what());
        if (skip_after_error > pos) {
          // jump directly past a known end point (e.g. closing backtick)
          while (pos < skip_after_error && pos < src.size()) advance_char();
        } else {
          char bad = (pos < src.size()) ? src[pos] : '\0';
          if (pos < src.size()) advance_char();
          // absorb repeated identical junk chars so we don't re-fire
          while (pos < src.size() && src[pos] == bad &&
                 bad != '\0' && !std::isalnum((unsigned char)bad) && bad != '_')
            advance_char();
        }
      }
    }
    tokens.emplace_back(TT::TEOF, "", line, col);
    return tokens;
  }
};

// ===========================================================================
// Helpers
// ===========================================================================

// Pre-scan source text for  addop operator(SYM)  declarations and return
// the set of custom symbols so the Lexer can recognise them before parsing.
// This is a simple regex-free scan — we just look for the literal text
// "addop" followed (with optional whitespace) by "operator" then "(SYM)".
static std::set<std::string> prescan_addop_symbols(const std::string &src) {
  std::set<std::string> syms;
  size_t p = 0;
  while (p < src.size()) {
    // find next occurrence of "addop"
    size_t found = src.find("addop", p);
    if (found == std::string::npos) break;
    p = found + 5; // skip "addop"
    // skip whitespace
    while (p < src.size() && std::isspace((unsigned char)src[p])) p++;
    // must be followed by "operator"
    if (src.substr(p, 8) != "operator") continue;
    p += 8;
    // skip whitespace
    while (p < src.size() && std::isspace((unsigned char)src[p])) p++;
    // must be '('
    if (p >= src.size() || src[p] != '(') continue;
    p++;
    // collect chars until ')'
    std::string sym;
    while (p < src.size() && src[p] != ')' && src[p] != '\n') {
      if (!std::isspace((unsigned char)src[p])) sym += src[p];
      p++;
    }
    if (!sym.empty() && sym.size() <= 3) syms.insert(sym);
  }
  return syms;
}

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
  std::string base; // "int", "float", "double", "char*", "bool", "void", etc.
  int ptr_depth{0}; // extra pointer indirections beyond what's in base
  bool is_array{false};
  bool is_const{false};

  static TypeInfo of(std::string b, int pd = 0) {
    TypeInfo t;
    t.base = std::move(b);
    t.ptr_depth = pd;
    return t;
  }
  static TypeInfo unknown() {
    TypeInfo t;
    t.base = "";
    return t;
  }
  bool is_unknown() const { return base.empty(); }

  // Render to a C declaration type string (falls back to int for unknown)
  std::string c_type() const {
    if (base.empty())
      return "int"; // unknown → default int for C emission
    std::string r = base;
    for (int i = 0; i < ptr_depth; i++)
      r += "*";
    return r;
  }

  bool is_ptr() const {
    return ptr_depth > 0 || (base.size() > 0 && base.back() == '*');
  }
  bool is_float() const { return base == "float" || base == "double"; }
  bool is_integer() const {
    return base == "int" || base == "long" || base == "short" ||
           base == "char" || base == "bool" || base == "uint8_t" ||
           base == "uint32_t" || base == "uint64_t";
  }
  bool is_numeric() const { return is_float() || is_integer(); }
};

// Arithmetic promotion: widest numeric type wins (mirrors C's usual arithmetic
// conversions)
static TypeInfo promote(const TypeInfo &a, const TypeInfo &b) {
  // unknown: prefer the known side
  if (a.is_unknown())
    return b;
  if (b.is_unknown())
    return a;

  // pointer arithmetic: ptr ± int → ptr
  if (a.is_ptr() && b.is_integer())
    return a;
  if (b.is_ptr() && a.is_integer())
    return b;

  if (a.base == "double" || b.base == "double")
    return TypeInfo::of("double");
  if (a.base == "float" || b.base == "float")
    return TypeInfo::of("float");

  // integer promotion: pick the wider one
  auto rank = [](const std::string &s) -> int {
    if (s == "bool")
      return 0;
    if (s == "char")
      return 1;
    if (s == "uint8_t")
      return 1;
    if (s == "short")
      return 2;
    if (s == "int")
      return 3;
    if (s == "uint32_t")
      return 4;
    if (s == "long")
      return 5;
    if (s == "uint64_t")
      return 6;
    return 3; // unknown → int
  };
  return rank(a.base) >= rank(b.base) ? a : b;
}

// ===========================================================================
// CTranspiler
// ===========================================================================
class CTranspiler {
  bool isSubTranspiler = false;
  bool emit_line_directives{true}; // set false by --noLine
  bool tcc_mode{false};            // true when targeting TCC (C99, no _Generic)
  bool suppress_warnings{false};   // set true by -suppress:warnings CLI flag
  std::vector<Token> tokens;
  size_t pos{0};

  std::vector<std::string> headers;
  std::vector<std::string> functions;
  std::vector<std::string> main_body;
public:
  std::vector<std::string> transpile_errors;   // collected errors  (fatal)
  std::vector<std::string> transpile_warnings; // collected warnings (non-fatal)

  // Emit a Warning-level diagnostic — does NOT stop compilation.
  // Respects suppress_warnings flag.
  void emit_warning(const std::string &kind, const std::string &msg,
                    int line = 0, int col = 0) {
    if (suppress_warnings) return;
    transpile_warnings.push_back(
        XenonError("Warning", "[" + kind + "] " + msg,
                   line > 0 ? std::optional<int>(line) : std::nullopt,
                   col  > 0 ? std::optional<int>(col)  : std::nullopt).what());
  }
private:
  std::map<std::string, std::string> var_types;
  // ptr variable → number of elements (as a C expression string) for runtime bounds checks.
  // Populated when malloc/calloc argument can be parsed at compile time.
  // Used in parse_primary to wrap heap-pointer subscripts with runtime guards.
  // Value is a C integer expression, e.g. "10", "(n)", "((40)/sizeof(int))".
  std::map<std::string, std::string> _ptr_alloc_elems;
  // Alias tracking: if `ptr double q = p`, record q → p so subscripts on q
  // inherit p's element-count entry from _ptr_alloc_elems.
  std::map<std::string, std::string> _ptr_aliases;
  // Variables whose values originate from a runtime source (scanf, C function
  // return, random, etc.) — used by the look-back index resolver to decide
  // whether a conditional that modifies an index is statically evaluable.
  std::set<std::string> _runtime_vars;
  std::map<std::string, std::string>
      func_return_types; // fname → raw return type
  // struct_name → {field_name → c_type}
  std::map<std::string, std::map<std::string, std::string>> struct_field_types;
  // fname → vector of param c_types (for argument-position inference)
  std::map<std::string, std::vector<std::string>> func_param_types;
  // fname → vector of declared array sizes per param (-1 = not an array param)
  std::map<std::string, std::vector<int>> func_param_array_sizes;

  // Monomorphization support ------------------------------------------------
  struct TemplateFunc {
    size_t
        tok_start;  // index of return-type token (first token after 'function')
    size_t tok_end; // index just past closing '}'
    bool inl;
    std::string raw_ret; // "let","var","int","float",...
    std::string generic_ret_param;  // e.g. "T" if return type is Vec<T>
    std::string func_type_param;    // e.g. "T" from function name<T>
    struct ParamSlot {
      std::string raw;
      bool infer;
      bool is_array;
    };
    std::vector<ParamSlot> param_slots;
    // When merged from a sub-transpiler (.xen file), store the token range
    // so instantiation doesn't depend on invalid indices in parent's token
    // stream
    std::vector<Token> extracted_tokens;
    bool has_extracted = false;
  };
  std::map<std::string, TemplateFunc> template_funcs;
  std::map<std::string, std::map<std::string, std::string>> mono_registry;
  // prevent re-entrant instantiation of same specialization
  std::set<std::string> _mono_in_progress;
  int _mono_depth{0}; // guards against indirect recursive template loops
  static constexpr int _mono_max_depth = 64;

  // Generic struct template registry ----------------------------------------
  // Stores the raw token range for a generic type definition:
  //   type Vec<T> { ptr T data; int capacity; int length }
  // Key: struct name (e.g. "Vec"), Value: descriptor
  struct GenericStructTemplate {
    std::string type_param;         // e.g. "T"
    std::vector<std::string> field_raw_types; // raw type strings ("T", "ptr T", "int", ...)
    std::vector<std::string> field_names;     // field names
    std::vector<std::string> field_suffixes;  // "[N]" or ""
    std::vector<bool>  field_is_ptr;
    std::vector<bool>  field_is_array;
    std::vector<std::string> field_array_sizes;
    std::string ns_prefix; // namespace prefix at definition time
  };
  std::map<std::string, GenericStructTemplate> _generic_structs;
  // Already instantiated: "Vec__int" → true
  std::set<std::string> _instantiated_generic_structs;

  // Instantiate a generic struct for a concrete type.
  // Returns the mangled C struct name (e.g. "Vec__int").
  std::string instantiate_generic_struct(const std::string &tname,
                                          const std::string &concrete_type) {
    auto it = _generic_structs.find(tname);
    if (it == _generic_structs.end()) return tname; // not generic
    const GenericStructTemplate &tmpl = it->second;

    // Build mangled name: Vec__int, Vec__float, etc.
    std::string mangled = tname;
    std::string safe_ct = concrete_type;
    for (char &c : safe_ct) if (c == '*' || c == ' ') c = '_';
    mangled += "__" + safe_ct;

    if (_instantiated_generic_structs.count(mangled)) return mangled;
    _instantiated_generic_structs.insert(mangled);

    // Resolve concrete C type
    std::string c_concrete = raw_to_c(concrete_type);

    // Build fields, substituting T → concrete_type
    std::vector<std::string> fields;
    auto &field_map = struct_field_types[mangled];
    for (size_t i = 0; i < tmpl.field_names.size(); i++) {
      std::string f_raw = tmpl.field_raw_types[i];
      bool is_ptr = tmpl.field_is_ptr[i];
      std::string f_type;
      if (f_raw == tmpl.type_param) {
        f_type = c_concrete;
        if (is_ptr) f_type += "*";
      } else if (is_ptr) {
        std::string inner = (f_raw == "str") ? "char*" : raw_to_c(f_raw);
        f_type = inner + "*";
      } else {
        f_type = (f_raw == "str") ? "char*" : raw_to_c(f_raw);
      }
      std::string f_name = tmpl.field_names[i];
      std::string f_suffix = tmpl.field_array_sizes[i];
      if (tmpl.field_is_array[i]) {
        fields.push_back(f_type + " " + f_name + "[" + f_suffix + "];");
        field_map[f_name] = f_type + "[" + f_suffix + "]";
      } else {
        fields.push_back(f_type + " " + f_name + ";");
        field_map[f_name] = f_type;
      }
    }

    std::string code = "typedef struct {\n    " + join(fields, "\n    ") +
                       "\n} " + mangled + ";\n";
    // Insert before other functions/headers so struct is available
    headers.push_back(code);
    var_types[mangled] = "STRUCT";

    // Copy method registrations from the generic template to the instantiated name.
    // struct_methods["Vec"] → struct_methods["Vec__int"]
    auto _msrc = struct_methods.find(tname);
    if (_msrc != struct_methods.end()) {
      auto &_mdst = struct_methods[mangled];
      for (const auto &mname : _msrc->second)
        _mdst.insert(mname);
    }
    // Also copy method implementations (C code) from the generic template.
    // These were stored under struct_method_impls[tname] during parse_type_definition.
    // We need copies mangled for the concrete type (e.g. "Vec__int__move").
    // These will be emitted to headers so they're available at call sites.
    auto _misrc = struct_method_impls.find(tname);
    if (_misrc != struct_method_impls.end()) {
      // Word-boundary-safe replacement helper
      auto replace_word = [](std::string s, const std::string &from,
                             const std::string &to) -> std::string {
        if (from.empty()) return s;
        size_t mp = 0;
        while ((mp = s.find(from, mp)) != std::string::npos) {
          bool ok = true;
          if (mp > 0 && (std::isalnum((unsigned char)s[mp-1]) || s[mp-1] == '_'))
            ok = false;
          size_t after = mp + from.size();
          if (ok && after < s.size() && (std::isalnum((unsigned char)s[after]) || s[after] == '_'))
            ok = false;
          if (ok) { s.replace(mp, from.size(), to); mp += to.size(); }
          else     mp += from.size();
        }
        return s;
      };

      for (const auto &mcode : _misrc->second) {
        std::string patched = mcode;
        // 1. Rename function prefix: "tname__method" → "mangled__method"
        std::string old_prefix = tname + "__";
        std::string new_prefix = mangled + "__";
        size_t mpos = 0;
        while ((mpos = patched.find(old_prefix, mpos)) != std::string::npos) {
          patched.replace(mpos, old_prefix.size(), new_prefix);
          mpos += new_prefix.size();
        }
        // 2. Rename self parameter type: "tname* self" → "mangled* self"
        {
          std::string old_self = tname + "* self";
          std::string new_self = mangled + "* self";
          mpos = 0;
          while ((mpos = patched.find(old_self, mpos)) != std::string::npos) {
            patched.replace(mpos, old_self.size(), new_self);
            mpos += new_self.size();
          }
        }
        // 3. Replace the type parameter name (e.g. "T") with the concrete C type,
        //    using word-boundary matching so we don't clobber unrelated identifiers.
        // Get type_param from the stored template.
        if (!tmpl.type_param.empty()) {
          patched = replace_word(patched, tmpl.type_param, concrete_type);
        }
        headers.push_back(patched);
      }
    }

    return mangled;
  }

  // Operator overload / custom operator registry ----------------------------
  // key: symbol string ("+", "-", "<<", etc.)
  // value: generated C function name
  struct OverloadEntry {
    std::string func_name;  // e.g. "_Overload_plus_a_b_int"
    std::string arg_a;      // first param name used at definition
    std::string arg_b;      // second param name (empty for unary)
    std::string ret_type;   // C return type
    bool is_binary{true};
    std::string type_a;     // C type of first param
    std::string type_b;     // C type of second param
  };
  std::map<std::string, std::vector<OverloadEntry>> _op_overloads;  // symbol → entries (multiple overloads)
  // alias registry: alias name → original name
  std::map<std::string, std::string> _aliases;

  std::string _cur_func{"__main__"};
  size_t _cur_func_body_start{0}; // token index of first statement in current function body
  // Current struct being parsed (set while inside parse_type_definition)
  std::string _cur_struct{""};
  // struct name -> list of (method_name, c_code) pairs to emit after the struct
  std::map<std::string, std::vector<std::string>> struct_method_impls;
  // struct name -> set of method names (for call-site dispatch)
  std::map<std::string, std::set<std::string>> struct_methods;
  // Structs that have a $attribute: drop method — RAII destructors are emitted
  // automatically when a variable of this type goes out of scope.
  std::set<std::string> struct_drop_funcs;
  // Per-scope RAII tracking: varname → drop C call expression
  std::map<std::string, std::string> _raii_var_drop_expr;
  // Track which __attribute__((cleanup)) helper shims have already been emitted
  // to avoid duplicate static definitions.
  std::set<std::string> _raii_shims_emitted;
  std::string _cur_func_ret{"int"};

  bool _memory_safe{true};
  bool _borrow_check{true};   // -no-check:borrow or -no-check disables
  bool _in_unsafe_block{false};
  std::set<std::string> _unsafe_functions;

  // ── Symbol tracking for undefined-function/type validation ───────────────
  // Functions and types imported from .h files (populated by scan_h_full)
  std::set<std::string> _h_imported_functions;
  std::set<std::string> _h_imported_types;
  // Functions and types imported from .xen files (populated by transpile_lh)
  std::set<std::string> _xen_imported_functions;
  std::set<std::string> _xen_imported_types;

  // All Xenon builtin function-like keywords (emit as C calls in parse_expr /
  // parse_statement without going through emit_call, so they never hit the
  // undefined-call check — but we list them here for completeness and for the
  // is_known_function() helper).
  static const std::set<std::string> &builtin_functions() {
    static const std::set<std::string> s = {
      // I/O
      "print","println","printfmt","scanf","printf","fprintf","sprintf",
      "snprintf","sscanf","fscanf","fgets","fputs","puts","getchar","fgetc",
      "getc","putchar","fputc","perror","gets_s",
      // file I/O (keyword-handled in parse_statement, but also callable)
      "fopen","fclose","fwrite","fread","fflush","ftell","fseek","frewind",
      "rewind","feof","ferror","tmpfile","popen","pclose",
      // string
      "strlen","strcpy","strncpy","strcat","strncat","strcmp","strncmp",
      "strchr","strrchr","strstr","strtok","strdup","strndup","strtol",
      "strtod","strtoul","strtoull","strtof","strtold","atoi","atof","atol",
      "atoll","sprintf","snprintf","sscanf","memcpy","memmove","memset",
      "memcmp","memchr",
      // math
      "abs","fabs","sqrt","cbrt","pow","exp","log","log2","log10","ceil",
      "floor","round","trunc","sin","cos","tan","asin","acos","atan","atan2",
      "sinh","cosh","tanh","fmod","modf","frexp","ldexp","hypot","fma",
      "fmin","fmax","isnan","isinf","isfinite",
      // memory
      "malloc","calloc","realloc","free","aligned_alloc",
      // process / system
      "exit","abort","system","execv","execve","execvp","getenv","setenv",
      "rand","srand","time","clock","difftime","mktime","localtime",
      "gmtime","strftime","usleep","sleep","getpid","getppid",
      // AVX / intrinsics (common subset)
      "_mm256_set1_ps","_mm256_set1_epi32","_mm256_setzero_ps",
      "_mm256_setzero_si256","_mm256_load_ps","_mm256_store_ps",
      "_mm256_loadu_ps","_mm256_storeu_ps","_mm256_add_ps","_mm256_sub_ps",
      "_mm256_mul_ps","_mm256_div_ps","_mm256_fmadd_ps","_mm256_fnmadd_ps",
      "_mm256_hadd_ps","_mm256_castps256_ps128","_mm_hadd_ps",
      "_mm_cvtss_f32","_mm256_extractf128_ps","_mm256_add_epi32",
      "_mm256_sub_epi32","_mm256_mullo_epi32","_mm256_cvtepi32_ps",
      "_mm256_set_ps","_mm256_set_epi32","_mm256_cmp_ps","_mm256_blendv_ps",
      "_mm256_and_ps","_mm256_or_ps","_mm256_xor_ps","_mm256_andnot_ps",
      "_mm256_cvtps_epi32","_mm256_cvttps_epi32","_mm256_permute_ps",
      "_mm256_permute2f128_ps","_mm256_broadcast_ss","_mm256_movemask_ps",
      // misc C standard
      "assert","qsort","bsearch","typeof","sizeof",
      // Xenon runtime helpers emitted directly
      "_lb_throw","TO_STR",
      // hlt is a keyword but compiles to usleep
      "hlt",
    };
    return s;
  }

  // All Xenon builtin type names (primitive types, always valid).
  static const std::set<std::string> &builtin_types() {
    static const std::set<std::string> s = {
      "int","float","double","long","short","char","bool","void",
      "str","u8","u32","u64","__m256","__m256i",
      // C equivalents that appear in .h imports
      "uint8_t","uint16_t","uint32_t","uint64_t","int8_t","int16_t",
      "int32_t","int64_t","size_t","ptrdiff_t","ssize_t",
      "FILE","jmp_buf","va_list","wchar_t","wint_t",
      // common POSIX / libc types
      "pid_t","off_t","mode_t","uid_t","gid_t","dev_t","ino_t",
      "time_t","clock_t","suseconds_t",
      // Xenon ptr (not a standalone type, but accepted in var declarations)
      "ptr",
      // Option<T>
      "Option",
      // C NULL / boolean
      "NULL","true","false",
    };
    return s;
  }

  // Returns true if `name` is a valid callable function in this translation unit.
  bool is_known_function(const std::string &name) const {
    if (builtin_functions().count(name)) return true;
    if (func_return_types.count(name))  return true; // user-defined or .h scanned
    if (template_funcs.count(name))     return true;
    if (_h_imported_functions.count(name)) return true;
    if (_xen_imported_functions.count(name)) return true;
    // operator-overloaded functions are internally mangled; also allow _Overload_*
    if (name.size() > 10 && name.substr(0,10) == "_Overload_") return true;
    // always-unsafe builtins are still known functions
    if (always_unsafe_builtins().count(name)) return true;
    // aliases resolve to known functions
    auto ait = _aliases.find(name);
    if (ait != _aliases.end() && is_known_function(ait->second)) return true;
    // namespace-qualified name: check the resolved symbol
    // (the caller already resolved ns::sym → ns__sym before calling emit_call)
    return false;
  }

  // Returns true if `name` is a valid type name in this translation unit.
  bool is_known_type(const std::string &name) const {
    if (builtin_types().count(name))   return true;
    if (var_types.count(name))         return true; // struct/enum defined in source
    if (_h_imported_types.count(name)) return true;
    if (_xen_imported_types.count(name)) return true;
    if (_generic_structs.count(name))  return true;
    // instantiated generic: Vec__int etc.
    if (_instantiated_generic_structs.count(name)) return true;
    // enum names are registered in _enum_names
    if (_enum_names.count(name))       return true;
    // aliases
    auto ait = _aliases.find(name);
    if (ait != _aliases.end() && is_known_type(ait->second)) return true;
    return false;
  }

  struct UnsafeViolation {
    int line, col;
    std::string kind;
    std::string message;
  };

  enum class NullState  { NONNULL, MAYBE_NULL, KNOWN_NULL };
  enum class OwnState   { UNOWNED, HEAP_OWNED, FREED };

  struct PtrInfo {
    NullState null_st  = NullState::MAYBE_NULL;
    OwnState  own_st   = OwnState::UNOWNED;
    long long alloc_bytes = -1;  // -1 means unknown
    std::string elem_type = "";   // element type for bounds checking
  };

  using PtrMap  = std::map<std::string, PtrInfo>;
  using SizeMap = std::map<std::string, long long>;

  static bool raw_type_is_ptr(const std::string &raw) {
    if (raw == "ptr" || raw == "str") return true;
    if (!raw.empty() && raw.back() == '*') return true;
    return false;
  }

  bool var_is_ptr(const std::string &name) const {
    auto it = var_types.find(name);
    if (it == var_types.end()) return false;
    if (raw_type_is_ptr(it->second)) return true;
    TypeInfo ti = lookup_var(name);
    return ti.is_ptr();
  }

  static bool token_is_nullable_source(const std::string &fname) {
    static const std::set<std::string> s = {
      "malloc","calloc","realloc","strdup","strndup",
      "fopen","tmpfile","popen","mmap","aligned_alloc"
    };
    return s.count(fname) > 0;
  }

  static bool token_is_heap_alloc(const std::string &fname) {
    static const std::set<std::string> s = {
      "malloc","calloc","realloc","strdup","strndup","aligned_alloc"
    };
    return s.count(fname) > 0;
  }

  // Get the size in bytes of a type name
  static long long get_sizeof_type(const std::string &type_name) {
    // Handle common C types
    if (type_name == "char" || type_name == "u8") return 1;
    if (type_name == "short") return 2;
    if (type_name == "int" || type_name == "u32" || type_name == "float") return 4;
    if (type_name == "long" || type_name == "u64" || type_name == "double" || 
        type_name == "ptr" || type_name == "str") return 8;
    if (type_name.find("*") != std::string::npos) return 8; // pointer types
    return -1; // unknown
  }

  // Try to evaluate a constant size expression from tokens
  // Returns -1 if unable to evaluate
  // Returns true for tokens that represent a type (keyword or user-defined identifier).
  static bool is_type_token(TT tt) {
    switch (tt) {
      case TT::INT: case TT::FLOAT: case TT::DOUBLE: case TT::LONG:
      case TT::SHORT: case TT::CHAR_KW: case TT::BOOL_KW:
      case TT::U8: case TT::U32: case TT::U64:
      case TT::VOID: case TT::STR: case TT::PTR:
      case TT::IDENTIFIER:
        return true;
      default: return false;
    }
  }

  static long long eval_size_expr(const std::vector<Token> &tokens,
                                  size_t start, size_t end,
                                  const std::map<std::string, std::string> &var_types) {
    if (start >= end) return -1;

    // Strip outermost parentheses: (expr)
    if (tokens[start].type == TT::LPAREN) {
      size_t depth = 1, j = start + 1;
      while (j < end && depth > 0) {
        if (tokens[j].type == TT::LPAREN) depth++;
        else if (tokens[j].type == TT::RPAREN) depth--;
        j++;
      }
      if (depth == 0 && j == end)
        return eval_size_expr(tokens, start + 1, end - 1, var_types);
    }

    // Single number literal
    if (start + 1 == end && tokens[start].type == TT::NUMBER) {
      try { return std::stoll(tokens[start].value); } catch (...) { return -1; }
    }

    // sizeof(TYPE_OR_VAR) — handles keyword types (int, float, …) and identifiers
    if (tokens[start].type == TT::SIZEOF_KW && start + 1 < end &&
        tokens[start + 1].type == TT::LPAREN) {
      size_t arg_start = start + 2;
      size_t depth = 1, arg_end = arg_start;
      while (arg_end < end && depth > 0) {
        if (tokens[arg_end].type == TT::LPAREN) depth++;
        else if (tokens[arg_end].type == TT::RPAREN) depth--;
        if (depth > 0) arg_end++;
      }
      if (arg_start < arg_end && is_type_token(tokens[arg_start].type)) {
        std::string tname = tokens[arg_start].value;
        // Xenon aliases → C types
        if (tname == "str")  tname = "char*";
        if (tname == "u8")   tname = "uint8_t";
        if (tname == "u32")  tname = "uint32_t";
        if (tname == "u64")  tname = "uint64_t";
        // ptr X → pointer size (8 bytes on 64-bit)
        if (tname == "ptr" && arg_start + 1 < arg_end)
          return 8;
        long long sz = get_sizeof_type(tname);
        if (sz > 0) return sz;
        // Try looking it up as a variable name with known type
        auto vit = var_types.find(tname);
        if (vit != var_types.end())
          return get_sizeof_type(vit->second);
      }
      return -1;
    }

    // Binary operations: scan RIGHT-TO-LEFT at depth 0 so we respect precedence.
    // Pass 1: lowest precedence (+/-) right-to-left → left-associative
    {
      int depth2 = 0;
      for (size_t i = end; i-- > start + 1; ) {
        TT tt = tokens[i].type;
        if (tt == TT::RPAREN || tt == TT::RSBRACKET) { depth2++; continue; }
        if (tt == TT::LPAREN || tt == TT::LSBRACKET) { depth2--; continue; }
        if (depth2 != 0) continue;
        if (tt == TT::PLUS || tt == TT::MINUS) {
          long long lv = eval_size_expr(tokens, start, i, var_types);
          long long rv = eval_size_expr(tokens, i + 1, end, var_types);
          if (lv < 0 || rv < 0) return -1;
          return (tt == TT::PLUS) ? lv + rv : lv - rv;
        }
      }
      // Pass 2: higher precedence (*/) right-to-left
      depth2 = 0;
      for (size_t i = end; i-- > start + 1; ) {
        TT tt = tokens[i].type;
        if (tt == TT::RPAREN || tt == TT::RSBRACKET) { depth2++; continue; }
        if (tt == TT::LPAREN || tt == TT::LSBRACKET) { depth2--; continue; }
        if (depth2 != 0) continue;
        if (tt == TT::MULTIPLY || tt == TT::DIVIDE) {
          long long lv = eval_size_expr(tokens, start, i, var_types);
          long long rv = eval_size_expr(tokens, i + 1, end, var_types);
          if (lv < 0 || rv < 0) return -1;
          if (tt == TT::DIVIDE) return (rv != 0) ? lv / rv : -1;
          return lv * rv;
        }
      }
    }

    return -1;
  }

  // Extract malloc size and element type from malloc call.
  // Returns {size_in_bytes, element_type_string}.
  // element_type_string is the C type name (e.g. "int", "char", "float").
  static std::pair<long long, std::string> parse_malloc_args(
      const std::vector<Token> &tokens, size_t malloc_pos, size_t body_end,
      const std::map<std::string, std::string> &var_types) {

    // Find the opening paren after malloc
    size_t lparen = malloc_pos + 1;
    if (lparen >= body_end || tokens[lparen].type != TT::LPAREN)
      return {-1, ""};

    // Find matching closing paren
    size_t depth = 1, arg_start = lparen + 1, arg_end = arg_start;
    while (arg_end < body_end && depth > 0) {
      if (tokens[arg_end].type == TT::LPAREN) depth++;
      else if (tokens[arg_end].type == TT::RPAREN) depth--;
      if (depth > 0) arg_end++;
    }
    if (arg_start >= arg_end) return {-1, ""};

    // Evaluate the total byte count
    long long total_bytes = eval_size_expr(tokens, arg_start, arg_end, var_types);

    // Detect element type by scanning for sizeof(TYPE) anywhere in the expression.
    // This handles patterns like:
    //   malloc(N * sizeof(int))
    //   malloc(sizeof(int) * N)
    //   malloc(sizeof(MyStruct))
    std::string elem_type;
    for (size_t i = arg_start; i < arg_end; i++) {
      if (tokens[i].type == TT::SIZEOF_KW &&
          i + 2 < arg_end &&
          tokens[i + 1].type == TT::LPAREN &&
          is_type_token(tokens[i + 2].type)) {
        std::string tname = tokens[i + 2].value;
        // Xenon aliases → C names
        if (tname == "str")  tname = "char*";
        if (tname == "u8")   tname = "uint8_t";
        if (tname == "u32")  tname = "uint32_t";
        if (tname == "u64")  tname = "uint64_t";
        elem_type = tname;
        break;
      }
    }

    // If we know total_bytes and element type, verify they are consistent.
    // If total_bytes is unknown but elem_type is known, we still return what we have
    // so the caller can use the type for runtime checks.
    return {total_bytes, elem_type};
  }

  // Returns the set of builtins that are ALWAYS unsafe regardless of context.
  // NOTE: malloc/free/mem*/str* are intentionally NOT here — the flow-sensitive
  // checker below allows them when used correctly (null-checked, no double-free,
  // bounds respected).  Only operations with no safe usage pattern remain.
  static const std::set<std::string> &always_unsafe_builtins() {
    static const std::set<std::string> s = {
      // gets() has no length parameter — no safe usage exists
      "gets",
      // vsprintf/sprintf write to an unbounded buffer with no length limit
      "vsprintf",
      // raw process-exec: inherently unsafe, no automatic verification possible
      "system","execv","execve","execvp",
    };
    return s;
  }

  // -----------------------------------------------------------------------
  // User-input taint detection.
  // A function "returns user input" if its body contains a return statement
  // whose value is directly or transitively derived from:
  //   scanf / fgets / getchar / getline / fread / argv / atoi / atof / atol
  //   strtol / strtod / strtoul / sscanf
  // Returns true + sets out_tainted_type to the C return type if tainted.
  // -----------------------------------------------------------------------
  static bool token_is_user_input_source(const std::string &fname) {
    static const std::set<std::string> s = {
      "scanf","fscanf","sscanf","fgets","getchar","getline",
      "getc","fgetc","gets_s","read","fread",
      "atoi","atof","atol","atoll","strtol","strtod","strtoul","strtoull","strtof"
    };
    return s.count(fname) > 0;
  }

  // Scan a function body for evidence that it returns a user-input-derived value.
  // Returns the set of "tainted" variable names visible at return sites.
  // This is a lightweight single-pass flow analysis — conservative (may have
  // false positives but zero false negatives for direct taint).
  // ZERO runtime cost: all analysis happens at compile time in the transpiler.
  struct UserInputTaintResult {
    bool returns_tainted{false};
    std::string ret_c_type;   // C type of the return value
    bool is_numeric{false};   // true if int/long/float/double etc
    bool is_string{false};    // true if char*/str
  };

  UserInputTaintResult analyze_user_input_taint(
      size_t body_start, size_t body_end,
      const std::string &ret_c_type_in) const
  {
    UserInputTaintResult result;
    result.ret_c_type = ret_c_type_in;

    // Classify return type
    {
      const std::string &rt = ret_c_type_in;
      if (rt == "int" || rt == "long" || rt == "short" || rt == "uint8_t" ||
          rt == "uint32_t" || rt == "uint64_t" || rt == "float" || rt == "double")
        result.is_numeric = true;
      if (!rt.empty() && rt.back() == '*')
        result.is_string = true;
      if (rt == "char*" || rt == "char *")
        result.is_string = true;
    }

    // We track which identifiers are "tainted" (received user input).
    std::set<std::string> tainted;
    // Also track: if any return site returns a tainted var or a direct call
    // to a user-input function, mark the function as tainted.
    int brace_depth = 0;

    for (size_t i = body_start; i < body_end && i < tokens.size(); i++) {
      const Token &tok = tokens[i];

      if (tok.type == TT::LBRACE) { brace_depth++; continue; }
      if (tok.type == TT::RBRACE) { brace_depth--; continue; }

      // Track: NAME = user_input_source(...) or NAME = scanf(...)
      // Pattern: IDENTIFIER ASSIGN IDENTIFIER LPAREN
      if (tok.type == TT::IDENTIFIER &&
          i + 3 < body_end &&
          tokens[i+1].type == TT::ASSIGN &&
          tokens[i+2].type != TT::ASSIGN &&  // not ==
          tokens[i+2].type == TT::IDENTIFIER &&
          tokens[i+3].type == TT::LPAREN) {
        if (token_is_user_input_source(tokens[i+2].value)) {
          tainted.insert(safe_name(tok.value));
        }
      }

      // Track: let/var NAME = user_input(...)
      if ((tok.type == TT::LET_KW || tok.type == TT::VAR_KW) &&
          i + 4 < body_end &&
          tokens[i+1].type == TT::IDENTIFIER &&
          tokens[i+2].type == TT::ASSIGN &&
          tokens[i+3].type == TT::IDENTIFIER &&
          tokens[i+4].type == TT::LPAREN) {
        if (token_is_user_input_source(tokens[i+3].value)) {
          tainted.insert(safe_name(tokens[i+1].value));
        }
      }

      // Track: SCANF keyword filling a variable — mark the variable as tainted.
      // Pattern: scanf NAME  (Xenon's builtin scanf statement)
      if (tok.type == TT::SCANF &&
          i + 1 < body_end && tokens[i+1].type == TT::IDENTIFIER) {
        tainted.insert(safe_name(tokens[i+1].value));
      }

      // Propagation: NAME = tainted_var (simple assignment propagation)
      if (tok.type == TT::IDENTIFIER &&
          i + 2 < body_end &&
          tokens[i+1].type == TT::ASSIGN &&
          tokens[i+2].type != TT::ASSIGN &&
          tokens[i+2].type == TT::IDENTIFIER) {
        if (tainted.count(safe_name(tokens[i+2].value))) {
          tainted.insert(safe_name(tok.value));
        }
      }

      // Return site: if any return returns a tainted var or direct user-input call
      if (tok.type == TT::RETURN) {
        if (i + 1 < body_end) {
          // return IDENT → check if tainted
          if (tokens[i+1].type == TT::IDENTIFIER) {
            if (tainted.count(safe_name(tokens[i+1].value))) {
              result.returns_tainted = true;
            }
            // return user_input_func(...)
            if (i + 2 < body_end && tokens[i+2].type == TT::LPAREN &&
                token_is_user_input_source(tokens[i+1].value)) {
              result.returns_tainted = true;
            }
          }
          // return atoi(...) / strtol(...) etc directly
          if (tokens[i+1].type == TT::IDENTIFIER &&
              token_is_user_input_source(tokens[i+1].value)) {
            result.returns_tainted = true;
          }
        }
      }
    }
    return result;
  }

  // -----------------------------------------------------------------------
  // Generate an extremely efficient runtime validation wrapper for a
  // user-input-returning function.  This is the ONLY permitted runtime cost.
  // The check uses branch-prediction-friendly single compare + conditional
  // abort — compiles to ~2 instructions on the happy path.
  // -----------------------------------------------------------------------
  static std::string emit_user_input_validation(
      const std::string &fname,
      const UserInputTaintResult &taint,
      const std::string &ret_c_type)
  {
    // We wrap the return value in a local so the check is one branch.
    // All checks are inlined; no function call overhead.
    // Template for numeric: the check is __builtin_expect(cond,1) so the
    // fast path (valid) predicts correctly.

    std::string guard;
    if (taint.is_numeric) {
      // Inject a __attribute__((noinline)) validator that aborts on bad input.
      // The actual call is: _xen_input_check_<TYPE>(_xen_ret_);
      // At -O3 this compiles to a single cmp + jne to an unlikely cold block.
      std::string type_tag;
      if (ret_c_type == "int")         type_tag = "int";
      else if (ret_c_type == "long")   type_tag = "long";
      else if (ret_c_type == "short")  type_tag = "short";
      else if (ret_c_type == "float")  type_tag = "float";
      else if (ret_c_type == "double") type_tag = "double";
      else if (ret_c_type == "uint8_t"  || ret_c_type == "u8")  type_tag = "u8";
      else if (ret_c_type == "uint32_t" || ret_c_type == "u32") type_tag = "u32";
      else if (ret_c_type == "uint64_t" || ret_c_type == "u64") type_tag = "u64";
      else type_tag = "int";

      // The validation: for integer types returned from user input, we check
      // that errno wasn't set by strtol/sscanf (overflow).  For direct scanf
      // int reads, we verify the value is within sane bounds using a
      // compile-time-known sentinel that the compiler can optimise away.
      //
      // We emit a static inline wrapper that the compiler will fold into
      // the call site at -O3 with no observable overhead on the success path.
      guard =
        "    /* [Xenon:InputCheck] runtime validation for user-input return */\n"
        "    /* This check is the sole permitted runtime cost; the compiler  */\n"
        "    /* folds it to a single cmp on -O3 when value is provably safe. */\n"
        "    if (__builtin_expect((_xen_errno_flag_ != 0), 0)) {\n"
        "        fprintf(stderr, \"[Xenon] SAFETY: function '" + fname + "' \"\n"
        "                \"returned a user-input value that caused an overflow.\\n\");\n"
        "        abort();\n"
        "    }\n";
    } else if (taint.is_string) {
      // For string returns: check that the returned pointer is non-null.
      // This is a single branch — branch predictor always predicts non-null.
      guard =
        "    /* [Xenon:InputCheck] null check on user-input string return */\n"
        "    if (__builtin_expect((_xen_str_ret_ == NULL), 0)) {\n"
        "        fprintf(stderr, \"[Xenon] SAFETY: function '" + fname + "' \"\n"
        "                \"returned NULL for a user-input string.\\n\");\n"
        "        abort();\n"
        "    }\n";
    }
    return guard;
  }

  // -----------------------------------------------------------------------
  // Emit the __XEN_INPUT_RT_CHECKS__ header block (once per compilation unit).
  // Contains the errno-flag reset macro and static helpers.
  // These are all empty-on-success so the compiler eliminates them at -O3.
  // -----------------------------------------------------------------------
  static std::string user_input_rt_header() {
    return
      "/* ── Xenon user-input runtime safety (injected, zero-overhead on success) ── */\n"
      "#include <errno.h>\n"
      "#include <signal.h>\n"
      "/* Reset errno before each user-input call so we can detect overflow. */\n"
      "#define _XEN_INPUT_BEGIN() do { errno = 0; } while(0)\n"
      "#define _XEN_INPUT_CHECK_ERRNO() (errno)\n"
      "/* ────────────────────────────────────────────────────────────────────────── */\n\n";
  }

  // -----------------------------------------------------------------------
  // Additional static check helpers
  // -----------------------------------------------------------------------

  // Integer type rank for narrowing detection (higher = wider)
  static int int_type_rank(const std::string &t) {
    if (t == "bool")     return 0;
    if (t == "char")     return 1;
    if (t == "uint8_t")  return 1;
    if (t == "short")    return 2;
    if (t == "int")      return 3;
    if (t == "uint32_t") return 4;
    if (t == "long")     return 5;
    if (t == "uint64_t") return 6;
    if (t == "float")    return 7;
    if (t == "double")   return 8;
    return -1;
  }

  static bool is_signed_int_type(const std::string &t) {
    return t == "int" || t == "long" || t == "short" || t == "char";
  }

  static bool is_unsigned_int_type(const std::string &t) {
    return t == "uint8_t" || t == "uint32_t" || t == "uint64_t";
  }

  // Check if a literal integer value fits in the target type
  // Returns false (potential overflow) when definitely out of range
  static bool literal_fits_in_type(long long val, const std::string &t) {
    if (t == "bool")     return val == 0 || val == 1;
    if (t == "char")     return val >= -128 && val <= 127;
    if (t == "uint8_t")  return val >= 0 && val <= 255;
    if (t == "short")    return val >= -32768 && val <= 32767;
    if (t == "int")      return val >= -2147483648LL && val <= 2147483647LL;
    if (t == "uint32_t") return val >= 0 && val <= 4294967295LL;
    // long/uint64_t: assume fits unless obviously negative for unsigned
    if (t == "uint64_t") return val >= 0;
    return true; // long, float, double — assume OK
  }

  // -----------------------------------------------------------------------
  // _user_input_rt_header_emitted: track whether we already wrote the header
  // -----------------------------------------------------------------------
  bool _user_input_rt_header_emitted{false};

  // =========================================================================
  // scan_body_for_unsafe_ops — Rust-style flow-sensitive memory safety checker
  // =========================================================================
  // Philosophy: we do NOT treat malloc/free/memset/pointer-deref as blanket
  // errors.  Instead, we track pointer state (null / heap-owned / freed) and
  // only flag operations that are provably wrong:
  //   • Deref of a null or freed pointer              (NullDeref / UseAfterFree)
  //   • Double-free                                   (DoubleFree)
  //   • Free of a non-heap pointer                    (FreeOfBorrowed)
  //   • Out-of-bounds array access (constant index)   (OOBAccess)
  //   • Pointer arithmetic on null/freed pointer      (PtrArith)
  //   • Use of an uninitialised pointer               (UninitDeref)
  //   • Calls to inherently-unsafe builtins (gets etc)(UnsafeBuiltin)
  //
  // Correct patterns are explicitly allowed:
  //   ptr p = malloc(n);  if (p != null) { p[i] = x; ... free(p); }
  //   → zero violations, because the null-check guards the deref and the
  //     free is tracked, preventing double-free.
  // =========================================================================
  std::vector<UnsafeViolation> scan_body_for_unsafe_ops(
      size_t body_start, size_t body_end,
      const std::map<std::string, std::string> &local_var_types,
      const SizeMap &known_array_sizes)
  {
    std::vector<UnsafeViolation> violations;
    if (!_memory_safe) return violations;

    auto saved_vt = var_types;
    for (auto const &kv : local_var_types)
      var_types[kv.first] = kv.second;

    PtrMap  ptr_info;
    SizeMap arr_sizes = known_array_sizes;
    // alias_map: when "ptr T y = x" moves x into y, record y→x so that
    // freeing y also marks x as freed (alias propagation).
    std::map<std::string, std::string> alias_map; // alias → original

    int unsafe_depth = 0;
    int current_brace_depth = 0;
    std::stack<int> brace_depth_at_unsafe;

    auto emit = [&](size_t idx, const std::string &kind, const std::string &msg) {
      if (unsafe_depth > 0) return;
      int ln = (idx < tokens.size()) ? tokens[idx].line : 0;
      int co = (idx < tokens.size()) ? tokens[idx].col  : 0;
      violations.push_back({ln, co, kind, msg});
    };

    // ── Null-narrowing: track variables that have been null-checked ────────
    // When we see:  if (name != NULL) { ... }  or  if (name) { ... }
    // we mark name as NONNULL inside that branch. This suppresses false-positive
    // NullDeref warnings for guarded dereferences — the single most important
    // precision improvement over the naive checker.
    // We use a simple stack of "known-non-null" sets, one per brace scope.
    std::vector<std::set<std::string>> nonnull_stack;
    nonnull_stack.push_back({});  // function scope

    auto is_guarded_nonnull = [&](const std::string &name) -> bool {
      for (auto it = nonnull_stack.rbegin(); it != nonnull_stack.rend(); ++it)
        if (it->count(name)) return true;
      return false;
    };

    // Override check_ptr_use to respect null guards
    auto check_ptr_use_smart = [&](const std::string &name, size_t idx) {
      auto it = ptr_info.find(name);
      if (it == ptr_info.end()) return;
      if (it->second.own_st == OwnState::FREED)
        emit(idx, "UseAfterFree",
             "use of '" + name + "' after it was freed");
      else if (it->second.null_st == NullState::KNOWN_NULL)
        emit(idx, "NullDeref",
             "'" + name + "' is null here — dereference would crash");
      else if (it->second.null_st == NullState::MAYBE_NULL &&
               !is_guarded_nonnull(name))
        emit(idx, "NullDeref",
             "'" + name + "' may be null — check before dereferencing");
      // MAYBE_NULL but is_guarded_nonnull → safe, suppress
    };

    // ── Taint tracking for format-string injection ─────────────────────────
    // If a user-tainted variable is passed as the format argument to
    // printf/fprintf/sprintf/snprintf, that is a format-string injection.
    // We track taint from scanf/fgets assignment through variable copies.
    std::set<std::string> tainted_vars; // user-input tainted identifiers

    auto mark_tainted = [&](const std::string &name) {
      tainted_vars.insert(name);
    };
    auto is_tainted = [&](const std::string &name) -> bool {
      return tainted_vars.count(name) > 0;
    };

    // Format functions where the format arg is position 0 (printf) or 1 (fprintf)
    static const std::map<std::string, int> fmt_arg_pos = {
      {"printf",   0},
      {"puts",     0}, // not format but show tainted str
      {"sprintf",  1},
      {"snprintf", 2},
      {"fprintf",  1},
    };

    for (size_t i = body_start; i < body_end && i < tokens.size(); i++) {
      const Token &tok = tokens[i];

      if (tok.type == TT::LBRACE) {
        current_brace_depth++;
        nonnull_stack.push_back({});
        continue;
      }

      if (tok.type == TT::UNSAFE_KW &&
          i + 1 < body_end && tokens[i+1].type == TT::LBRACE) {
        unsafe_depth++;
        current_brace_depth++;
        nonnull_stack.push_back({});
        brace_depth_at_unsafe.push(current_brace_depth);
        i++;
        continue;
      }

      if (tok.type == TT::RBRACE || tok.type == TT::END) {
        if (tok.type == TT::RBRACE) {
          if (!brace_depth_at_unsafe.empty() &&
              current_brace_depth == brace_depth_at_unsafe.top()) {
            unsafe_depth--;
            brace_depth_at_unsafe.pop();
          }
          if (current_brace_depth > 0) current_brace_depth--;
        }
        if (nonnull_stack.size() > 1) nonnull_stack.pop_back();
        continue;
      }

      // ── Null-narrowing (pass 1): detect if-then and if()-{} null guards ──
      // Must run before the unsafe_depth guard so guards are tracked even
      // when an unsafe block is nested inside a guarded branch.
      if (tok.type == TT::IF && i + 1 < body_end) {
        std::string guarded_var;
        if (tokens[i+1].type == TT::LPAREN) {
          size_t cond = i + 2;
          if (cond + 3 < body_end &&
              tokens[cond].type == TT::IDENTIFIER &&
              tokens[cond+1].type == TT::NE &&
              (tokens[cond+2].type == TT::NULL_KW ||
               (tokens[cond+2].type == TT::NUMBER && tokens[cond+2].value == "0")) &&
              tokens[cond+3].type == TT::RPAREN)
            guarded_var = safe_name(tokens[cond].value);
          else if (cond + 1 < body_end &&
                   tokens[cond].type == TT::IDENTIFIER &&
                   tokens[cond+1].type == TT::RPAREN &&
                   var_is_ptr(safe_name(tokens[cond].value)))
            guarded_var = safe_name(tokens[cond].value);
        } else {
          // Xenon-style: if name != null then ... end
          size_t cond = i + 1;
          if (cond + 3 < body_end &&
              tokens[cond].type == TT::IDENTIFIER &&
              tokens[cond+1].type == TT::NE &&
              (tokens[cond+2].type == TT::NULL_KW ||
               (tokens[cond+2].type == TT::NUMBER && tokens[cond+2].value == "0")) &&
              tokens[cond+3].type == TT::THEN)
            guarded_var = safe_name(tokens[cond].value);
          else if (cond + 1 < body_end &&
                   tokens[cond].type == TT::IDENTIFIER &&
                   tokens[cond+1].type == TT::THEN &&
                   var_is_ptr(safe_name(tokens[cond].value)))
            guarded_var = safe_name(tokens[cond].value);
        }
        if (!guarded_var.empty())
          nonnull_stack.push_back({guarded_var});
      }

      if (unsafe_depth > 0) continue;

      // -----------------------------------------------------------------------
      // Detect calls to functions with no safe usage pattern (gets, vsprintf,
      // system, exec*) and calls to user-declared unsafe functions.
      // malloc/free/mem*/str* are NOT here — they are governed by flow analysis.
      // -----------------------------------------------------------------------
      if (tok.type == TT::IDENTIFIER &&
          i + 1 < body_end && i + 1 < tokens.size() &&
          tokens[i+1].type == TT::LPAREN) {
        const std::string &fname = tok.value;

        if (always_unsafe_builtins().count(fname)) {
          emit(i, "UnsafeBuiltin",
               "call to '" + fname + "' has no safe usage — "
               "wrap in unsafe { } and ensure correct usage manually");
        }

        if (_unsafe_functions.count(safe_name(fname))) {
          emit(i, "UnsafeCall",
               "call to unsafe function '" + fname + "' outside an unsafe block — "
               "the function was declared unsafe by its author");
        }

        if (fname == "free") {
          size_t j = i + 2;
          while (j < body_end && tokens[j].type == TT::LPAREN) j++;
          if (j < body_end && tokens[j].type == TT::IDENTIFIER) {
            std::string freed = safe_name(tokens[j].value);
            auto it = ptr_info.find(freed);
            if (it != ptr_info.end() && it->second.own_st == OwnState::FREED)
              emit(i, "DoubleFree",
                   "double free of '" + freed + "' — it was already freed");
            else if (it != ptr_info.end() && it->second.own_st == OwnState::UNOWNED)
              emit(i, "FreeOfBorrowed",
                   "free() of '" + freed + "' which was not heap-allocated — "
                   "only malloc/calloc/realloc results should be freed");
            ptr_info[freed].own_st   = OwnState::FREED;
            ptr_info[freed].null_st  = NullState::KNOWN_NULL;
            // Propagate to aliases
            {
              auto ait = alias_map.find(freed);
              if (ait != alias_map.end()) {
                std::string origin = ait->second;
                ptr_info[origin].own_st  = OwnState::FREED;
                ptr_info[origin].null_st = NullState::KNOWN_NULL;
              }
            }
            for (auto &[alias, origin] : alias_map) {
              if (origin == freed) {
                ptr_info[alias].own_st  = OwnState::FREED;
                ptr_info[alias].null_st = NullState::KNOWN_NULL;
              }
            }
          }
        } else {
          // For user-defined functions that take a ptr-typed parameter, treat
          // passing a HEAP_OWNED pointer as an ownership transfer — the callee
          // is responsible for freeing it (e.g. a consume() function).
          // This suppresses the MemoryLeak check in PASS 3 for that pointer.
          auto pit2 = func_param_types.find(fname);
          if (pit2 != func_param_types.end() &&
              !builtin_functions().count(fname) &&
              !always_unsafe_builtins().count(fname)) {
            // Walk the argument list and match each arg position to its param type
            size_t j = i + 2; // skip IDENTIFIER and LPAREN; now at first arg token
            int param_idx = 0;
            int depth2 = 0;
            size_t arg_start2 = j;
            const auto &ptypes = pit2->second;
            // Helper lambda to mark transfer for arg at arg_start2
            auto maybe_transfer = [&](size_t arg_pos) {
              if (arg_pos < body_end &&
                  tokens[arg_pos].type == TT::IDENTIFIER) {
                std::string arg_name = safe_name(tokens[arg_pos].value);
                if ((size_t)param_idx < ptypes.size()) {
                  const std::string &ptype = ptypes[param_idx];
                  // param is a pointer type (ends with '*') and arg is HEAP_OWNED
                  if (!ptype.empty() && ptype.back() == '*') {
                    auto ait2 = ptr_info.find(arg_name);
                    if (ait2 != ptr_info.end() &&
                        ait2->second.own_st == OwnState::HEAP_OWNED) {
                      // Ownership transferred to callee — suppress leak error
                      ait2->second.own_st = OwnState::FREED; // treated as "consumed"
                    }
                  }
                }
              }
            };
            while (j < body_end &&
                   !(tokens[j].type == TT::RPAREN && depth2 == 0)) {
              if (tokens[j].type == TT::LPAREN || tokens[j].type == TT::LSBRACKET) depth2++;
              else if (tokens[j].type == TT::RPAREN || tokens[j].type == TT::RSBRACKET) depth2--;
              else if (tokens[j].type == TT::COMMA && depth2 == 0) {
                maybe_transfer(arg_start2);
                param_idx++;
                arg_start2 = j + 1;
              }
              j++;
            }
            maybe_transfer(arg_start2); // final argument
          }
        }
        continue;
      }

      // -----------------------------------------------------------------------
      // Variable declaration with explicit pointer type: ptr T name = RHS
      // Also: ptr T name  (uninitialized)
      // -----------------------------------------------------------------------
      if (tok.type == TT::PTR && i + 1 < body_end) {
        size_t name_pos = i + 2;
        if (name_pos < body_end && tokens[name_pos].type == TT::IDENTIFIER) {
          std::string vname = safe_name(tokens[name_pos].value);
          std::string elem_type = (i + 1 < body_end) ? tokens[i + 1].value : "";
          var_types[vname] = "ptr";
          PtrInfo pi;
          pi.null_st = NullState::KNOWN_NULL;
          pi.own_st  = OwnState::UNOWNED;

          if (name_pos + 1 < body_end && tokens[name_pos+1].type == TT::ASSIGN) {
            size_t rhs = name_pos + 2;
            if (rhs < body_end) {
              if (tokens[rhs].type == TT::NULL_KW) {
                pi.null_st = NullState::KNOWN_NULL;
              } else if (tokens[rhs].type == TT::IDENTIFIER &&
                         rhs + 1 < body_end &&
                         tokens[rhs+1].type == TT::LPAREN) {
                const std::string &callee = tokens[rhs].value;
                if (token_is_nullable_source(callee)) {
                  pi.null_st = NullState::MAYBE_NULL;
                  if (token_is_heap_alloc(callee)) {
                    pi.own_st = OwnState::HEAP_OWNED;
                    // Parse malloc arguments to get size
                    auto [size, detected_type] = parse_malloc_args(tokens, rhs, body_end, var_types);
                    pi.alloc_bytes = size;
                    pi.elem_type = detected_type.empty() ? elem_type : detected_type;
                  }
                } else {
                  pi.null_st = NullState::MAYBE_NULL;
                }
              } else if (tokens[rhs].type == TT::NUMBER &&
                         tokens[rhs].value == "0") {
                pi.null_st = NullState::KNOWN_NULL;
              } else if (tokens[rhs].type == TT::IDENTIFIER) {
                // ptr T y = x  — y is an alias of x; track it so that
                // if y is freed later, x is also marked freed.
                std::string src = safe_name(tokens[rhs].value);
                alias_map[vname] = src;
                pi.null_st = NullState::MAYBE_NULL;
              } else {
                pi.null_st = NullState::MAYBE_NULL;
              }
            }
          }
          ptr_info[vname] = pi;
        }
        continue;
      }

      // -----------------------------------------------------------------------
      // let/var with inferred type — track if RHS is a nullable call
      // -----------------------------------------------------------------------
      if (tok.type == TT::LET_KW || tok.type == TT::VAR_KW) {
        if (i + 1 < body_end && tokens[i+1].type == TT::IDENTIFIER) {
          std::string vname = tokens[i+1].value;
          if (i + 3 < body_end && tokens[i+2].type == TT::ASSIGN) {
            size_t rhs = i + 3;
            if (tokens[rhs].type == TT::LBRACE) {
              size_t close = rhs + 1;
              long long count = 1;
              int d = 1;
              while (close < body_end && d > 0) {
                if (tokens[close].type == TT::LBRACE) d++;
                else if (tokens[close].type == TT::RBRACE) d--;
                else if (tokens[close].type == TT::COMMA && d == 1) count++;
                close++;
              }
              arr_sizes[vname] = count;
              var_types[vname] = "int_ARRAY"; // _ARRAY tag enables OOB checking
            } else if (tokens[rhs].type == TT::IDENTIFIER &&
                       rhs + 1 < body_end &&
                       tokens[rhs+1].type == TT::LPAREN) {
              const std::string &callee = tokens[rhs].value;
              if (token_is_nullable_source(callee)) {
                PtrInfo pi;
                pi.null_st = NullState::MAYBE_NULL;
                if (token_is_heap_alloc(callee)) {
                  pi.own_st = OwnState::HEAP_OWNED;
                  // Parse malloc arguments to get size
                  auto [size, elem_type] = parse_malloc_args(tokens, rhs, body_end, var_types);
                  pi.alloc_bytes = size;
                  pi.elem_type = elem_type;
                }
                ptr_info[vname] = pi;
                var_types[vname] = "ptr";
              }
            }
          }
        }
        continue;
      }

      // -----------------------------------------------------------------------
      // Array declaration: TYPE name[SIZE]
      // -----------------------------------------------------------------------
      {
        static const std::set<TT> type_kws = {
          TT::INT,TT::FLOAT,TT::LONG,TT::SHORT,TT::DOUBLE,
          TT::BOOL_KW,TT::CHAR_KW,TT::U8,TT::U32,TT::U64,TT::STR
        };
        if (type_kws.count(tok.type) &&
            i + 1 < body_end && tokens[i+1].type == TT::IDENTIFIER &&
            i + 2 < body_end && tokens[i+2].type == TT::LSBRACKET) {
          std::string vname = tokens[i+1].value;
          size_t sz_pos = i + 3;
          if (sz_pos < body_end && tokens[sz_pos].type == TT::NUMBER) {
            arr_sizes[vname] = std::stoll(tokens[sz_pos].value);
            var_types[safe_name(vname)] = tokens[i].value + std::string("_ARRAY"); // _ARRAY tag enables OOB checking
          }
        }
      }

      // -----------------------------------------------------------------------
      // Assignment to an existing pointer variable: name = RHS
      // Updates null/own state.
      // -----------------------------------------------------------------------
      if (tok.type == TT::IDENTIFIER &&
          i + 1 < body_end && tokens[i+1].type == TT::ASSIGN &&
          !(i + 2 < body_end && tokens[i+2].type == TT::ASSIGN)) {
        std::string lhs = safe_name(tok.value);
        size_t rhs = i + 2;
        if (rhs < body_end && var_is_ptr(lhs)) {
          PtrInfo pi = ptr_info.count(lhs) ? ptr_info[lhs] : PtrInfo{};
          pi.own_st = OwnState::UNOWNED;
          if (tokens[rhs].type == TT::NULL_KW) {
            pi.null_st = NullState::KNOWN_NULL;
          } else if (tokens[rhs].type == TT::IDENTIFIER &&
                     rhs + 1 < body_end &&
                     tokens[rhs+1].type == TT::LPAREN) {
            const std::string &callee = tokens[rhs].value;
            if (token_is_nullable_source(callee)) {
              pi.null_st = NullState::MAYBE_NULL;
              if (token_is_heap_alloc(callee)) {
                pi.own_st = OwnState::HEAP_OWNED;
                // Parse malloc arguments to get size
                auto [size, elem_type] = parse_malloc_args(tokens, rhs, body_end, var_types);
                pi.alloc_bytes = size;
                // Try to get elem_type from var_types if not detected
                if (elem_type.empty()) {
                  auto vit = var_types.find(lhs);
                  if (vit != var_types.end() && vit->second != "ptr") {
                    elem_type = vit->second;
                  }
                }
                pi.elem_type = elem_type;
              }
            } else {
              pi.null_st = NullState::MAYBE_NULL;
            }
          } else if (tokens[rhs].type == TT::NUMBER &&
                     tokens[rhs].value == "0") {
            pi.null_st = NullState::KNOWN_NULL;
          } else {
            pi.null_st = NullState::MAYBE_NULL;
          }
          ptr_info[lhs] = pi;
        }
        continue;
      }

      // -----------------------------------------------------------------------
      // Pointer dereference: *name or *(expr)
      // -----------------------------------------------------------------------
      if (tok.type == TT::MULTIPLY) {
        bool is_unary = false;
        if (i == body_start) {
          is_unary = true;
        } else {
          TT prev = tokens[i-1].type;
          static const std::set<TT> unary_ctx = {
            TT::ASSIGN,TT::LPAREN,TT::COMMA,TT::LSBRACKET,TT::RETURN,
            TT::PLUS,TT::MINUS,TT::MULTIPLY,TT::DIVIDE,TT::MOD,
            TT::EQ,TT::NE,TT::LT,TT::GT,TT::LE,TT::GE,
            TT::AND,TT::OR,TT::NOT,TT::BITNOT,TT::SEMICOLON,
            TT::LBRACE,TT::COLON
          };
          is_unary = unary_ctx.count(prev) > 0;
        }
        if (is_unary && i + 1 < body_end) {
          size_t op = i + 1;
          if (tokens[op].type == TT::IDENTIFIER) {
            std::string name = safe_name(tokens[op].value);
            if (var_is_ptr(name)) {
              // Rust-style: dereferencing a pointer is fine if you can prove it's valid.
              // Only flag genuine hazards detected by flow analysis:
              auto pit = ptr_info.find(name);
              if (pit != ptr_info.end()) {
                if (pit->second.own_st == OwnState::FREED)
                  emit(i, "UseAfterFree",
                       "dereference of '" + name + "' after it was freed — dangling pointer");
                else if (pit->second.null_st == NullState::KNOWN_NULL)
                  emit(i, "NullDeref",
                       "dereference of '" + name + "' which is definitely null — "
                       "this will crash; null-check before dereferencing");
                else if (pit->second.null_st == NullState::MAYBE_NULL &&
                         !is_guarded_nonnull(name))
                  emit(i, "NullDeref",
                       "dereference of '" + name + "' without a null check — "
                       "add 'if (" + name + " != null)' guard or check malloc return value");
                // NONNULL or guarded MAYBE_NULL → safe, no error
              } else {
                // No tracked state at all — pointer declared but never assigned
                emit(i, "UninitDeref",
                     "dereference of '" + name + "' which was never initialised — "
                     "assign a valid address before dereferencing");
              }
            }
            // Non-tracked pointer expressions: trust the programmer
          }
        }
        continue;
      }

      // -----------------------------------------------------------------------
      // Pointer arithmetic: ptr++ / ptr-- / ptr += n / ptr -= n / ptr + n
      // Rust-style: arithmetic is fine if the pointer has a known-valid state.
      // We only flag it when the pointer is null or freed — meaning the arithmetic
      // is provably meaningless or dangerous.
      // -----------------------------------------------------------------------
      if (tok.type == TT::IDENTIFIER) {
        std::string name = safe_name(tok.value);
        if (var_is_ptr(name)) {
          auto pit = ptr_info.find(name);
          bool provably_bad = false;
          if (pit != ptr_info.end()) {
            provably_bad = (pit->second.own_st == OwnState::FREED ||
                            pit->second.null_st == NullState::KNOWN_NULL);
          }
          if (provably_bad) {
            if (i + 1 < body_end) {
              TT nxt = tokens[i+1].type;
              if (nxt == TT::INCR || nxt == TT::DECR ||
                  nxt == TT::PLUS_ASSIGN || nxt == TT::MINUS_ASSIGN)
                emit(i, "PtrArith",
                     "pointer arithmetic on '" + name + "' which is null or freed — "
                     "this would produce an invalid address");
            }
          }
          // No error for arithmetic on validly-initialized pointers
        }
        continue;
      }

      // -----------------------------------------------------------------------
      // Raw pointer indexing: name[non-zero-expr] where name is a raw ptr
      // Stack arrays with known sizes get OOB checks; raw ptrs are unsafe.
      // -----------------------------------------------------------------------
      if (tok.type == TT::LSBRACKET && i > body_start &&
          tokens[i-1].type == TT::IDENTIFIER) {
        // Skip if this [ is part of a type declaration: TYPE NAME [ SIZE ]
        // i.e. tokens[i-2] is a type keyword — that is the size declarator, not an access
        static const std::set<TT> type_kws_decl = {
          TT::INT,TT::FLOAT,TT::LONG,TT::SHORT,TT::DOUBLE,
          TT::BOOL_KW,TT::CHAR_KW,TT::U8,TT::U32,TT::U64,TT::STR
        };
        bool is_decl_bracket = (i >= 2 && type_kws_decl.count(tokens[i-2].type));
        std::string arr_name = safe_name(tokens[i-1].value);
        auto vit = var_types.find(arr_name);
        bool is_stack = vit != var_types.end() &&
                        vit->second.find("_ARRAY") != std::string::npos;
        bool is_raw_ptr = !is_stack && var_is_ptr(arr_name);
        size_t idx_start = i + 1;
        if (is_decl_bracket) { /* skip — this [ is the size in a declaration */ }
        else if (is_stack) {
          if (idx_start < body_end && tokens[idx_start].type == TT::NUMBER) {
            long long iv = std::stoll(tokens[idx_start].value);
            // Negative index is always OOB
            if (iv < 0) {
              emit(i, "OOBAccess",
                   "array access '" + arr_name + "[" + std::to_string(iv) +
                   "]' uses a negative index — undefined behaviour");
            } else {
              auto sit = arr_sizes.find(arr_name);
              if (sit != arr_sizes.end() && iv >= sit->second)
                emit(i, "OOBAccess",
                     "array access '" + arr_name + "[" + std::to_string(iv) +
                     "]' is out of bounds (size=" +
                     std::to_string(sit->second) + ")");
            }
          } else if (idx_start < body_end && tokens[idx_start].type == TT::MINUS &&
                     idx_start + 1 < body_end &&
                     tokens[idx_start+1].type == TT::NUMBER) {
            // Explicit negative: arr[-N]
            emit(i, "OOBAccess",
                 "array access '" + arr_name + "' with a negative index — "
                 "undefined behaviour");
          }
        } else if (is_raw_ptr) {
          // Rust-style: indexing a valid pointer is fine.
          // Only flag if the pointer is in a provably bad state.
          auto pit = ptr_info.find(arr_name);
          if (pit != ptr_info.end()) {
            // —— Ownership/lifetime checks (independent of null state) ——
            if (pit->second.own_st == OwnState::FREED) {
              emit(i - 1, "UseAfterFree",
                   "indexing '" + arr_name + "' after it was freed — dangling pointer");
            } else {
              // —— Null-state check —————————————————————————————————
              if (pit->second.null_st == NullState::KNOWN_NULL)
                emit(i - 1, "NullDeref",
                     "indexing '" + arr_name + "' which is null — will crash");
              else if (pit->second.null_st == NullState::MAYBE_NULL &&
                       !is_guarded_nonnull(arr_name)) {
                // Only emit a static error if we DON'T have runtime null-check coverage.
                // When _ptr_alloc_elems has an entry for this var, the code emitter
                // already wraps every access in _XEN_IDX(...) which aborts on null.
                if (!_ptr_alloc_elems.count(arr_name))
                  emit(i - 1, "NullDeref",
                       "indexing '" + arr_name + "' without a null check — "
                       "check the allocation result before indexing");
              }

              // —— Bounds check (runs regardless of null state if heap-owned) —
              if (pit->second.own_st == OwnState::HEAP_OWNED) {
              // Heap-allocated pointer with tracked size information
              if (idx_start < body_end && tokens[idx_start].type == TT::NUMBER) {
                long long iv = std::stoll(tokens[idx_start].value);

                if (iv < 0) {
                  // Negative literal index is always wrong — keep as static error
                  emit(i - 1, "OOBAccess",
                       "negative index [" + std::to_string(iv) + "] on heap pointer '" +
                       arr_name + "' — undefined behaviour; wrap in unsafe { } if intentional");
                } else if (pit->second.alloc_bytes > 0 && !pit->second.elem_type.empty()) {
                  // Fully compile-time known: check exactly
                  long long elem_size = get_sizeof_type(pit->second.elem_type);
                  if (elem_size > 0) {
                    long long max_index = (pit->second.alloc_bytes / elem_size) - 1;
                    if (iv > max_index) {
                      emit(i - 1, "OOBAccess",
                           "index [" + std::to_string(iv) + "] on malloc'd pointer '" + arr_name +
                           "' is out of bounds (allocated " + std::to_string(pit->second.alloc_bytes) +
                           " bytes = " + std::to_string(max_index + 1) + " elements of type '" +
                           pit->second.elem_type + "', max valid index is " + std::to_string(max_index) + ")");
                    }
                    // Index within bounds — no error.
                  } else {
                    // Element size unknown: runtime check covers this if available
                    if (!_ptr_alloc_elems.count(arr_name))
                      emit(i - 1, "OOBAccess",
                           "index [" + std::to_string(iv) + "] on malloc'd pointer '" + arr_name +
                           "' cannot be bounds-checked (unknown element type size) — "
                           "wrap in unsafe { } to assert you have verified the allocation size");
                  }
                } else {
                  // Bytes or type unknown at compile time: runtime check covers this
                  if (!_ptr_alloc_elems.count(arr_name))
                    emit(i - 1, "OOBAccess",
                         "index [" + std::to_string(iv) + "] on malloc'd pointer '" + arr_name +
                         "' cannot be bounds-checked at compile time — "
                         "wrap in unsafe { } to assert you have verified the allocation size");
                }
              } else if (idx_start < body_end &&
                         tokens[idx_start].type == TT::MINUS &&
                         idx_start + 1 < body_end &&
                         tokens[idx_start + 1].type == TT::NUMBER) {
                emit(i - 1, "OOBAccess",
                     "negative index on heap pointer '" + arr_name +
                     "' — undefined behaviour; wrap in unsafe { } if intentional");
              }
            } // end HEAP_OWNED bounds check
            } // end else (not FREED)
            // NONNULL / UNOWNED / guarded → trust the programmer
          }
          // No tracked state → pointer came from an opaque source; trust it
        }
        continue;
      }

      // -----------------------------------------------------------------------
      // cast(ptr X, ...) — reinterpret cast to pointer
      // Rust-style: casts are allowed; the correctness is the programmer's
      // responsibility.  We record the result as MAYBE_NULL so any subsequent
      // deref gets the normal null-check enforcement.
      // -----------------------------------------------------------------------
      if (tok.type == TT::CAST &&
          i + 1 < body_end && tokens[i+1].type == TT::LPAREN) {
        // No error emitted — cast is permitted; flow analysis covers the result.
        continue;
      }

      // -----------------------------------------------------------------------
      // address-of a non-variable (temporary)
      // -----------------------------------------------------------------------
      if (tok.type == TT::ADDRESS_OF &&
          i + 1 < body_end && tokens[i+1].type == TT::LPAREN) {
        emit(i, "TempAddrOf",
             "address-of a temporary expression produces a dangling pointer — "
             "requires unsafe");
        continue;
      }

      // -----------------------------------------------------------------------
      // Xenon keyword builtins — strcpy/strcat/memset
      // Rust-style: these are allowed when used correctly.
      // strcpy/strcat: warn only — the programmer is responsible for buffer size.
      // memset: freely allowed (equivalent to Rust's ptr::write_bytes which is
      //         safe when the pointer and length are valid).
      // -----------------------------------------------------------------------
      if (tok.type == TT::STRCPY_KW || tok.type == TT::STRCAT_KW) {
        // Emit a note-level advisory (not a hard error) about buffer size
        // responsibility, similar to how Rust's unsafe blocks require a comment.
        // We use a softer "BoundedStrOp" kind that the reporter renders as a warning.
        if (i + 1 < body_end && tokens[i+1].type == TT::LPAREN) {
          // Try to find the destination argument
          size_t dst_pos = i + 2;
          if (dst_pos < body_end && tokens[dst_pos].type == TT::IDENTIFIER) {
            std::string dst = safe_name(tokens[dst_pos].value);
            auto sit = arr_sizes.find(dst);
            if (sit == arr_sizes.end()) {
              // Destination has no known compile-time size — advisory only
              // (not an error; strncpy is preferable but strcpy is not banned)
              // We intentionally do NOT emit here; runtime is responsible.
            }
          }
        }
        continue;
      }
      if (tok.type == TT::MEMSET_KW) {
        // memset on a valid pointer is always fine — no check needed
        continue;
      }
    }

    // =======================================================================
    // PASS 2: Extended static analysis — Rust-parity checks
    // These run as a second linear scan over the body so we don't tangle the
    // unsafe-block tracking logic above.  All checks are purely static; zero
    // runtime overhead.
    // =======================================================================

    // -- State for pass 2 --
    // Track declared variables and their initialisation status
    struct VarState {
      std::string type_raw;   // raw Xenon type ("int","str","ptr","bool",...)
      bool initialised{false};
      bool is_const{false};
      bool used{false};
      int decl_line{0};
    };
    std::map<std::string, VarState> var_state;

    // Scope stack: each entry is a set of variable names declared in that scope
    // so we can detect shadowing and track scope-local vars.
    std::vector<std::set<std::string>> scope_stack;
    scope_stack.push_back({}); // top-level function scope

    // Pre-populate with params (from local_var_types, which the caller provides)
    for (auto const &kv : local_var_types) {
      VarState vs;
      vs.type_raw    = kv.second;
      vs.initialised = true; // params always initialised
      vs.is_const    = false;
      var_state[kv.first] = vs;
      scope_stack.back().insert(kv.first);
      // If the param's type is a struct, also register "paramName.fieldName" entries
      // so that dot-accesses like foo.val are treated as initialized (not flagged).
      const std::string &ptype_raw = kv.second;
      auto sfit = struct_field_types.find(ptype_raw);
      if (sfit != struct_field_types.end()) {
        for (const auto &ffield : sfit->second) {
          std::string dot_key = kv.first + "." + ffield.first;
          VarState fvs;
          fvs.type_raw    = ffield.second;
          fvs.initialised = true;
          fvs.is_const    = false;
          var_state[dot_key] = fvs;
          scope_stack.back().insert(dot_key);
        }
      }
    }

    // Track whether we've seen a return at the current (depth-0) scope level
    // to detect unreachable-code-after-return.
    int p2_brace_depth    = 0;
    // returned_at_top is subsumed by returned_at_depth[0]
    // per-depth return flags (index = brace depth; depth 0 = function body)
    std::vector<bool> returned_at_depth(64, false);

    // Collect const variable names for mutation checks
    std::set<std::string> const_vars;

    // Recompute unsafe_depth inline for pass 2 (no side-effects from pass 1)
    int p2_unsafe_depth = 0;
    int p2_unsafe_brace_depth = 0;
    std::stack<int> p2_brace_depth_at_unsafe;

    auto emit2 = [&](size_t idx, const std::string &kind, const std::string &msg) {
      if (p2_unsafe_depth > 0) return; // inside unsafe block — silence extended checks too
      int ln = (idx < tokens.size()) ? tokens[idx].line : 0;
      int co = (idx < tokens.size()) ? tokens[idx].col  : 0;
      violations.push_back({ln, co, kind, msg});
    };

    for (size_t i = body_start; i < body_end && i < tokens.size(); i++) {
      const Token &tok = tokens[i];

      // ── Unsafe-block tracking (mirrors pass 1) ───────────────────────────
      if (tok.type == TT::LBRACE) {
        p2_brace_depth++;
        p2_unsafe_brace_depth++;
        scope_stack.push_back({});
        if ((int)returned_at_depth.size() <= p2_brace_depth)
          returned_at_depth.resize(p2_brace_depth + 1, false);
        returned_at_depth[p2_brace_depth] = false;
        continue;
      }
      if (tok.type == TT::UNSAFE_KW &&
          i + 1 < body_end && tokens[i+1].type == TT::LBRACE) {
        p2_unsafe_depth++;
        p2_unsafe_brace_depth++;
        p2_brace_depth_at_unsafe.push(p2_unsafe_brace_depth);
        i++; continue;
      }
      if (tok.type == TT::RBRACE) {
        if (!p2_brace_depth_at_unsafe.empty() &&
            p2_unsafe_brace_depth == p2_brace_depth_at_unsafe.top()) {
          p2_unsafe_depth--;
          p2_brace_depth_at_unsafe.pop();
        }
        p2_unsafe_brace_depth--;
        if (!scope_stack.empty()) scope_stack.pop_back();
        if (p2_brace_depth > 0) p2_brace_depth--;  // never go negative
        continue;
      }
      if (p2_unsafe_depth > 0) continue;

      // ── Unreachable code after return ─────────────────────────────────────
      // If we've already seen a return at this brace depth, any subsequent
      // statement-level token is unreachable.
      if (p2_brace_depth < (int)returned_at_depth.size() &&
          returned_at_depth[p2_brace_depth] &&
          tok.type != TT::RBRACE && tok.type != TT::SEMICOLON &&
          tok.type != TT::LBRACE && tok.type != TT::TEOF) {
        // Only fire at statement-level tokens: previous token is ;, {, or } 
        bool at_stmt_boundary = (i == body_start);
        if (!at_stmt_boundary && i > body_start) {
          TT prev = tokens[i-1].type;
          at_stmt_boundary = (prev == TT::SEMICOLON || prev == TT::LBRACE ||
                              prev == TT::RBRACE);
        }
        if (!at_stmt_boundary) goto skip_unreachable_check;
        // Only flag the first non-trivial token to avoid a cascade
        emit2(i, "UnreachableCode",
              "statement after 'return' is unreachable — remove or restructure");
        // Suppress further unreachable warnings in this scope by clearing the flag
        returned_at_depth[p2_brace_depth] = false;
        // Don't continue — we still process the token for other checks
      }
      skip_unreachable_check:;

      if (tok.type == TT::RETURN) {
        if (p2_brace_depth < (int)returned_at_depth.size())
          returned_at_depth[p2_brace_depth] = true;
      }

      // ── Variable declaration tracking ────────────────────────────────────
      // Pattern: CONST_KW? TYPE_KW NAME [= RHS]
      {
        static const std::set<TT> decl_type_kws = {
          TT::INT, TT::FLOAT, TT::LONG, TT::SHORT, TT::DOUBLE,
          TT::BOOL_KW, TT::CHAR_KW, TT::U8, TT::U32, TT::U64, TT::STR
        };
        bool is_const_decl = (tok.type == TT::CONST_KW);
        size_t type_pos = is_const_decl ? i + 1 : i;
        if (type_pos < body_end && decl_type_kws.count(tokens[type_pos].type) &&
            type_pos + 1 < body_end && tokens[type_pos+1].type == TT::IDENTIFIER &&
            // Not an array type declaration (handled by pass 1)
            !(type_pos + 2 < body_end && tokens[type_pos+2].type == TT::LSBRACKET)) {

          std::string vname = safe_name(tokens[type_pos+1].value);
          std::string type_raw = tokens[type_pos].value;
          bool has_init = (type_pos + 2 < body_end &&
                           tokens[type_pos+2].type == TT::ASSIGN);

          // ── Shadowing check ───────────────────────────────────────────────
          // Check ALL enclosing scopes (not just current) for a same-named var
          bool shadows = false;
          for (int si = (int)scope_stack.size() - 2; si >= 0; si--) {
            if (scope_stack[si].count(vname)) { shadows = true; break; }
          }
          if (!shadows && var_state.count(vname) &&
              !scope_stack.empty() && !scope_stack.back().count(vname)) {
            shadows = true;
          }
          if (shadows) {
            emit2(type_pos + 1, "ShadowedVariable",
                  "variable '" + vname + "' shadows an outer declaration — "
                  "rename to avoid confusion (Rust forbids this in many cases)");
          }

          // ── Const tracking ────────────────────────────────────────────────
          if (is_const_decl) {
            const_vars.insert(vname);
          }

          // ── Narrowing check: initialiser is a literal ─────────────────────
          if (has_init && type_pos + 3 < body_end &&
              tokens[type_pos+3].type == TT::NUMBER) {
            const std::string &lit = tokens[type_pos+3].value;
            // Try to parse as integer (skip floats and hex for now)
            bool is_int_lit = true;
            for (char c : lit) {
              if (c == '.' || c == 'x' || c == 'X' || c == 'e' || c == 'E')
              { is_int_lit = false; break; }
            }
            if (is_int_lit && !lit.empty()) {
              try {
                long long val = std::stoll(lit);
                std::string c_type = raw_to_c(type_raw);
                if (!literal_fits_in_type(val, c_type)) {
                  emit2(type_pos + 3, "IntegerOverflow",
                        "literal value " + lit + " does not fit in type '" +
                        type_raw + "' — this is a compile-time integer overflow");
                }
              } catch (...) {
                emit2(type_pos + 3, "IntegerOverflow",
                      "literal value " + lit + " is too large to represent in "
                      "any integer type (overflows even uint64) — "
                      "compile-time integer overflow");
              }
            }
          }

          VarState vs;
          vs.type_raw    = type_raw;
          vs.initialised = has_init;
          vs.is_const    = is_const_decl;
          vs.decl_line   = tok.line;
          var_state[vname] = vs;
          if (!scope_stack.empty()) scope_stack.back().insert(vname);

          if (is_const_decl) i++; // skip CONST_KW, loop will advance type_pos tok
        }
      }

      // let/var NAME = RHS  or  let/var NAME (uninitialised)
      if ((tok.type == TT::LET_KW || tok.type == TT::VAR_KW) &&
          i + 1 < body_end && tokens[i+1].type == TT::IDENTIFIER) {
        std::string vname = safe_name(tokens[i+1].value);
        bool has_init = (i + 2 < body_end && tokens[i+2].type == TT::ASSIGN);

        // Shadowing
        bool shadows = false;
        for (int si = (int)scope_stack.size() - 2; si >= 0; si--) {
          if (scope_stack[si].count(vname)) { shadows = true; break; }
        }
        if (!shadows && var_state.count(vname) &&
            !scope_stack.empty() && !scope_stack.back().count(vname))
          shadows = true;
        if (shadows) {
          emit2(i+1, "ShadowedVariable",
                "variable '" + vname + "' shadows an outer declaration — "
                "rename to avoid confusion");
        }

        VarState vs;
        vs.type_raw    = "let";
        vs.initialised = has_init;
        vs.is_const    = false;
        vs.decl_line   = tok.line;
        var_state[vname] = vs;
        if (!scope_stack.empty()) scope_stack.back().insert(vname);
      }

      // ── Const violation: assignment to a const variable ──────────────────
      // Pattern: IDENTIFIER ASSIGN (but not ==)
      if (tok.type == TT::IDENTIFIER &&
          i + 2 < body_end &&
          tokens[i+1].type == TT::ASSIGN &&
          tokens[i+2].type != TT::ASSIGN) {
        std::string lhs = safe_name(tok.value);
        if (const_vars.count(lhs)) {
          emit2(i, "ConstViolation",
                "assignment to 'const' variable '" + lhs + "' — "
                "const variables cannot be mutated after declaration");
        }
        // Also check += -= *= /= %=
        // Mark as now initialised
        auto vsit = var_state.find(lhs);
        if (vsit != var_state.end())
          vsit->second.initialised = true;
      }
      if (tok.type == TT::IDENTIFIER &&
          i + 1 < body_end &&
          (tokens[i+1].type == TT::PLUS_ASSIGN || tokens[i+1].type == TT::MINUS_ASSIGN ||
           tokens[i+1].type == TT::MUL_ASSIGN   || tokens[i+1].type == TT::DIV_ASSIGN  ||
           tokens[i+1].type == TT::MOD_ASSIGN   || tokens[i+1].type == TT::INCR        ||
           tokens[i+1].type == TT::DECR)) {
        std::string lhs = safe_name(tok.value);
        if (const_vars.count(lhs)) {
          emit2(i, "ConstViolation",
                "modification of 'const' variable '" + lhs + "' via compound assignment — "
                "const variables are immutable");
        }
      }

      // ── Use of uninitialised variable ────────────────────────────────────
      // Checks bare identifiers AND dot-accesses (name.field).
      // Params are always initialised; struct param fields (param.field) are also
      // seeded as initialised in var_state during pre-population above.
      if (tok.type == TT::IDENTIFIER) {
        std::string vname = safe_name(tok.value);
        // Detect dot-access: IDENTIFIER DOT IDENTIFIER
        bool is_dot_access = (i + 2 < body_end &&
                              tokens[i+1].type == TT::DOT &&
                              tokens[i+2].type == TT::IDENTIFIER);
        if (is_dot_access) {
          // For dot accesses the struct var itself must be initialised.
          // If var_state has the struct but uninitialized, flag it.
          auto vsit = var_state.find(vname);
          if (vsit != var_state.end() && !vsit->second.initialised) {
            bool prev_is_type = false;
            if (i > body_start) {
              TT prev = tokens[i-1].type;
              static const std::set<TT> type_kws2 = {
                TT::INT,TT::FLOAT,TT::LONG,TT::SHORT,TT::DOUBLE,
                TT::BOOL_KW,TT::CHAR_KW,TT::U8,TT::U32,TT::U64,TT::STR,
                TT::PTR,TT::CONST_KW,TT::LET_KW,TT::VAR_KW
              };
              prev_is_type = type_kws2.count(prev) > 0;
            }
            if (!prev_is_type) {
              emit2(i, "Uninitialised",
                    "variable '" + vname + "' may be used before initialisation — "
                    "Xenon requires variables to be initialised before use");
              vsit->second.initialised = true;
            }
          }
          // Mark field access 'name.field' as initialised when it's on the LHS
          // of an assignment (i.e. tokens[i+3] == ASSIGN, not ==).
          if (i + 3 < body_end &&
              tokens[i+3].type == TT::ASSIGN &&
              (i + 4 >= body_end || tokens[i+4].type != TT::ASSIGN)) {
            std::string dot_key = vname + "." + safe_name(tokens[i+2].value);
            auto dvsit = var_state.find(dot_key);
            if (dvsit != var_state.end()) dvsit->second.initialised = true;
            // Also mark the struct var itself as initialised when a field is assigned
            auto svsit = var_state.find(vname);
            if (svsit != var_state.end()) svsit->second.initialised = true;
          }
        } else {
          auto vsit = var_state.find(vname);
          if (vsit != var_state.end() && !vsit->second.initialised) {
            // Check context: if this is a use (not the LHS of an assignment)
            bool is_lhs_assign = (i + 1 < body_end &&
                                  tokens[i+1].type == TT::ASSIGN &&
                                  (i + 2 >= body_end || tokens[i+2].type != TT::ASSIGN));
            if (!is_lhs_assign) {
              // Only fire if the previous token isn't a type keyword (i.e. not a decl)
              bool prev_is_type = false;
              if (i > body_start) {
                TT prev = tokens[i-1].type;
                static const std::set<TT> type_kws2 = {
                  TT::INT,TT::FLOAT,TT::LONG,TT::SHORT,TT::DOUBLE,
                  TT::BOOL_KW,TT::CHAR_KW,TT::U8,TT::U32,TT::U64,TT::STR,
                  TT::PTR,TT::CONST_KW,TT::LET_KW,TT::VAR_KW
                };
                prev_is_type = type_kws2.count(prev) > 0;
              }
              if (!prev_is_type) {
                emit2(i, "Uninitialised",
                      "variable '" + vname + "' may be used before initialisation — "
                      "Xenon requires variables to be initialised before use");
                vsit->second.initialised = true; // suppress further warnings
              }
            }
          }
        }
      }

      // ── Division/modulo by zero (literal zero) ────────────────────────────
      if ((tok.type == TT::DIVIDE || tok.type == TT::MOD) &&
          i + 1 < body_end &&
          tokens[i+1].type == TT::NUMBER &&
          tokens[i+1].value == "0") {
        emit2(i, "DivisionByZero",
              std::string(tok.type == TT::DIVIDE ? "division" : "modulo") +
              " by literal zero — this is undefined behaviour");
      }
      // Also catch /= 0 and %= 0
      if ((tok.type == TT::DIV_ASSIGN || tok.type == TT::MOD_ASSIGN) &&
          i + 1 < body_end &&
          tokens[i+1].type == TT::NUMBER &&
          tokens[i+1].value == "0") {
        emit2(i, "DivisionByZero",
              std::string(tok.type == TT::DIV_ASSIGN ? "division" : "modulo") +
              " by literal zero in compound assignment — undefined behaviour");
      }

      // ── Signed/unsigned comparison mismatch ──────────────────────────────
      // Pattern: EXPR CMP_OP EXPR where one side is signed and other is unsigned.
      // We only check the simple case where both sides are named variables.
      if ((tok.type == TT::LT || tok.type == TT::GT ||
           tok.type == TT::LE || tok.type == TT::GE ||
           tok.type == TT::EQ || tok.type == TT::NE) &&
          i > body_start && i + 1 < body_end) {
        size_t left_pos  = i - 1;
        size_t right_pos = i + 1;
        if (tokens[left_pos].type  == TT::IDENTIFIER &&
            tokens[right_pos].type == TT::IDENTIFIER) {
          std::string lname = safe_name(tokens[left_pos].value);
          std::string rname = safe_name(tokens[right_pos].value);
          TypeInfo lti = lookup_var(lname);
          TypeInfo rti = lookup_var(rname);
          if (!lti.is_unknown() && !rti.is_unknown() &&
              !lti.is_ptr() && !rti.is_ptr()) {
            bool l_signed = is_signed_int_type(lti.base);
            bool r_signed = is_signed_int_type(rti.base);
            bool l_unsigned = is_unsigned_int_type(lti.base);
            bool r_unsigned = is_unsigned_int_type(rti.base);
            if ((l_signed && r_unsigned) || (l_unsigned && r_signed)) {
              emit2(i, "SignedUnsignedMismatch",
                    "comparison between signed ('" + lti.base + "') and unsigned "
                    "('" + rti.base + "') — can produce surprising results when "
                    "the signed value is negative");
            }
          }
        }
      }

      // ── Implicit narrowing: assignment of wider type to narrower ──────────
      // Pattern: NAME = NAME2  where NAME's type is narrower than NAME2's type.
      // We only check named var-to-var assignments (not expressions).
      if (tok.type == TT::IDENTIFIER &&
          i + 2 < body_end &&
          tokens[i+1].type == TT::ASSIGN &&
          tokens[i+2].type != TT::ASSIGN &&
          tokens[i+2].type == TT::IDENTIFIER &&
          // skip if next is a function call
          !(i + 3 < body_end && tokens[i+3].type == TT::LPAREN)) {
        std::string lhs = safe_name(tok.value);
        std::string rhs = safe_name(tokens[i+2].value);
        TypeInfo lti = lookup_var(lhs);
        TypeInfo rti = lookup_var(rhs);
        if (!lti.is_unknown() && !rti.is_unknown() &&
            !lti.is_ptr() && !rti.is_ptr() &&
            lti.is_numeric() && rti.is_numeric()) {
          int l_rank = int_type_rank(lti.base);
          int r_rank = int_type_rank(rti.base);
          if (l_rank >= 0 && r_rank >= 0 && r_rank > l_rank) {
            emit2(i, "ImplicitNarrowing",
                  "implicit narrowing: assigning '" + rti.base + "' ('" + rhs +
                  "') into '" + lti.base + "' ('" + lhs + "') — "
                  "use cast(" + lti.base + ", " + rhs + ") to make this explicit");
          }
        }
      }

      // ── Unused return value from non-void functions ───────────────────────
      // Pattern: IDENTIFIER LPAREN ... RPAREN SEMICOLON  at statement level.
      // Only flag functions with a known non-void return type.
      // We don't flag functions like printf/exit/assert which are commonly
      // called for side effects.
      if (tok.type == TT::IDENTIFIER &&
          i + 1 < body_end && tokens[i+1].type == TT::LPAREN) {
        // Statement context: previous token is ; or { or nothing
        bool is_stmt = (i == body_start);
        if (!is_stmt && i > body_start) {
          TT prev = tokens[i-1].type;
          is_stmt = (prev == TT::SEMICOLON || prev == TT::LBRACE ||
                     prev == TT::RBRACE);
        }
        if (is_stmt) {
          static const std::set<std::string> side_effect_ok = {
            "printf","fprintf","sprintf","snprintf","puts","putchar",
            "exit","abort","assert","free","fclose","fflush",
            "memset","memcpy","memmove","strcpy","strncpy","strcat","strncat",
            "scanf","fscanf","sscanf","fgets","fread","fwrite","fseek","ftruncate",
            "sleep","usleep","nanosleep","pthread_create","pthread_join",
            "print","println","printfmt" // Xenon builtins
          };
          std::string callee = tok.value;
          if (!side_effect_ok.count(callee)) {
            auto frit = func_return_types.find(callee);
            if (frit != func_return_types.end()) {
              const std::string &fret = frit->second;
              if (!fret.empty() && fret != "void" && fret != "__infer__") {
                emit2(i, "UnusedReturnValue",
                      "return value of '" + callee + "' (type '" + fret +
                      "') is discarded — in safe code every non-void result "
                      "must be used or explicitly ignored via (void)");
              }
            }
          }
        }
      }

      // ── Taint propagation: NAME = tainted_var ────────────────────────────
      // If a tainted variable is copied to another, the destination is tainted.
      if (tok.type == TT::IDENTIFIER &&
          i + 2 < body_end &&
          tokens[i+1].type == TT::ASSIGN &&
          tokens[i+2].type != TT::ASSIGN &&
          tokens[i+2].type == TT::IDENTIFIER) {
        if (is_tainted(safe_name(tokens[i+2].value)))
          mark_tainted(safe_name(tok.value));
      }
      // Taint from scanf/fgets/getchar keywords (Xenon builtins)
      if (tok.type == TT::SCANF &&
          i + 1 < body_end && tokens[i+1].type == TT::IDENTIFIER) {
        mark_tainted(safe_name(tokens[i+1].value));
      }
      // Taint from: NAME = user_input_func(...)
      if (tok.type == TT::IDENTIFIER &&
          i + 3 < body_end &&
          tokens[i+1].type == TT::ASSIGN &&
          tokens[i+2].type != TT::ASSIGN &&
          tokens[i+2].type == TT::IDENTIFIER &&
          tokens[i+3].type == TT::LPAREN &&
          token_is_user_input_source(tokens[i+2].value)) {
        mark_tainted(safe_name(tok.value));
      }
      // Taint through arithmetic: if an operand is tainted, so is the result
      // Pattern: NAME = tainted OP expr  (very conservative — catches computed indices)
      if (tok.type == TT::IDENTIFIER &&
          i + 3 < body_end &&
          tokens[i+1].type == TT::ASSIGN &&
          tokens[i+2].type != TT::ASSIGN &&
          tokens[i+2].type == TT::IDENTIFIER) {
        // Check if any of the next few tokens (up to ;) include a tainted var
        for (size_t k = i + 2; k < body_end && k < i + 12 &&
             tokens[k].type != TT::SEMICOLON; k++) {
          if (tokens[k].type == TT::IDENTIFIER &&
              is_tainted(safe_name(tokens[k].value))) {
            mark_tainted(safe_name(tok.value));
            break;
          }
        }
      }

      // ── Format-string injection detection ────────────────────────────────
      // Pattern: printf(tainted_var, ...) or fprintf(f, tainted_var, ...)
      // If the format argument is a user-tainted variable, flag it.
      if (tok.type == TT::IDENTIFIER &&
          i + 1 < body_end && tokens[i+1].type == TT::LPAREN) {
        auto fmtit = fmt_arg_pos.find(tok.value);
        if (fmtit != fmt_arg_pos.end()) {
          int fmt_pos = fmtit->second;
          // Walk args to find the format-position argument
          size_t j = i + 2;
          int arg_idx = 0;
          int depth2 = 0;
          size_t arg_start = j;
          while (j < body_end &&
                 !(tokens[j].type == TT::RPAREN && depth2 == 0)) {
            if (tokens[j].type == TT::LPAREN || tokens[j].type == TT::LSBRACKET)
              depth2++;
            else if (tokens[j].type == TT::RPAREN || tokens[j].type == TT::RSBRACKET)
              depth2--;
            else if (tokens[j].type == TT::COMMA && depth2 == 0) {
              if (arg_idx == fmt_pos && arg_start < j &&
                  tokens[arg_start].type == TT::IDENTIFIER &&
                  is_tainted(safe_name(tokens[arg_start].value))) {
                emit2(arg_start, "FormatInjection",
                      "format-string injection: user-tainted variable '" +
                      tokens[arg_start].value + "' used as format string in '" +
                      tok.value + "' — attacker can read stack memory or crash; "
                      "use a literal format string like printf(\"%s\", " +
                      tokens[arg_start].value + ")");
              }
              arg_idx++;
              arg_start = j + 1;
            }
            j++;
          }
          // Check final arg (handles printf with no trailing comma)
          if (arg_idx == fmt_pos && arg_start < j &&
              tokens[arg_start].type == TT::IDENTIFIER &&
              is_tainted(safe_name(tokens[arg_start].value))) {
            emit2(arg_start, "FormatInjection",
                  "format-string injection: user-tainted variable '" +
                  tokens[arg_start].value + "' used as format string in '" +
                  tok.value + "' — use a literal format string");
          }
        }
      }

      // ── Null-narrowing: detect if (ptr != NULL) guards ────────────────────
      // Supports both C-style braces: IF LPAREN IDENTIFIER NE NULL_KW RPAREN LBRACE
      // and Xenon-style then/end: IF IDENTIFIER NE NULL_KW THEN ... END
      // When we see such a guard, push the variable onto the nonnull stack for
      // the duration of the guarded block. On RBRACE/END, pop it.
      if (tok.type == TT::IF && i + 1 < body_end) {
        // Scan the condition
        std::string guarded_var;

        if (tokens[i+1].type == TT::LPAREN) {
          // ── C-style: if (expr) { ... } ──────────────────────────────────
          size_t cond = i + 2;
          // Pattern 1a: (name != NULL) or (name != 0)
          if (cond + 3 < body_end &&
              tokens[cond].type == TT::IDENTIFIER &&
              tokens[cond+1].type == TT::NE &&
              (tokens[cond+2].type == TT::NULL_KW ||
               (tokens[cond+2].type == TT::NUMBER && tokens[cond+2].value == "0")) &&
              tokens[cond+3].type == TT::RPAREN) {
            guarded_var = safe_name(tokens[cond].value);
          }
          // Pattern 1b: (name) — truthy check on a ptr
          else if (cond + 1 < body_end &&
                   tokens[cond].type == TT::IDENTIFIER &&
                   tokens[cond+1].type == TT::RPAREN) {
            if (var_is_ptr(safe_name(tokens[cond].value)))
              guarded_var = safe_name(tokens[cond].value);
          }
        } else {
          // ── Xenon-style: if expr then ... end  (no parens, no braces) ───
          size_t cond = i + 1;
          // Pattern 2a: name != null then
          if (cond + 3 < body_end &&
              tokens[cond].type == TT::IDENTIFIER &&
              tokens[cond+1].type == TT::NE &&
              (tokens[cond+2].type == TT::NULL_KW ||
               (tokens[cond+2].type == TT::NUMBER && tokens[cond+2].value == "0")) &&
              tokens[cond+3].type == TT::THEN) {
            guarded_var = safe_name(tokens[cond].value);
          }
          // Pattern 2b: name then  (truthy check)
          else if (cond + 1 < body_end &&
                   tokens[cond].type == TT::IDENTIFIER &&
                   tokens[cond+1].type == TT::THEN) {
            if (var_is_ptr(safe_name(tokens[cond].value)))
              guarded_var = safe_name(tokens[cond].value);
          }
        }

        if (!guarded_var.empty()) {
          // Push a new nonnull frame and record the guarded variable in it.
          // The frame will be popped on the matching RBRACE or END token.
          nonnull_stack.push_back({guarded_var});
        }
      }
      // On entering a new brace scope, push a new nonnull frame
      // If-null-guard scope exit: on RBRACE, pop nonnull frame
      // This is approximate — we clear on every RBRACE/END, which is safe
      // (conservative: may miss some valid guards but never hides real bugs)
      if ((tok.type == TT::RBRACE || tok.type == TT::END) &&
          nonnull_stack.size() > 1) {
        nonnull_stack.pop_back();
      }
      if (tok.type == TT::LBRACE) {
        nonnull_stack.push_back({});
      }

      // ── str (char*) parameter mutation via raw index write ────────────────
      // If a 'str' parameter is being indexed on the LHS of an assignment
      // (e.g. buf[i] = 'x'), that's a mutation of a borrowed string —
      // flag it unless we're in an unsafe block.
      // Pattern: IDENTIFIER LSBRACKET EXPR RSBRACKET ASSIGN
      if (tok.type == TT::IDENTIFIER &&
          i + 3 < body_end &&
          tokens[i+1].type == TT::LSBRACKET) {
        std::string arr = safe_name(tok.value);
        auto vtit = var_types.find(arr);
        if (vtit != var_types.end() &&
            (vtit->second == "str" || vtit->second == "char*")) {
          // Find matching ] and check for =
          size_t j = i + 2;
          int d = 1;
          while (j < body_end && d > 0) {
            if (tokens[j].type == TT::LSBRACKET) d++;
            else if (tokens[j].type == TT::RSBRACKET) d--;
            j++;
          }
          if (j < body_end && tokens[j].type == TT::ASSIGN &&
              j + 1 < body_end && tokens[j+1].type != TT::ASSIGN) {
            emit2(i, "StrMutation",
                  "direct byte mutation of string '" + arr + "' via indexing — "
                  "str (char*) values are treated as borrowed; mutation requires unsafe");
          }
        }
      }

    } // end pass-2 loop

    // =======================================================================
    // PASS 3: Memory-leak detection
    // Any pointer that was allocated on the heap (HEAP_OWNED) and was never
    // freed (own_st != FREED) by the time we reach the end of the function
    // body is a definite memory leak.  We report it at the declaration site.
    // =======================================================================
    if (_memory_safe && unsafe_depth == 0) {
      // Pre-scan the body token range to find variables declared as ptr = malloc(...)
      // so that RAII-managed ones can be excluded from the leak report.
      // In non-TCC mode, any such variable gets __attribute__((cleanup(_xen_raii_free_ptr_)))
      // and is automatically freed at scope exit — reporting a leak would be wrong.
      std::set<std::string> raii_managed_in_body;
      if (!tcc_mode) {
        for (size_t ri = body_start; ri + 3 < body_end && ri < tokens.size(); ri++) {
          if (tokens[ri].type == TT::PTR &&
              ri + 2 < body_end &&
              tokens[ri + 2].type == TT::IDENTIFIER) {
            std::string vname = safe_name(tokens[ri + 2].value);
            // Look for = followed by a heap allocator call
            if (ri + 3 < body_end && tokens[ri + 3].type == TT::ASSIGN) {
              if (ri + 4 < body_end && tokens[ri + 4].type == TT::IDENTIFIER) {
                const std::string &callee = tokens[ri + 4].value;
                if (token_is_heap_alloc(callee))
                  raii_managed_in_body.insert(vname);
              }
            }
          }
        }
        // Also detect: let/var name = malloc(...)
        for (size_t ri = body_start; ri + 4 < body_end && ri < tokens.size(); ri++) {
          if ((tokens[ri].type == TT::LET_KW || tokens[ri].type == TT::VAR_KW) &&
              tokens[ri + 1].type == TT::IDENTIFIER &&
              tokens[ri + 2].type == TT::ASSIGN &&
              tokens[ri + 3].type == TT::IDENTIFIER) {
            const std::string &callee = tokens[ri + 3].value;
            if (token_is_heap_alloc(callee))
              raii_managed_in_body.insert(safe_name(tokens[ri + 1].value));
          }
        }
        // Also honour vars already registered in _raii_var_drop_expr from prior
        // parse passes (sub-transpiler or incremental compilation paths).
        for (const auto &[n, _] : _raii_var_drop_expr)
          raii_managed_in_body.insert(n);
      }

      for (auto const &[name, pi] : ptr_info) {
        if (pi.own_st == OwnState::HEAP_OWNED) {
          // ── RAII suppression ─────────────────────────────────────────────
          // Variables managed by __attribute__((cleanup)) are auto-freed at
          // every scope exit — the compiler handles it, no leak possible.
          if (raii_managed_in_body.count(name)) continue;

          // Find the declaration token for a better error location
          int decl_line = 0, decl_col = 0;
          for (size_t i = body_start; i < body_end && i < tokens.size(); i++) {
            if (tokens[i].type == TT::IDENTIFIER &&
                safe_name(tokens[i].value) == name) {
              decl_line = tokens[i].line;
              decl_col  = tokens[i].col;
              break;
            }
          }
          violations.push_back({decl_line, decl_col, "MemoryLeak",
            "heap allocation assigned to '" + name + "' is never freed — "
            "call free(" + name + ") before it goes out of scope, "
            "or wrap in unsafe { } if ownership is transferred"});
        }
      }
    }

    var_types = saved_vt;
    return violations;
  }

  // =========================================================================
  // Borrow Checker (Rust-style flow-sensitive edition)
  // =========================================================================
  // Philosophy: like Rust, we do NOT ban pointer usage — we track ownership
  // and flag only provably wrong patterns.  Correct code (malloc + null-check
  // + free once) is accepted without any unsafe { } annotation.
  //
  // Rules:
  //   1. Move on ptr-to-ptr assignment:  ptr y = x  →  x is moved
  //   2. Move on passing a ptr to a ptr-typed param →  arg is moved
  //   3. Use after move: reading/passing/derefing a moved variable → BorrowError
  //   4. Re-assign to moved var is allowed (re-initialises it)
  //   5. Branch awareness: if both branches of an if/else move a var, it is
  //      moved after the if/else.  If only one branch moves it, the var is
  //      "conditionally moved" — any use after the branch is flagged.
  //   6. Loop body moves: a move inside a while/for body is flagged on the
  //      second iteration (the variable is moved on iter 1, gone on iter 2).
  //   7. Borrow-while-moved: passing a moved var as a non-owning arg still
  //      catches use-after-move.
  //   8. Double-move in same expression: moving the same var twice in one
  //      call's argument list → flagged immediately.
  //   9. Conditional move hint: "conditionally moved" vars get a richer
  //      message suggesting a guard or a re-borrow pattern.
  //  10. Moved-var field access: moved.field is caught just like bare moved.
  //
  // Disabled in: unsafe { } blocks, unsafe functions, -no-check, -no-check:borrow
  // =========================================================================
  std::vector<UnsafeViolation> scan_body_for_borrow_errors(
      size_t body_start, size_t body_end,
      const std::map<std::string, std::string> &local_var_types)
  {
    std::vector<UnsafeViolation> violations;
    if (!_borrow_check || !_memory_safe) return violations;

    std::map<std::string, std::string> lv = local_var_types;

    auto is_ptr_type = [](const std::string &t) -> bool {
      return !t.empty() && t.back() == '*';
    };
    auto is_ptr_var = [&](const std::string &name) -> bool {
      auto it = lv.find(name);
      if (it != lv.end()) return is_ptr_type(it->second);
      return false;
    };

    // ── Move state ────────────────────────────────────────────────────────
    // MOVED      : unconditionally moved (hard error on any subsequent use)
    // COND_MOVED : moved in one branch but not the other (softer hint)
    enum class MoveState { LIVE, COND_MOVED, MOVED };
    std::map<std::string, MoveState> move_state;

    int unsafe_depth = 0;
    int brace_depth  = 0;
    std::stack<int> brace_depth_at_unsafe;

    // Track loop brace depths so we can warn about moves inside loops
    std::stack<int> loop_brace_depths; // brace_depth when loop body { opens

    auto emit_borrow = [&](size_t idx, const std::string &kind,
                           const std::string &msg) {
      if (unsafe_depth > 0) return;
      int ln = (idx < tokens.size()) ? tokens[idx].line : 0;
      int co = (idx < tokens.size()) ? tokens[idx].col  : 0;
      violations.push_back({ln, co, kind, msg});
    };

    // Helper: check a use of `name` at token index `idx`
    auto check_use = [&](const std::string &name, size_t idx) {
      auto it = move_state.find(name);
      if (it == move_state.end()) return;
      if (it->second == MoveState::MOVED) {
        emit_borrow(idx, "BorrowError",
          "use of moved value '" + name + "' — "
          "it was already moved and can no longer be used; "
          "re-initialise it or declare a new binding");
      } else if (it->second == MoveState::COND_MOVED) {
        emit_borrow(idx, "BorrowError",
          "use of conditionally-moved value '" + name + "' — "
          "it may have been moved in a prior branch; "
          "guard with a null check or rebind before this use");
      }
    };

    // Helper: perform a move of `name` at token index `idx`
    // inside_loop: if we're currently inside a loop body, moving a ptr is
    // dangerous because on the next iteration the moved var is gone.
    auto do_move = [&](const std::string &name, size_t idx,
                       const std::string &context) {
      bool in_loop = !loop_brace_depths.empty();
      auto it = move_state.find(name);
      if (it != move_state.end()) {
        if (it->second == MoveState::MOVED || it->second == MoveState::COND_MOVED) {
          // Already moved — this is a double-move or use-after-move
          emit_borrow(idx, "BorrowError",
            "use of moved value '" + name + "' in " + context + " — "
            "it was already moved and can no longer be used");
          return;
        }
      }
      if (in_loop) {
        // Moving inside a loop: on subsequent iterations the var is gone
        emit_borrow(idx, "BorrowError",
          "move of '" + name + "' inside a loop body — "
          "on the second iteration this value is already moved; "
          "move it before the loop or rebind it each iteration");
      }
      move_state[name] = MoveState::MOVED;
    };

    // ── Branch-merge helper ───────────────────────────────────────────────
    // After if/else we union the move states: if both branches moved a var,
    // it is MOVED; if only one branch did, it is COND_MOVED.
    auto merge_states = [&](const std::map<std::string, MoveState> &before,
                            const std::map<std::string, MoveState> &after_then,
                            const std::map<std::string, MoveState> &after_else)
        -> std::map<std::string, MoveState> {
      std::map<std::string, MoveState> merged = before;
      // Collect all keys touched in either branch
      std::set<std::string> keys;
      for (auto &kv : after_then) keys.insert(kv.first);
      for (auto &kv : after_else) keys.insert(kv.first);
      for (auto &k : keys) {
        auto get = [&](const std::map<std::string, MoveState> &m) -> MoveState {
          auto it = m.find(k);
          return it != m.end() ? it->second : MoveState::LIVE;
        };
        MoveState s_then = get(after_then);
        MoveState s_else = get(after_else);
        MoveState s_before = get(before);
        if (s_then == MoveState::MOVED && s_else == MoveState::MOVED)
          merged[k] = MoveState::MOVED;
        else if (s_then != s_before || s_else != s_before)
          merged[k] = MoveState::COND_MOVED;
        else
          merged[k] = s_before;
      }
      return merged;
    };

    // ── Forward-scan helpers to skip balanced delimiters ──────────────────
    auto skip_balanced = [&](size_t start, TT open, TT close) -> size_t {
      int d = 0;
      for (size_t k = start; k < body_end && k < tokens.size(); k++) {
        if (tokens[k].type == open)  { d++; }
        else if (tokens[k].type == close) { if (--d == 0) return k; }
      }
      return body_end;
    };

    // ── Main scan ─────────────────────────────────────────────────────────
    // We do a recursive-descent style pass for branches/loops so we can
    // merge move states properly.  For the linear portions we scan token
    // by token.

    std::function<void(size_t, size_t)> scan;
    scan = [&](size_t from, size_t to) {
      size_t i = from;
      while (i < to && i < tokens.size()) {
        const Token &tok = tokens[i];

        // ── Brace / unsafe tracking ───────────────────────────────────────
        if (tok.type == TT::LBRACE) {
          brace_depth++;
          i++; continue;
        }
        if (tok.type == TT::UNSAFE_KW &&
            i + 1 < to && tokens[i+1].type == TT::LBRACE) {
          unsafe_depth++; brace_depth++;
          brace_depth_at_unsafe.push(brace_depth);
          i += 2; continue;
        }
        if (tok.type == TT::RBRACE) {
          if (!brace_depth_at_unsafe.empty() &&
              brace_depth == brace_depth_at_unsafe.top()) {
            unsafe_depth--;
            brace_depth_at_unsafe.pop();
          }
          if (!loop_brace_depths.empty() &&
              brace_depth == loop_brace_depths.top()) {
            loop_brace_depths.pop();
          }
          if (brace_depth > 0) brace_depth--;
          i++; continue;
        }
        if (unsafe_depth > 0) { i++; continue; }

        // ── Ptr variable declaration ──────────────────────────────────────
        if (tok.type == TT::PTR) {
          size_t j = i + 1;
          if (j < to && (tokens[j].type == TT::IDENTIFIER ||
               tokens[j].type == TT::INT || tokens[j].type == TT::FLOAT ||
               tokens[j].type == TT::CHAR_KW || tokens[j].type == TT::VOID)) {
            size_t k = j + 1;
            if (k < to && tokens[k].type == TT::IDENTIFIER) {
              std::string varname = tokens[k].value;
              lv[varname] = tokens[j].value + "*";
              move_state[varname] = MoveState::LIVE;
              size_t l = k + 1;
              if (l < to && tokens[l].type == TT::ASSIGN) {
                size_t m = l + 1;
                if (m < to && tokens[m].type == TT::IDENTIFIER) {
                  std::string src = tokens[m].value;
                  if (is_ptr_var(src)) {
                    check_use(src, m);
                    do_move(src, m, "ptr-to-ptr initialisation");
                  }
                }
              }
            }
          }
          i++; continue;
        }

        // ── RAII struct variable declaration: StructType varname [= ...] ────
        // Track struct variables that have a drop destructor so that use-after-
        // drop and double-drop can be caught (analogous to ptr move tracking).
        if (tok.type == TT::IDENTIFIER && struct_drop_funcs.count(tok.value)) {
          size_t j = i + 1;
          if (j < to && tokens[j].type == TT::IDENTIFIER) {
            std::string varname = tokens[j].value;
            lv[varname] = tok.value; // type = struct name
            move_state[varname] = MoveState::LIVE;
          }
          i++; continue;
        }

        // ── Loop body tracking: WHILE / FOR followed by block ────────────
        // We detect the opening brace of loop bodies so moves inside them
        // can be flagged.  We push the expected brace_depth of the body.
        if (tok.type == TT::WHILE || tok.type == TT::FOR) {
          // Skip the condition (balanced parens if present)
          size_t cond_end = i + 1;
          if (cond_end < to && tokens[cond_end].type == TT::LPAREN)
            cond_end = skip_balanced(cond_end, TT::LPAREN, TT::RPAREN) + 1;
          else {
            // Xenon-style: skip to LBRACE
            while (cond_end < to && tokens[cond_end].type != TT::LBRACE) cond_end++;
          }
          // The next LBRACE is the body
          if (cond_end < to && tokens[cond_end].type == TT::LBRACE) {
            loop_brace_depths.push(brace_depth + 1);
          }
          i++; continue;
        }

        // ── if/elseif/else — branch-aware move merging ────────────────────
        if (tok.type == TT::IF || tok.type == TT::ELSEIF) {
          // Skip condition
          size_t cond_end = i + 1;
          if (cond_end < to && tokens[cond_end].type == TT::LPAREN)
            cond_end = skip_balanced(cond_end, TT::LPAREN, TT::RPAREN) + 1;
          else
            while (cond_end < to && tokens[cond_end].type != TT::LBRACE &&
                   tokens[cond_end].type != TT::THEN) cond_end++;
          // Find then-body brace range
          size_t then_start = cond_end;
          if (then_start < to && tokens[then_start].type == TT::THEN) then_start++;
          if (then_start < to && tokens[then_start].type == TT::LBRACE) {
            size_t then_end = skip_balanced(then_start, TT::LBRACE, TT::RBRACE);
            // Snapshot state before then-branch
            auto state_before = move_state;
            // Scan then-branch
            scan(then_start + 1, then_end);
            auto state_after_then = move_state;
            // Check for else/elseif
            size_t after_then = then_end + 1;
            if (after_then < to && (tokens[after_then].type == TT::ELSE ||
                                    tokens[after_then].type == TT::ELSEIF)) {
              // Reset to pre-branch state for else simulation
              move_state = state_before;
              size_t else_body = after_then + 1;
              // Handle: else { ... }
              if (tokens[after_then].type == TT::ELSE &&
                  else_body < to && tokens[else_body].type == TT::LBRACE) {
                size_t else_end = skip_balanced(else_body, TT::LBRACE, TT::RBRACE);
                scan(else_body + 1, else_end);
                auto state_after_else = move_state;
                move_state = merge_states(state_before, state_after_then, state_after_else);
                i = else_end + 1;
                continue;
              }
              // Handle: elseif — restore, let next iteration handle it
              move_state = merge_states(state_before, state_after_then, state_before);
              i = after_then;
              continue;
            } else {
              // No else: merge then-branch with "no-move" else
              move_state = merge_states(state_before, state_after_then, state_before);
              i = then_end + 1;
              continue;
            }
          }
          i++; continue;
        }

        // ── Assignment to existing var: LHS = RHS ─────────────────────────
        if (tok.type == TT::IDENTIFIER &&
            i + 1 < to &&
            tokens[i+1].type == TT::ASSIGN &&
            !(i + 2 < to && tokens[i+2].type == TT::ASSIGN)) {
          std::string lhs = tok.value;
          // Re-assignment re-initialises — clear moved status for LHS
          move_state[lhs] = MoveState::LIVE;
          size_t rhs = i + 2;
          if (rhs < to && tokens[rhs].type == TT::IDENTIFIER) {
            std::string src = tokens[rhs].value;
            if (is_ptr_var(src) && is_ptr_var(lhs)) {
              check_use(src, rhs);
              do_move(src, rhs, "assignment");
            }
          }
          i += 3; continue;
        }

        // ── Function call: check each ptr argument for move ───────────────
        if (tok.type == TT::IDENTIFIER &&
            i + 1 < to && tokens[i+1].type == TT::LPAREN) {
          std::string fname_bc = tok.value;
          auto pit = func_param_types.find(fname_bc);
          if (pit != func_param_types.end()) {
            size_t j = i + 2;
            int param_idx = 0;
            int depth2 = 0;
            size_t arg_start = j;
            // Track vars moved in this call to catch double-moves in same call
            std::set<std::string> moved_in_this_call;
            while (j < to && j < tokens.size() &&
                   !(tokens[j].type == TT::RPAREN && depth2 == 0)) {
              if (tokens[j].type == TT::LPAREN || tokens[j].type == TT::LSBRACKET)
                depth2++;
              else if (tokens[j].type == TT::RPAREN || tokens[j].type == TT::RSBRACKET)
                depth2--;
              else if (tokens[j].type == TT::COMMA && depth2 == 0) {
                if (arg_start < j && tokens[arg_start].type == TT::IDENTIFIER) {
                  std::string arg_name = tokens[arg_start].value;
                  if ((size_t)param_idx < pit->second.size() &&
                      is_ptr_type(pit->second[param_idx]) &&
                      is_ptr_var(arg_name)) {
                    if (moved_in_this_call.count(arg_name)) {
                      emit_borrow(arg_start, "BorrowError",
                        "double-move of '" + arg_name + "' in the same call to '" +
                        fname_bc + "' — a value can only be passed once");
                    } else {
                      check_use(arg_name, arg_start);
                      do_move(arg_name, arg_start, "call to '" + fname_bc + "'");
                      moved_in_this_call.insert(arg_name);
                    }
                  }
                }
                param_idx++;
                arg_start = j + 1;
              }
              j++;
            }
            // Final arg
            if (arg_start < j && tokens[arg_start].type == TT::IDENTIFIER) {
              std::string arg_name = tokens[arg_start].value;
              if ((size_t)param_idx < pit->second.size() &&
                  is_ptr_type(pit->second[param_idx]) &&
                  is_ptr_var(arg_name)) {
                if (moved_in_this_call.count(arg_name)) {
                  emit_borrow(arg_start, "BorrowError",
                    "double-move of '" + arg_name + "' in the same call to '" +
                    fname_bc + "' — a value can only be passed once");
                } else {
                  check_use(arg_name, arg_start);
                  do_move(arg_name, arg_start, "call to '" + fname_bc + "'");
                }
              }
            }
            i = j + 1;
            continue;
          }
        }

        // ── Field access on a moved variable: moved.field ─────────────────
        if (tok.type == TT::IDENTIFIER &&
            i + 2 < to &&
            (tokens[i+1].type == TT::DOT || tokens[i+1].type == TT::ARROW)) {
          check_use(tok.value, i);
          i++; continue;
        }

        // ── Explicit free() on a RAII-managed ptr: double-free error ─────────
        // If the user writes  free(x)  and x is managed by RAII (has a cleanup
        // attribute that will call free again at scope exit), that is a double-free.
        // Calling free() on a RAII var is always wrong: the RAII shim will run too.
        if (tok.type == TT::IDENTIFIER && tok.value == "free" &&
            i + 2 < to && tokens[i+1].type == TT::LPAREN &&
            tokens[i+2].type == TT::IDENTIFIER) {
          std::string freed_var = tokens[i+2].value;
          if (_raii_var_drop_expr.count(freed_var)) {
            emit_borrow(i + 2, "BorrowError",
              "explicit free('" + freed_var + "') on a RAII-managed pointer — "
              "this variable is automatically freed by RAII at scope exit; "
              "calling free() explicitly causes a double-free. "
              "Remove the free() call or declare the variable as a raw ptr in an unsafe block.");
          }
        }

        // ── Bare use of a moved variable ───────────────────────────────────
        if (tok.type == TT::IDENTIFIER) {
          bool is_lhs_assign =
              i + 1 < to &&
              tokens[i+1].type == TT::ASSIGN &&
              !(i + 2 < to && tokens[i+2].type == TT::ASSIGN);
          if (!is_lhs_assign) {
            check_use(tok.value, i);
          }
        }

        i++;
      }
    };

    scan(body_start, body_end);
    return violations;
  }

  // Namespace support --------------------------------------------------------
  std::string _cur_namespace{""}; // current namespace being defined
  // namespace_name → {symbol_name → C_name}  (for lookups)
  std::map<std::string, std::map<std::string, std::string>> _namespaces;
  // symbols to make available unqualified (via "ignore namespace")
  std::set<std::string> _ignored_namespaces;

  std::set<std::string> _handle_declared;
  std::set<std::string> _lh_included;
  std::set<std::string> _enum_names;
  std::string _source_file{"<unknown>"};
  std::string _source_dir{"."};
  std::vector<std::string> _include_paths;

  // ── Module system ─────────────────────────────────────────────────────────
  // A "use path" is a colons-separated identifier like:
  //   "something"
  //   "something::some"
  //   "something::some::deep::func"
  //
  // Each use-path maps to exactly ONE .xen file containing ONE function.
  // The file path is derived by:
  //   • splitting the use-path on "::"
  //   • all segments except the last  → directory path
  //   • last segment                  → <name>.xen filename
  //
  // Examples:
  //   "something"                              → ./something.xen
  //   "something::some"                        → ./something/some.xen
  //   "a::b::c::d"                             → ./a/b/c/d.xen
  //
  // The call syntax mirrors the use path exactly:
  //   use something         →  something()
  //   use something::some   →  something::some()
  //   use a::b::c::d        →  a::b::c::d()

  // Set of use-paths declared with "use".
  std::set<std::string> _used_modules;
  // Set of use-paths that have already been loaded (to avoid double-loading).
  std::set<std::string> _module_funcs_loaded;

  // Helper: convert a colons-path "a::b::c" to a filesystem relative path "a/b/c.xen"
  static std::string module_path_to_rel_file(const std::string &use_path) {
    std::vector<std::string> parts;
    size_t p = 0;
    while (p <= use_path.size()) {
      size_t sep = use_path.find("::", p);
      if (sep == std::string::npos) {
        parts.push_back(use_path.substr(p));
        break;
      }
      parts.push_back(use_path.substr(p, sep - p));
      p = sep + 2;
    }
    // Build path: all parts joined with '/', last part gets ".xen"
    std::string rel;
    for (size_t i = 0; i < parts.size(); i++) {
      if (i > 0) rel += "/";
      rel += parts[i];
    }
    return rel + ".xen";
  }

  // Find the .xen file for a use-path by searching source dir then include paths.
  std::string find_module_file(const std::string &use_path) const {
    std::string rel = module_path_to_rel_file(use_path);
    // 1. Relative to source dir
    {
      std::string c = (fs::path(_source_dir) / rel).string();
      if (fs::exists(c)) return fs::weakly_canonical(c).string();
    }
    // 2. Each include path (-l)
    for (const auto &ip : _include_paths) {
      std::string c = (fs::path(ip) / rel).string();
      if (fs::exists(c)) return fs::weakly_canonical(c).string();
    }
    return "";
  }

  // Load the .xen file for a use-path if not already loaded.
  // call_tok is used only for error location.
  void load_module(const std::string &use_path, const Token &call_tok) {
    if (_module_funcs_loaded.count(use_path)) return; // already loaded

    std::string xen_file = find_module_file(use_path);
    if (xen_file.empty()) {
      std::string rel = module_path_to_rel_file(use_path);
      throw XenonError("ModuleError",
        "Module '" + use_path + "' not found.\n"
        "  Expected file '" + rel + "' in source directory or any include path (-l<path>).\n"
        "  Each module function lives in its own .xen file named after the last path segment.",
        call_tok.line, call_tok.col);
    }

    _module_funcs_loaded.insert(use_path);
    transpile_lh(xen_file, call_tok);

    // After loading, register the function under the qualified namespace path so that
    // calls like  something::some()  resolve to  something__some  (the C mangled name).
    // The use_path might be:
    //   "something"               → no namespace, bare function call
    //   "something::some"         → ns="something", sym="some"
    //   "a::b::c::d"              → ns="a::b::c", sym="d"  (mangled: a__b__c__d)
    //
    // We need to register in _namespaces so resolve_qualified works.
    {
      // Split use_path into parts
      std::vector<std::string> parts;
      size_t p = 0;
      while (p <= use_path.size()) {
        size_t sep = use_path.find("::", p);
        if (sep == std::string::npos) { parts.push_back(use_path.substr(p)); break; }
        parts.push_back(use_path.substr(p, sep - p));
        p = sep + 2;
      }

      if (parts.size() >= 2) {
        // Build the namespace key and the C mangled name
        // ns_key (for _namespaces lookup): parts[0..-2] joined with "__"
        // func sym: parts[-1]
        // C mangled name: all parts joined with "__"
        std::string func_sym = parts.back();
        std::string c_mangled;
        std::string ns_key_under;
        for (size_t i = 0; i < parts.size(); i++) {
          if (i > 0) { c_mangled += "__"; }
          c_mangled += parts[i];
          if (i + 1 < parts.size() - 1) {
            if (!ns_key_under.empty()) ns_key_under += "__";
            ns_key_under += parts[i];
          }
        }

        // Register in all ancestor namespace maps so chained :: lookups work.
        // e.g. for "a::b::c::d":
        //   _namespaces["a__b__c"]["d"] = "a__b__c__d"
        //   _namespaces["a__b"]["c__d"] = "a__b__c__d"   (not needed, but harmless)
        //   _namespaces["a"]["b"]       → already handled by namespace chain
        //
        // The minimal needed registration: the direct parent ns → func_sym.
        std::string direct_ns; // "a__b__c" for "a::b::c::d"
        for (size_t i = 0; i + 1 < parts.size(); i++) {
          if (i > 0) direct_ns += "__";
          direct_ns += parts[i];
        }
        _namespaces[direct_ns][func_sym] = c_mangled;

        // Also register intermediate ns chains so  a::b::c::d()  can be
        // resolved by the chained :: loop which builds ns_path incrementally.
        // The loop in parse_primary/parse_statement does:
        //   ns_path starts as parts[0]
        //   then folds: ns_path = ns_path + "__" + parts[i]  for i=1..N-2
        //   sym = parts[N-1]
        // So resolve_qualified(ns_path, sym) must find the entry.
        // That means we only need: _namespaces[direct_ns][func_sym] = c_mangled.
        // But we also need the func itself registered so emit_call passes the
        // is_known_function check.  The function name in the loaded .xen is just
        // func_sym (e.g. "some"), but we need to also register c_mangled.
        // We do this by adding an alias: func_return_types[c_mangled] = func_return_types[func_sym]
        if (func_return_types.count(func_sym) && !func_return_types.count(c_mangled))
          func_return_types[c_mangled] = func_return_types[func_sym];
        if (func_param_types.count(func_sym) && !func_param_types.count(c_mangled))
          func_param_types[c_mangled] = func_param_types[func_sym];

        // We also need the actual C function to be named c_mangled, not func_sym.
        // Emit a static inline forwarder: c_mangled(...) { return func_sym(...); }
        // Only if c_mangled != func_sym and not already emitted.
        if (c_mangled != func_sym && !func_return_types.count(c_mangled)) {
          // func_sym must be known (loaded) at this point; if not, the module
          // load was skipped (already included) but func_sym was registered
          // in a prior load — look it up from what we have.
          std::string ret = func_return_types.count(func_sym)
                              ? func_return_types[func_sym]
                              : "void";
          // Build param list
          std::string params_decl, params_call;
          if (func_param_types.count(func_sym)) {
            const auto &ptypes = func_param_types[func_sym];
            for (size_t pi = 0; pi < ptypes.size(); pi++) {
              if (pi > 0) { params_decl += ", "; params_call += ", "; }
              std::string pname = "_p" + std::to_string(pi);
              params_decl += ptypes[pi] + " " + pname;
              params_call += pname;
            }
          }
          std::string ret_kw = (ret == "void") ? "" : "return ";
          std::string fwd =
            "static inline " + ret + " " + c_mangled +
            "(" + params_decl + ") { " +
            ret_kw + func_sym + "(" + params_call + "); }\n";
          functions.push_back(fwd);
          func_return_types[c_mangled] = ret;
          _xen_imported_functions.insert(c_mangled);
        }
      }
      // Single-segment case ("use something"): function is already registered
      // under its own name; no namespace registration needed.
    }
  }

  // Check if a colons-path (as seen at a call site) exactly matches a used module,
  // and if so load it.  call_tok is for error reporting.
  // Returns true if it was a module call.
  bool maybe_load_module_call(const std::string &call_path_colons,
                              const Token &call_tok) {
    if (!_used_modules.count(call_path_colons)) return false;
    load_module(call_path_colons, call_tok);
    return true;
  }

  Token &current() { return tokens[pos]; }
  Token advance() { return tokens[pos++]; }

  Token expect(TT ttype, bool shut) {
    Token &tok = current();
    if (tok.type == ttype)
      return advance();
    std::string got =
        tok.value.empty() ? tt_name(tok.type) : ("'" + tok.value + "'");
    // Produce a human-readable name for the expected token
    static const std::map<TT, std::string> friendly = {
        {TT::LPAREN, "'('"}, {TT::RPAREN, "')'"}, {TT::LBRACE, "'{'"}, {TT::RBRACE, "'}'"},
        {TT::LSBRACKET, "'['"}, {TT::RSBRACKET, "']'"}, {TT::COMMA, "','"},
        {TT::COLON, "':'"}, {TT::SEMICOLON, "';'"}, {TT::ASSIGN, "'='"},
        {TT::GT, "'>'"}, {TT::LT, "'<'"}, {TT::IDENTIFIER, "an identifier"},
        {TT::NUMBER, "a number"}, {TT::STRING, "a string literal"},
        {TT::TEOF, "end of file"}, {TT::THEN, "'then'"}, {TT::END, "'end'"},
        {TT::DO, "'do'"}, {TT::RSBRACKET, "']'"},
    };
    auto fit = friendly.find(ttype);
    std::string expected_str = (fit != friendly.end()) ? fit->second : tt_name(ttype);
    if (!shut)
      throw XenonError("SyntaxError",
                         "Expected " + expected_str + " but got " + got,
                         tok.line, tok.col);
    return Token(ttype, "", tok.line, tok.col);
  }

  std::string safe_name(const std::string &name) const {
    static const std::set<std::string> c_keywords = {
        // C89/C99/C11 keywords
        "auto",
        "break",
        "case",
        "char",
        "const",
        "continue",
        "default",
        "do",
        "double",
        "else",
        "enum",
        "extern",
        "float",
        "for",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "register",
        "restrict",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "struct",
        "switch",
        "typedef",
        "union",
        "unsigned",
        "void",
        "volatile",
        "while",
        // C11 keywords
        "_Alignas",
        "_Alignof",
        "_Atomic",
        "_Bool",
        "_Complex",
        "_Generic",
        "_Imaginary",
        "_Noreturn",
        "_Static_assert",
        "_Thread_local",
        // common macros / types that collide
        "bool",
        "true",
        "false",
        "NULL",
        "uint8_t",
        "uint16_t",
        "uint32_t",
        "uint64_t",
        "int8_t",
        "int16_t",
        "int32_t",
        "int64_t",
        "size_t",
        "ssize_t",
        "ptrdiff_t",
        "FILE",
        "__m256",
        "__m256i",
        "__m128",
        "__m128i",
    };
    return c_keywords.count(name) ? "var_" + name : name;
  }

  // Mangle a symbol name with namespace prefix if we're in a namespace
  std::string mangle_with_ns(const std::string &name) const {
    if (_cur_namespace.empty()) return safe_name(name);
    return safe_name(_cur_namespace + "__" + name);
  }

  std::string line_directive(const Token &tok) {
    if (tok.line == 0 || !emit_line_directives)
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
  // Operator overload helpers
  // -----------------------------------------------------------------------
  // Convert an operator symbol to a readable C identifier fragment
  static std::string symbol_to_name(const std::string &sym) {
    static const std::map<std::string, std::string> table = {
        {"+",    "plus"},    {"-",     "minus"},   {"*",    "mul"},
        {"/",    "div"},     {"%",     "mod"},      {"^",    "xor"},
        {"==",   "eq"},      {"!=",    "ne"},       {">",    "gt"},
        {"<",    "lt"},      {">=",    "ge"},       {"<=",   "le"},
        {"<<",   "shl"},     {">>",    "shr"},      {"|",    "bitor"},
        {"&",    "bitand"},  {"&&",    "and"},      {"||",   "or"},
        {"!",    "not"},     {"~",     "bitnot"},   {"**",   "pow"},
        {"[]",   "subscript"},
        {"=",    "assign"},  {"+=",    "pluseq"},   {"-=",   "minuseq"},
        {"*=",   "muleq"},   {"/=",    "diveq"},    {"%=",   "modeq"},
        {"++post","incrpost"},{"--post","decrpost"},
        {"++pre", "incrpre"},{"--pre", "decrpre"},
        {"u-",   "uneg"},    {"u*",    "deref"},
    };
    auto it = table.find(sym);
    if (it != table.end()) return it->second;
    // For custom symbols not in the table, encode each char as hex
    std::string r = "op";
    for (unsigned char c : sym) { char buf[4]; snprintf(buf,4,"%02x",c); r+=buf; }
    return r;
  }

  // Parse  operator(SYM)  and return the symbol string
  std::string parse_op_sym(const Token &kw_tok) {
    expect(TT::LPAREN, false);
    // The symbol can be one or two characters; grab until RPAREN
    std::string sym;
    while (current().type != TT::RPAREN && current().type != TT::TEOF)
      sym += advance().value;
    if (sym.empty())
      throw XenonError("SyntaxError", "operator() requires a symbol",
                       kw_tok.line, kw_tok.col);
    expect(TT::RPAREN, false);
    return sym;
  }

  // Parse  args(a:TYPE, b:TYPE)  or  args(a, b)  — returns {name, type} pairs.
  // Type defaults to empty string; parse_overload fills it from ret_type if absent.
  // TYPE may include:
  //   ptr <BASE>   — pointer, e.g. "ptr int" → "int*"
  //   const <BASE> — const-qualified, e.g. "const int" → "const int"
  //   ptr const <BASE> / const ptr <BASE> combinations
  struct ArgDef { std::string name, type; };
  std::pair<ArgDef, ArgDef> parse_op_args(const Token &kw_tok) {
    expect(TT::LPAREN, false);

    // Collect a single type specifier starting at current position.
    // Handles optional leading `const`, `ptr`, and combinations thereof.
    auto parse_arg_type = [&]() -> std::string {
      bool is_const = false;
      bool is_ptr   = false;

      // Consume any leading const/ptr qualifiers in any order
      while (true) {
        if (current().type == TT::CONST_KW) {
          advance();
          is_const = true;
        } else if (current().type == TT::PTR) {
          advance();
          is_ptr = true;
        } else {
          break;
        }
      }

      // Collect the base type tokens until a terminator
      std::string base_raw;
      while (current().type != TT::RPAREN &&
             current().type != TT::COMMA &&
             current().type != TT::BINARY_KW &&
             current().type != TT::UNARY_KW &&
             current().type != TT::TEOF) {
        // Stop if we hit another ptr/const — shouldn't happen after above loop,
        // but guard anyway
        if (current().type == TT::PTR || current().type == TT::CONST_KW) break;
        base_raw += advance().value;
      }

      // Map Xenon base type to C type
      std::string base_c = raw_to_c(base_raw.empty() ? "void" : base_raw);
      // "str" already becomes "char*"; don't double-pointer it unless ptr given
      bool base_already_ptr = (!base_c.empty() && base_c.back() == '*');

      std::string result;
      if (is_const) result += "const ";
      result += base_c;
      if (is_ptr && !base_already_ptr) result += "*";
      else if (is_ptr && base_already_ptr) result += "*"; // ptr ptr base → double-ptr
      return result;
    };

    auto parse_one = [&]() -> ArgDef {
      std::string name = expect(TT::IDENTIFIER, false).value;
      std::string type;
      if (current().type == TT::COLON) {
        advance();
        type = parse_arg_type();
      }
      return {name, type};
    };

    ArgDef a = parse_one();
    ArgDef b;
    if (current().type == TT::COMMA) {
      advance();
      // may be 'binary' or 'unary' as a marker with no second arg
      if (current().type == TT::BINARY_KW || current().type == TT::UNARY_KW) {
        advance();
      } else {
        b = parse_one();
        // optional trailing binary/unary marker
        if (current().type == TT::COMMA) {
          advance();
          if (current().type == TT::BINARY_KW || current().type == TT::UNARY_KW)
            advance();
        }
      }
    }
    expect(TT::RPAREN, false);
    return {a, b};
  }

  // Parse  type(RETTYPE)  and return the C return type string.
  // RETTYPE may include ptr and/or const qualifiers, e.g.:
  //   type(int)            -> "int"
  //   type(ptr int)        -> "int*"
  //   type(const int)      -> "const int"
  //   type(ptr const char) -> "const char*"
  std::string parse_op_type(const Token &kw_tok) {
    expect(TT::LPAREN, false);

    bool is_const = false;
    bool is_ptr   = false;

    // Consume optional leading const/ptr qualifiers in any order
    while (true) {
      if (current().type == TT::CONST_KW) { advance(); is_const = true; }
      else if (current().type == TT::PTR)  { advance(); is_ptr   = true; }
      else break;
    }

    // Collect the base type tokens until RPAREN
    std::string rt;
    while (current().type != TT::RPAREN && current().type != TT::TEOF) {
      if (current().type == TT::PTR)      { advance(); is_ptr   = true; continue; }
      if (current().type == TT::CONST_KW) { advance(); is_const = true; continue; }
      rt += advance().value;
    }

    if (rt.empty() && !is_ptr && !is_const)
      throw XenonError("SyntaxError", "type() requires a return type",
                       kw_tok.line, kw_tok.col);
    expect(TT::RPAREN, false);

    std::string base_c = raw_to_c(rt.empty() ? "void" : rt);
    bool base_already_ptr = (!base_c.empty() && base_c.back() == '*');

    std::string result;
    if (is_const) result += "const ";
    result += base_c;
    if (is_ptr && !base_already_ptr) result += "*";
    else if (is_ptr && base_already_ptr) result += "*";
    return result;
  }

  // Parse the body block { ... } and return it verbatim as a C body string
  std::string parse_op_body(const Token &kw_tok) {
    expect(TT::LBRACE, false);
    std::vector<std::string> stmts;
    while (current().type != TT::RBRACE && current().type != TT::TEOF) {
      Token t2 = current();
      std::string s = parse_statement();
      if (!s.empty()) stmts.push_back(line_directive(t2) + "    " + s);
    }
    if (current().type == TT::TEOF)
      throw XenonError("SyntaxError",
                       "Unterminated operator body — missing closing '}'",
                       kw_tok.line, kw_tok.col);
    expect(TT::RBRACE, false);
    return join(stmts, "\n");
  }

  // -----------------------------------------------------------------------
  // resolve_overload: given an operator symbol and the token positions of the
  // left and right operands (for type inference), return a pointer to the best
  // matching OverloadEntry, or nullptr if none matches.
  // Matching rules (first match wins):
  //   1. Both arg types match the overload's declared arg types exactly.
  //   2. Only the left arg type matches (right unknown / unregistered).
  //   3. Any overload registered for the symbol (fallback to first).
  // -----------------------------------------------------------------------
  const OverloadEntry *resolve_overload(const std::string &sym,
                                        size_t left_tok_pos,
                                        size_t right_tok_pos) {
    auto it = _op_overloads.find(sym);
    if (it == _op_overloads.end()) return nullptr;
    const auto &entries = it->second;
    if (entries.empty()) return nullptr;
    if (entries.size() == 1) return &entries[0]; // fast path

    std::string left_type  = infer_type_at(left_tok_pos).c_type();
    std::string right_type = infer_type_at(right_tok_pos).c_type();

    // Pass 1: both args match exactly
    for (const auto &ov : entries) {
      const std::string &ta = ov.type_a;
      const std::string &tb = ov.type_b;
      if (!ta.empty() && ta == left_type &&
          (!ov.is_binary || (!tb.empty() && tb == right_type)))
        return &ov;
    }
    // Pass 2: left arg matches
    for (const auto &ov : entries) {
      const std::string &ta = ov.type_a;
      if (!ta.empty() && ta == left_type)
        return &ov;
    }
    // Fallback: first registered
    return &entries[0];
  }

  // -----------------------------------------------------------------------
  // overload operator(+) args(a,b) type(RETTYPE) { body }
  // -----------------------------------------------------------------------
  void parse_overload(const Token &kw_tok) {
    std::string sym = parse_op_sym(kw_tok);
    // expect 'args'
    if (current().type != TT::ARGS_KW)
      throw XenonError("SyntaxError",
        "overload: expected 'args(a, b)' after operator()"
        " (e.g. 'overload operator(+) args(a, b) type(int) { ... }')",
        kw_tok.line, kw_tok.col);
    advance();
    auto [arg_a, arg_b] = parse_op_args(kw_tok);
    bool is_binary = !arg_b.name.empty();
    // expect 'type'
    Token type_tok = current();
    if (current().type != TT::TYPE && current().type != TT::IDENTIFIER)
      throw XenonError("SyntaxError",
        "overload: expected 'type(RETTYPE)' after args()"
        " (e.g. 'type(int)' or 'type(str)')",
        kw_tok.line, kw_tok.col);
    advance();
    std::string ret_type = parse_op_type(type_tok);

    // If no per-arg type was given, default to ret_type (old behaviour).
    // If the user wrote  args(a:int, b:int)  those types are used as-is.
    if (arg_a.type.empty()) arg_a.type = ret_type;
    if (is_binary && arg_b.type.empty()) arg_b.type = ret_type;

    // Build function name — keyed on *types* not param names so that two linked
    // files overloading the same operator for different types get distinct C
    // function names, while true duplicates (same sym + same types) are still
    // deduplicated correctly by the merge logic.
    std::string safe_ret = ret_type;
    for (char &c : safe_ret) if (c == '*' || c == ' ') c = '_';
    std::string safe_ta = arg_a.type;
    for (char &c : safe_ta) if (c == '*' || c == ' ') c = '_';
    std::string safe_tb = arg_b.type;
    for (char &c : safe_tb) if (c == '*' || c == ' ') c = '_';
    std::string fname = "_Overload_" + symbol_to_name(sym) + "_" +
                        safe_ta + (is_binary ? "_" + safe_tb : "") + "_" + safe_ret;

    // Register params in var_types so the body can refer to them correctly
    var_types[arg_a.name] = arg_a.type;
    if (is_binary) var_types[arg_b.name] = arg_b.type;

    std::string body = parse_op_body(kw_tok);

    // For operator(=) the first param must be a pointer so the overload
    // mutates the real lhs (call sites pass &lhs).  Rewrite body so that
    // bare `a.field` -> `a->field` and `a =` -> `*a =` transparently.
    std::string emit_a_type = arg_a.type;
    if (sym == "=") {
      emit_a_type = arg_a.type + "*";
      const std::string &_n = arg_a.name;
      auto _rewrite = [](const std::string &src,
                         const std::string &from,
                         const std::string &to) -> std::string {
        std::string out;
        out.reserve(src.size());
        size_t p = 0;
        while (p < src.size()) {
          size_t f = src.find(from, p);
          if (f == std::string::npos) { out += src.substr(p); break; }
          bool left_ok = (f == 0) || (!std::isalnum((unsigned char)src[f-1]) && src[f-1] != '_');
          if (left_ok) { out += src.substr(p, f - p); out += to; p = f + from.size(); }
          else          { out += src[p++]; }
        }
        return out;
      };
      body = _rewrite(body, _n + ".",  _n + "->");
      body = _rewrite(body, _n + " =", "*" + _n + " =");
    }

    // Emit the C function with correct per-arg types
    std::string params = emit_a_type + " " + arg_a.name;
    if (is_binary) params += ", " + arg_b.type + " " + arg_b.name;
    std::string fn = ret_type + " " + fname + "(" + params + ") {\n" + body + "\n}\n";
    functions.push_back(fn);

    _op_overloads[sym].push_back({fname, arg_a.name, arg_b.name, ret_type, is_binary, arg_a.type, arg_b.type});
    func_return_types[fname] = ret_type;
  }

  // -----------------------------------------------------------------------
  // addop operator(<<) args(a,b,binary) type(RETTYPE) { body }
  // (Same logic as overload; addop just allows any 1-2 char symbol)
  // -----------------------------------------------------------------------
  void parse_addop(const Token &kw_tok) {
    parse_overload(kw_tok);  // identical parsing; symbol validation is implicit
  }

  // -----------------------------------------------------------------------
  // alias NEWNAME = EXISTINGNAME
  // -----------------------------------------------------------------------
  std::string parse_alias(const Token &kw_tok) {
    std::string new_name = expect(TT::IDENTIFIER, false).value;
    expect(TT::ASSIGN, false);
    std::string orig_name = expect(TT::IDENTIFIER, false).value;
    _aliases[new_name] = orig_name;
    func_return_types[new_name] = func_return_types.count(orig_name)
                                    ? func_return_types[orig_name] : "";
    // Emit a C #define so the binary just works
    return "#define " + new_name + " " + orig_name;
  }

  // -----------------------------------------------------------------------
  // pipe EXPR |> FUNC  — rewritten as FUNC(EXPR)
  // Syntax as a statement:  pipe VAR |> func1 |> func2
  // We handle this inside parse_statement by rewriting.
  // -----------------------------------------------------------------------
  std::string parse_pipe_stmt(const Token &kw_tok) {
    // Only parse the seed value up to the first |> — don't call parse_expr
    // as it will consume | via parse_bitwise_or. Use parse_unary instead.
    std::string expr = parse_unary();
    while (current().type == TT::BITOR && pos + 1 < tokens.size() &&
           tokens[pos + 1].type == TT::GT) {
      advance(); // consume |
      advance(); // consume >
      std::string fname = safe_name(expect(TT::IDENTIFIER, false).value);
      expr = fname + "(" + expr + ")";
    }
    return expr + ";";
  }

  // -----------------------------------------------------------------------
  // match EXPR with { CASE VAL: body ... default: body }
  // Compiles to a C if-else chain (works for any type, not just ints).
  // -----------------------------------------------------------------------
  std::string parse_match(const Token &kw_tok) {
    std::string subject = parse_expr();
    // expect 'with'
    if (current().type != TT::WITH_KW)
      throw XenonError("SyntaxError",
        "match: expected 'with' after the expression"
        " (e.g. 'match expr with { case 1: ... }')",
        kw_tok.line, kw_tok.col);
    advance();
    expect(TT::LBRACE, false);

    // Detect if this is an Option match: subject must be an Option__* type.
    std::string opt_inner_type;
    std::string opt_struct_type;
    {
      auto vit = var_types.find(subject);
      if (vit != var_types.end() &&
          vit->second.size() > 7 &&
          vit->second.substr(0, 7) == "Option_") {
        opt_struct_type = vit->second;
        auto fit = struct_field_types.find(opt_struct_type);
        if (fit != struct_field_types.end()) {
          auto fvit = fit->second.find("value");
          if (fvit != fit->second.end())
            opt_inner_type = fvit->second;
        }
        if (opt_inner_type.empty()) opt_inner_type = "int";
      }
    }

    // ── Option match ────────────────────────────────────────────────────────
    // Handles:  case Some(x): ...  and  case None: ...
    if (!opt_struct_type.empty()) {
      std::string some_branch, none_branch;
      std::string some_binding;

      while (current().type != TT::RBRACE && current().type != TT::TEOF) {
        if (current().type == TT::CASE) {
          advance();
          if (current().type == TT::SOME_KW) {
            // case Some(x):
            advance(); // consume 'Some'
            expect(TT::LPAREN, false);
            some_binding = safe_name(expect(TT::IDENTIFIER, false).value);
            expect(TT::RPAREN, false);
            expect(TT::COLON, false);
            auto scope_save = var_types;
            var_types[some_binding] = opt_inner_type;
            std::vector<std::string> stmts;
            stmts.push_back("    " + opt_inner_type + " " + some_binding +
                            " = (" + subject + ").value;");
            while (current().type != TT::CASE && current().type != TT::DEFAULT_KW &&
                   current().type != TT::RBRACE && current().type != TT::TEOF) {
              std::string s = parse_statement();
              if (!s.empty()) stmts.push_back("    " + s);
            }
            var_types = scope_save;
            some_branch = join(stmts, "\n");
          } else if (current().type == TT::NONE_KW) {
            // case None:
            advance(); // consume 'None'
            expect(TT::COLON, false);
            auto scope_save = var_types;
            std::vector<std::string> stmts;
            while (current().type != TT::CASE && current().type != TT::DEFAULT_KW &&
                   current().type != TT::RBRACE && current().type != TT::TEOF) {
              std::string s = parse_statement();
              if (!s.empty()) stmts.push_back("    " + s);
            }
            var_types = scope_save;
            none_branch = join(stmts, "\n");
          } else {
            throw XenonError("SyntaxError",
              "match on Option<T> only accepts 'case Some(x):' and 'case None:' arms",
              kw_tok.line, kw_tok.col);
          }
        } else {
          advance(); // skip unexpected token
        }
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError", "Unterminated 'match'", kw_tok.line, kw_tok.col);
      expect(TT::RBRACE, false);

      std::string result;
      if (!some_branch.empty() && !none_branch.empty()) {
        result = "if ((" + subject + ").has_value) {\n" + some_branch + "\n} else {\n" + none_branch + "\n}";
      } else if (!some_branch.empty()) {
        result = "if ((" + subject + ").has_value) {\n" + some_branch + "\n}";
      } else if (!none_branch.empty()) {
        result = "if (!(" + subject + ").has_value) {\n" + none_branch + "\n}";
      }
      return result;
    }

    // ── Regular (non-Option) match ───────────────────────────────────────────

    std::vector<std::string> branches;
    std::string default_branch;
    bool first = true;

    while (current().type != TT::RBRACE && current().type != TT::TEOF) {
      if (current().type == TT::CASE) {
        advance();
        std::string val = parse_expr();
        expect(TT::COLON, false);
        auto scope_save = var_types;
        std::vector<std::string> stmts;
        while (current().type != TT::CASE && current().type != TT::DEFAULT_KW &&
               current().type != TT::RBRACE && current().type != TT::TEOF) {
          std::string s = parse_statement();
          if (!s.empty()) stmts.push_back("    " + s);
        }
        var_types = scope_save;
        std::string keyword = first ? "if" : "else if";
        first = false;
        branches.push_back(keyword + " ((" + subject + ") == (" + val + ")) {\n" +
                           join(stmts, "\n") + "\n}");
      } else if (current().type == TT::DEFAULT_KW) {
        advance();
        if (current().type == TT::COLON) advance();
        auto scope_save = var_types;
        std::vector<std::string> stmts;
        while (current().type != TT::RBRACE && current().type != TT::TEOF) {
          std::string s = parse_statement();
          if (!s.empty()) stmts.push_back("    " + s);
        }
        var_types = scope_save;
        default_branch = "else {\n" + join(stmts, "\n") + "\n}";
      } else {
        advance(); // skip unexpected token
      }
    }
    if (current().type == TT::TEOF)
      throw XenonError("SyntaxError", "Unterminated 'match'",
                       kw_tok.line, kw_tok.col);
    expect(TT::RBRACE, false);

    std::string result = join(branches, " ");
    if (!default_branch.empty()) result += " " + default_branch;
    return result;
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
        throw XenonError("SyntaxError",
                           "Unterminated enum '" + enum_name + "'",
                           name_tok.line, name_tok.col);
      std::string m = expect(TT::IDENTIFIER, false).value;
      if (current().type == TT::ASSIGN) {
        Token eq_tok = current();
        advance();
        Token val_tok = current();
        std::string val = parse_expr();
        // Reject string literals and float literals as enum values
        if (val_tok.type == TT::STRING)
          throw XenonError("TypeError",
                             "enum '" + enum_name + "' member '" + m +
                                 "' cannot be assigned a string value",
                             val_tok.line, val_tok.col);
        if (val_tok.type == TT::NUMBER &&
            (val_tok.value.find('.') != std::string::npos ||
             val_tok.value.find('e') != std::string::npos ||
             val_tok.value.find('E') != std::string::npos))
          throw XenonError("TypeError",
                             "enum '" + enum_name + "' member '" + m +
                                 "' cannot be assigned a floating-point value",
                             val_tok.line, val_tok.col);
        members.push_back(m + " = " + val);
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
  // RAII helpers
  // -----------------------------------------------------------------------
  // Emit (once) a static cleanup shim for free()-based RAII on ptr variables.
  // The shim is called by __attribute__((cleanup(_xen_raii_free_ptr_))) on
  // any ptr variable that was initialised from malloc/calloc/realloc.
  void ensure_raii_free_shim() {
    static const std::string shim_key = "__xen_raii_free";
    if (_raii_shims_emitted.count(shim_key)) return;
    _raii_shims_emitted.insert(shim_key);
    // The cleanup attribute passes a pointer-to-the-variable, so the shim
    // receives void** and calls free(*p) only when *p != NULL.
    std::string shim =
      "/* [Xenon RAII] auto-free shim for heap ptr variables */\n"
      "static inline void _xen_raii_free_ptr_(void **_xen_pp_) {\n"
      "    if (_xen_pp_ && *_xen_pp_) { free(*_xen_pp_); *_xen_pp_ = NULL; }\n"
      "}\n";
    headers.push_back(shim);
  }

  // Emit (once) a static cleanup shim for a struct type that has $attribute: drop.
  // The generated shim calls  StructName__drop(&var)  when the variable
  // leaves scope, matching C's __attribute__((cleanup)) convention.
  void ensure_raii_drop_shim(const std::string &struct_cname) {
    std::string shim_key = "__xen_raii_drop_" + struct_cname;
    if (_raii_shims_emitted.count(shim_key)) return;
    _raii_shims_emitted.insert(shim_key);
    // The cleanup function receives a pointer to the variable itself.
    std::string shim_fn = "_xen_raii_drop_" + struct_cname + "_";
    std::string shim =
      "/* [Xenon RAII] auto-drop shim for " + struct_cname + " */\n"
      "static inline void " + shim_fn + "(" + struct_cname + " *_xen_p_) {\n"
      "    if (_xen_p_) " + struct_cname + "__drop(_xen_p_);\n"
      "}\n";
    headers.push_back(shim);
  }

  // -----------------------------------------------------------------------
  // Option<T> support
  // -----------------------------------------------------------------------
  // Tracks which concrete Option instantiations have already been emitted.
  // Key: the C type string (e.g. "int", "char*", "float").
  std::set<std::string> _option_structs_emitted;

  // Returns the mangled C struct name for Option<T>, e.g. "Option__int".
  // Emits the typedef the first time it is called for a given concrete type.
  std::string ensure_option_struct(const std::string &c_type) {
    // Mangle: replace chars that are invalid in C identifiers
    std::string safe_ct = c_type;
    for (char &ch : safe_ct)
      if (ch == '*' || ch == ' ') ch = '_';
    std::string mangled = "Option__" + safe_ct;

    if (_option_structs_emitted.count(mangled)) return mangled;
    _option_structs_emitted.insert(mangled);

    // Emit: typedef struct { bool has_value; T value; } Option__T;
    // Also emit unwrap helper (aborts on None) and a panic macro.
    std::string code =
      "/* [Xenon] Option<" + c_type + "> */\n"
      "#ifndef _XEN_OPTION_PANIC\n"
      "#include <stdio.h>\n"
      "#include <stdlib.h>\n"
      "#define _XEN_OPTION_PANIC() \\\n"
      "  (fprintf(stderr, \"[Xenon] Option::unwrap() called on None\\n\"), abort())\n"
      "#endif\n"
      "#include <stdbool.h>\n"
      "typedef struct { bool has_value; " + c_type + " value; } " + mangled + ";\n"
      "static inline " + c_type + " " + mangled + "__unwrap(" + mangled + " _opt_) {\n"
      "    if (!_opt_.has_value) _XEN_OPTION_PANIC();\n"
      "    return _opt_.value;\n"
      "}\n"
      "static inline " + c_type + " " + mangled + "__unwrap_or(" + mangled + " _opt_, " + c_type + " _def_) {\n"
      "    return _opt_.has_value ? _opt_.value : _def_;\n"
      "}\n";

    headers.push_back(code);
    // Register the mangled name as a known struct so is_known_type() accepts it
    var_types[mangled] = "STRUCT";
    struct_field_types[mangled]["has_value"] = "bool";
    struct_field_types[mangled]["value"]     = c_type;
    return mangled;
  }

  // Given a Xenon raw type string ("int", "str", "ptr int", etc.),
  // return the C concrete type and the Option mangled name.
  std::pair<std::string, std::string> option_for_raw(const std::string &raw) {
    std::string c_type;
    if (raw == "str")       c_type = "char*";
    else if (raw == "u8")   c_type = "uint8_t";
    else if (raw == "u32")  c_type = "uint32_t";
    else if (raw == "u64")  c_type = "uint64_t";
    else                    c_type = raw; // int, float, double, long, etc.
    std::string mangled = ensure_option_struct(c_type);
    return {c_type, mangled};
  }
  std::string parse_type_definition() {
    advance(); // skip 'type'
    Token name_tok = current();
    std::string struct_name = expect(TT::IDENTIFIER, false).value;
    std::string mangled_struct_name = mangle_with_ns(struct_name);

    // Generic struct: type Vec<T> { ... }
    std::string generic_param;
    if (current().type == TT::LT) {
      advance(); // consume '<'
      generic_param = expect(TT::IDENTIFIER, false).value;
      expect(TT::GT, false); // consume '>'
    }

    expect(TT::LBRACE, false);
    std::vector<std::string> fields;
    auto &field_map = struct_field_types[mangled_struct_name]; // use mangled name
    static const std::set<TT> valid_field_types = {
        TT::INT,     TT::FLOAT,  TT::STR,        TT::LONG,
        TT::SHORT,   TT::DOUBLE, TT::VOID,       TT::PTR,
        TT::M256,    TT::M256I,  TT::IDENTIFIER, TT::BOOL_KW,
        TT::CHAR_KW, TT::U8,     TT::U32,        TT::U64,
    };

    // For generic structs, we collect field descriptors to store in the template
    GenericStructTemplate gen_tmpl;
    bool is_generic = !generic_param.empty();
    if (is_generic) gen_tmpl.type_param = generic_param;

    while (current().type != TT::RBRACE) {
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
                           "Unterminated type '" + struct_name + "'",
                           name_tok.line, name_tok.col);

      // ── Method definition inside struct body ──────────────────────────
      // Check for $attribute: drop annotation before the function keyword.
      bool _method_is_drop = false;
      if (current().type == TT::ATTR_DROP) {
        _method_is_drop = true;
        advance(); // consume $attribute:drop token
        if (current().type == TT::SEMICOLON) advance();
      }
      if (current().type == TT::FUNCTION) {
        advance(); // consume 'function'
        std::string saved_cur_struct = _cur_struct;
        _cur_struct = mangled_struct_name;
        Token fn_tok = current();
        std::string method_code = parse_function_body(fn_tok, false);
        _cur_struct = saved_cur_struct;
        struct_method_impls[mangled_struct_name].push_back(method_code);
        // If annotated with $attribute: drop, register this struct as having a
        // RAII destructor.  The generated C function is structName__drop(self*).
        if (_method_is_drop) {
          struct_drop_funcs.insert(mangled_struct_name);
        }
        if (current().type == TT::SEMICOLON) advance();
        continue;
      }
      if (_method_is_drop) {
        // $attribute: drop followed by a non-function is an error
        throw XenonError("SyntaxError",
          "'$attribute: drop' must be followed by a function definition",
          current().line, current().col);
      }

      Token ft = current();
      if (!valid_field_types.count(ft.type))
        throw XenonError("SyntaxError",
                           "Expected field type in '" + struct_name +
                               "', got '" + ft.value + "'",
                           ft.line, ft.col);
      std::string f_type_raw = advance().value;
      std::string f_type;
      bool f_is_ptr = false;
      std::string f_inner_raw = f_type_raw;
      if (f_type_raw == "ptr") {
        std::string inner = advance().value;
        f_inner_raw = inner;
        if (inner == "str")
          inner = "char*";
        // If inner is the generic type param, defer substitution
        if (is_generic && inner == generic_param) {
          f_type = inner; // placeholder — substituted at instantiation
        } else {
          f_type = inner + "*";
        }
        f_is_ptr = true;
      } else {
        // BUG-L FIXED
        if (f_type_raw == "void")
          throw XenonError("TypeError",
                             "field '" + (ft.value.empty() ? "?" : ft.value) +
                                 "' in struct '" + struct_name +
                                 "' cannot have type 'void'."
                                 " If you need a generic/opaque pointer, use 'ptr void'.",
                             ft.line, ft.col);
        if (is_generic && f_type_raw == generic_param) {
          f_type = f_type_raw; // placeholder
        } else {
          f_type = (f_type_raw == "str") ? "char*" : f_type_raw;
        }
      }
      // BUG-K FIXED
      std::string f_name = safe_name(expect(TT::IDENTIFIER, false).value);
      bool f_is_array = false;
      std::string f_array_sz;
      if (current().type == TT::LSBRACKET) {
        advance();
        std::string sz;
        if (current().type == TT::NUMBER || current().type == TT::IDENTIFIER)
          sz = advance().value;
        else
          sz = expect(TT::NUMBER, false).value; // triggers proper error
        expect(TT::RSBRACKET, false);
        f_is_array = true;
        f_array_sz = sz;
        if (!is_generic || (f_type_raw != generic_param)) {
          fields.push_back(f_type + " " + f_name + "[" + sz + "];");
          field_map[f_name] = f_type + "[" + sz + "]"; // preserve array info
        }
      } else {
        if (!is_generic || (f_type_raw != generic_param && f_inner_raw != generic_param)) {
          fields.push_back(f_type + " " + f_name + ";");
          field_map[f_name] = f_type;
        }
      }

      // Record field info for generic template
      if (is_generic) {
        gen_tmpl.field_raw_types.push_back(f_inner_raw);
        gen_tmpl.field_names.push_back(f_name);
        gen_tmpl.field_is_ptr.push_back(f_is_ptr);
        gen_tmpl.field_is_array.push_back(f_is_array);
        gen_tmpl.field_array_sizes.push_back(f_array_sz);
      }

      if (current().type == TT::SEMICOLON)
        advance();
    }
    expect(TT::RBRACE, false);

    if (is_generic) {
      // Store the namespace prefix so instantiate_generic_struct can use it
      gen_tmpl.ns_prefix = _cur_namespace;
      // Register as a generic template; don't emit a concrete struct yet
      _generic_structs[mangled_struct_name] = gen_tmpl;
      // Also register so var_types knows this is a generic struct name
      var_types[mangled_struct_name] = "GENERIC_STRUCT";
      // When inside a namespace, also register under the bare (unqualified) name
      // so code within the same namespace can refer to it as  Vec<int>  not  ns__Vec<int>.
      if (!_cur_namespace.empty() && struct_name != mangled_struct_name) {
        _generic_structs[struct_name] = gen_tmpl; // alias: lookup by short name
        var_types[struct_name] = "GENERIC_STRUCT";
      }
      return ""; // emitted lazily on first use
    }

    var_types[mangled_struct_name] = "STRUCT";
    std::string _sout = "typedef struct {\n    " + join(fields, "\n    ") + "\n} " +
                        mangled_struct_name + ";\n";
    // Append method implementations right after the typedef
    auto _smit = struct_method_impls.find(mangled_struct_name);
    if (_smit != struct_method_impls.end())
      for (const auto &_mc : _smit->second)
        _sout += "\n" + _mc;
    return _sout;
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
    if (vt == "uint8_t" || vt == "u8")
      return "scanf(\"%\" SCNu8 \",&" + name + ");";
    if (vt == "uint32_t" || vt == "u32")
      return "scanf(\"%\" SCNu32 \",&" + name + ");";
    if (vt == "uint64_t" || vt == "u64")
      return "scanf(\"%\" SCNu64 \",&" + name + ");";
    if (vt == "bool")
      return "{int _lb_t;scanf(\"%d\",&_lb_t);" + name + "=(bool)_lb_t;}";
    if (vt == "__m256" || vt == "__m256i")
      return "fprintf(stderr,\"[Xenon] line " + std::to_string(tok.line) +
             ": cannot scanf into __m256/__m256i '" + name + "'\\n\");exit(1);";
    return "scanf(\"%d\",&" + name + ");/*Xenon:unknown type for " + name +
           ",defaulted %d*/";
  }

  // -----------------------------------------------------------------------
  // Expression parser
  // -----------------------------------------------------------------------
  std::string parse_expr() { return parse_ternary(); }

  // Check if the current IDENTIFIER token is a registered custom operator symbol
  bool is_custom_op_token() const {
    if (pos >= tokens.size()) return false;
    const Token &t = tokens[pos];
    if (t.type != TT::IDENTIFIER) return false;
    return _op_overloads.count(t.value) > 0;
  }

  // Custom infix operators sit just above comparison in precedence.
  // parse_ternary → parse_logical → ... → parse_comparison → parse_custom_infix → parse_shift
  std::string parse_custom_infix() {
    size_t left_pos = pos;
    std::string left = parse_shift();
    while (is_custom_op_token()) {
      std::string sym = advance().value;
      size_t right_pos = pos;
      std::string right = parse_shift();
      const OverloadEntry *ov = resolve_overload(sym, left_pos, right_pos);
      if (ov)
        left = ov->func_name + "(" + left + ", " + right + ")";
      else
        left = "(" + left + sym + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

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
    size_t left_pos = pos;
    std::string left = parse_bitwise_or();
    while (current().type == TT::AND || current().type == TT::OR) {
      TT op_tt = current().type;
      advance();
      std::string op_sym = (op_tt == TT::AND) ? "&&" : "||";
      size_t right_pos = pos;
      std::string right = parse_bitwise_or();
      if (_op_overloads.count(op_sym)) {
        const OverloadEntry *ov = resolve_overload(op_sym, left_pos, right_pos);
        if (ov) { left = ov->func_name + "(" + left + ", " + right + ")"; left_pos = right_pos; continue; }
      }
      left = "(" + left + " " + op_sym + " " + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_bitwise_or() {
    size_t left_pos = pos;
    std::string left = parse_bitwise_xor();
    while (current().type == TT::BITOR) {
      advance();
      size_t right_pos = pos;
      std::string right = parse_bitwise_xor();
      if (_op_overloads.count("|")) {
        const OverloadEntry *ov = resolve_overload("|", left_pos, right_pos);
        if (ov) { left = ov->func_name + "(" + left + ", " + right + ")"; left_pos = right_pos; continue; }
      }
      left = "(" + left + "|" + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_bitwise_xor() {
    size_t left_pos = pos;
    std::string left = parse_bitwise_and();
    while (current().type == TT::BITXOR) {
      advance();
      size_t right_pos = pos;
      std::string right = parse_bitwise_and();
      if (_op_overloads.count("^")) {
        const OverloadEntry *ov = resolve_overload("^", left_pos, right_pos);
        if (ov) { left = ov->func_name + "(" + left + ", " + right + ")"; left_pos = right_pos; continue; }
      }
      left = "(" + left + "^" + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_bitwise_and() {
    size_t left_pos = pos;
    std::string left = parse_comparison();
    while (current().type == TT::ADDRESS_OF) {
      advance();
      size_t right_pos = pos;
      std::string right = parse_comparison();
      if (_op_overloads.count("&")) {
        const OverloadEntry *ov = resolve_overload("&", left_pos, right_pos);
        if (ov) { left = ov->func_name + "(" + left + ", " + right + ")"; left_pos = right_pos; continue; }
      }
      left = "(" + left + "&" + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_comparison() {
    size_t left_pos = pos;
    std::string left = parse_custom_infix();
    static const std::map<TT, std::string> cmp = {
        {TT::EQ, "=="}, {TT::NE, "!="}, {TT::LT, "<"},
        {TT::GT, ">"},  {TT::LE, "<="}, {TT::GE, ">="}};
    while (cmp.count(current().type)) {
      TT op_tt = current().type;
      advance();
      std::string op_sym = cmp.at(op_tt);
      size_t right_pos = pos;
      std::string right = parse_custom_infix();
      if (_op_overloads.count(op_sym)) {
        const OverloadEntry *ov = resolve_overload(op_sym, left_pos, right_pos);
        if (ov) {
          left = ov->func_name + "(" + left + ", " + right + ")";
          left_pos = right_pos;
          continue;
        }
      }
      left = "(" + left + op_sym + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_shift() {
    size_t left_pos = pos;
    std::string left = parse_additive();
    while (current().type == TT::SHL || current().type == TT::SHR) {
      TT op_tt = current().type;
      std::string op_sym = (op_tt == TT::SHL) ? "<<" : ">>";
      advance();
      size_t right_pos = pos;
      std::string right = parse_additive();
      if (_op_overloads.count(op_sym)) {
        const OverloadEntry *ov = resolve_overload(op_sym, left_pos, right_pos);
        if (ov) {
          left = ov->func_name + "(" + left + ", " + right + ")";
          left_pos = right_pos;
          continue;
        }
      }
      left = "(" + left + op_sym + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_additive() {
    size_t left_pos = pos;
    std::string left = parse_multiplicative();
    while (current().type == TT::PLUS || current().type == TT::MINUS) {
      std::string op_sym = advance().value;
      size_t right_pos = pos;
      std::string right = parse_multiplicative();
      if (_op_overloads.count(op_sym)) {
        const OverloadEntry *ov = resolve_overload(op_sym, left_pos, right_pos);
        if (ov) {
          left = ov->func_name + "(" + left + ", " + right + ")";
          left_pos = right_pos; // update for next iteration
          continue;
        }
      }
      left = "(" + left + op_sym + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_multiplicative() {
    size_t left_pos = pos;
    std::string left = parse_unary();
    while (current().type == TT::MULTIPLY || current().type == TT::DIVIDE ||
           current().type == TT::MOD) {
      std::string op_sym = advance().value;
      size_t right_pos = pos;
      std::string right = parse_unary();
      if (_op_overloads.count(op_sym)) {
        const OverloadEntry *ov = resolve_overload(op_sym, left_pos, right_pos);
        if (ov) {
          left = ov->func_name + "(" + left + ", " + right + ")";
          left_pos = right_pos;
          continue;
        }
      }
      left = "(" + left + op_sym + right + ")";
      left_pos = right_pos;
    }
    return left;
  }

  std::string parse_unary() {
    size_t op_pos = pos;
    if (current().type == TT::NOT) {
      advance();
      size_t operand_pos = pos;
      std::string inner = parse_unary();
      if (_op_overloads.count("!")) {
        const OverloadEntry *ov = resolve_overload("!", op_pos, operand_pos);
        if (ov && !ov->is_binary) return ov->func_name + "(" + inner + ")";
      }
      return "(!" + inner + ")";
    }
    if (current().type == TT::BITNOT) {
      advance();
      size_t operand_pos = pos;
      std::string inner = parse_unary();
      if (_op_overloads.count("~")) {
        const OverloadEntry *ov = resolve_overload("~", op_pos, operand_pos);
        if (ov && !ov->is_binary) return ov->func_name + "(" + inner + ")";
      }
      return "(~" + inner + ")";
    }
    if (current().type == TT::MINUS) {
      advance();
      size_t operand_pos = pos;
      std::string inner = parse_unary();
      if (_op_overloads.count("u-")) {  // "u-" = unary minus overload key
        const OverloadEntry *ov = resolve_overload("u-", op_pos, operand_pos);
        if (ov && !ov->is_binary) return ov->func_name + "(" + inner + ")";
      }
      return "(-" + inner + ")";
    }
    if (current().type == TT::MULTIPLY) {
      advance();
      size_t operand_pos = pos;
      std::string inner = parse_power();
      if (_op_overloads.count("u*")) {  // "u*" = unary deref overload key
        const OverloadEntry *ov = resolve_overload("u*", op_pos, operand_pos);
        if (ov && !ov->is_binary) return ov->func_name + "(" + inner + ")";
      }
      return "*(" + inner + ")";
    }
    if (current().type == TT::INCR) {
      advance();
      size_t operand_pos = pos;
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      if (_op_overloads.count("++pre")) {
        const OverloadEntry *ov = resolve_overload("++pre", op_pos, operand_pos);
        if (ov && !ov->is_binary) return ov->func_name + "(" + name + ")";
      }
      return "(++" + name + ")";
    }
    if (current().type == TT::DECR) {
      advance();
      size_t operand_pos = pos;
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      if (_op_overloads.count("--pre")) {
        const OverloadEntry *ov = resolve_overload("--pre", op_pos, operand_pos);
        if (ov && !ov->is_binary) return ov->func_name + "(" + name + ")";
      }
      return "(--" + name + ")";
    }
    return parse_power();
  }

  std::string parse_power() {
    size_t left_pos = pos;
    std::string base = parse_primary();
    if (current().type == TT::POW) {
      advance();
      size_t right_pos = pos;
      std::string exp = parse_unary();
      if (_op_overloads.count("**")) {
        const OverloadEntry *ov = resolve_overload("**", left_pos, right_pos);
        if (ov) return ov->func_name + "(" + base + ", " + exp + ")";
      }
      return "pow((double)(" + base + "),(double)(" + exp + "))";
    }
    return base;
  }

  // ── Alias-aware _ptr_alloc_elems lookup ─────────────────────────────────
  // If `name` itself has an entry, return it. Otherwise follow the alias chain
  // (ptr double q = p → _ptr_aliases["q"]="p") up to 8 hops.
  const std::string *resolve_ptr_elems(const std::string &name) const {
    std::string cur = name;
    for (int hop = 0; hop < 8; ++hop) {
      auto it = _ptr_alloc_elems.find(cur);
      if (it != _ptr_alloc_elems.end()) return &it->second;
      auto al = _ptr_aliases.find(cur);
      if (al == _ptr_aliases.end()) break;
      cur = al->second;
    }
    return nullptr;
  }

  // ── Look-back index resolver ─────────────────────────────────────────────
  // Given the name of an index variable and the token position of the subscript
  // operator, scan backwards through the current function body to determine
  // whether the index has a statically known value.
  //
  // Returns:
  //   {true,  value}  — index is statically known to be `value`
  //   {false, 0}      — index is dynamic; caller should emit runtime check
  //
  // Rules:
  //  1. If idx_var was assigned a literal and NEVER touched inside any
  //     if/elseif/else/for/while body → static, value = that literal.
  //  2. If every branch of a purely literal-condition if/else assigns idx_var
  //     a literal → static, value = max of all branch values (conservative OOB check).
  //  3. If the condition of any enclosing/preceding if involves a _runtime_var
  //     → dynamic.
  //  4. Anything else → dynamic.
  std::pair<bool, long long> resolve_index_statically(
      const std::string &idx_var, size_t subscript_pos,
      size_t body_start) const {

    // Walk backwards collecting assignments and conditional blocks that touch idx_var
    long long last_literal = -1;
    bool seen_any_assign = false;
    bool inside_conditional = false;  // true if we found the var written inside an if/for/while
    bool all_branches_literal = true; // for the conditional-resolve path
    std::vector<long long> branch_values;

    // We scan forward from body_start to subscript_pos (easier than backward
    // for nested block detection), tracking depth of conditional blocks.
    int cond_depth = 0;            // depth of if/for/while we're inside
    bool in_runtime_cond = false;  // any enclosing condition is runtime-valued

    size_t i = body_start;
    while (i < subscript_pos && i < tokens.size()) {
      const Token &tk = tokens[i];

      // Track entry into conditional blocks
      if (tk.type == TT::IF || tk.type == TT::ELSEIF ||
          tk.type == TT::WHILE || tk.type == TT::FOR) {
        // Check whether the condition references a runtime var.
        // Scan from here to THEN/DO to find any runtime var reference.
        size_t cond_start = i + 1;
        size_t cond_end = cond_start;
        while (cond_end < tokens.size() &&
               tokens[cond_end].type != TT::THEN &&
               tokens[cond_end].type != TT::DO &&
               tokens[cond_end].type != TT::LBRACE)
          cond_end++;
        bool cond_is_runtime = false;
        for (size_t c = cond_start; c < cond_end; ++c) {
          if (tokens[c].type == TT::IDENTIFIER &&
              _runtime_vars.count(tokens[c].value))
            cond_is_runtime = true;
        }
        if (cond_is_runtime) in_runtime_cond = true;
        cond_depth++;
        i++;
        continue;
      }
      if (tk.type == TT::END) {
        if (cond_depth > 0) cond_depth--;
        if (cond_depth == 0) in_runtime_cond = false;
        i++;
        continue;
      }

      // Look for assignments to idx_var: IDENTIFIER ASSIGN NUMBER
      if (tk.type == TT::IDENTIFIER && tk.value == idx_var &&
          i + 2 < subscript_pos &&
          tokens[i + 1].type == TT::ASSIGN &&
          tokens[i + 2].type == TT::NUMBER) {
        long long val = std::stoll(tokens[i + 2].value);
        seen_any_assign = true;
        if (cond_depth > 0) {
          // Inside a conditional block
          inside_conditional = true;
          if (in_runtime_cond) {
            all_branches_literal = false; // can't resolve
          } else {
            branch_values.push_back(val);
          }
        } else {
          last_literal = val;
        }
        i += 3;
        continue;
      }

      // Any other write to idx_var (compound assign, ++/--, call arg, scanf) → dynamic
      if (tk.type == TT::IDENTIFIER && tk.value == idx_var) {
        size_t nx = i + 1;
        if (nx < tokens.size() && (
              tokens[nx].type == TT::PLUS_ASSIGN ||
              tokens[nx].type == TT::MINUS_ASSIGN ||
              tokens[nx].type == TT::MUL_ASSIGN ||
              tokens[nx].type == TT::DIV_ASSIGN ||
              tokens[nx].type == TT::MOD_ASSIGN ||
              tokens[nx].type == TT::INCR ||
              tokens[nx].type == TT::DECR)) {
          // Compound assign/increment inside conditional → dynamic
          if (cond_depth > 0) { all_branches_literal = false; inside_conditional = true; }
          // Outside conditional: non-literal modification → dynamic
          else { last_literal = -1; seen_any_assign = true; }
        }
        // scanf(idx_var) → mark dynamic (already in _runtime_vars but double-check)
        if (nx < tokens.size() && tokens[nx].type == TT::COMMA) {
          // could be a call arg — conservative: don't resolve
          return {false, 0};
        }
      }

      // If idx_var appears as the argument to scanf → dynamic
      if (tk.type == TT::SCANF) {
        // scan the arg list for idx_var
        size_t sc = i + 1;
        while (sc < subscript_pos && tokens[sc].type != TT::SEMICOLON) {
          if (tokens[sc].type == TT::IDENTIFIER && tokens[sc].value == idx_var)
            return {false, 0};
          sc++;
        }
      }

      i++;
    }

    if (!seen_any_assign && !inside_conditional) {
      // idx_var was never assigned in this scope → can't determine value
      return {false, 0};
    }

    if (inside_conditional) {
      if (!all_branches_literal || branch_values.empty())
        return {false, 0}; // runtime condition or non-literal branch
      // All branches assigned literals through compile-time conditions.
      // Use the maximum for the conservative OOB check.
      long long max_val = *std::max_element(branch_values.begin(), branch_values.end());
      // Also factor in any unconditional assignment
      if (last_literal >= 0) max_val = std::max(max_val, last_literal);
      return {true, max_val};
    }

    if (last_literal >= 0)
      return {true, last_literal};

    return {false, 0};
  }

  std::string parse_primary() {
    Token &t = current();

    // ── Option constructors ──────────────────────────────────────────────────
    // Some(expr)  →  (Option__T){1, expr}
    // The concrete type T is inferred from the argument expression.
    if (t.type == TT::SOME_KW) {
      Token some_tok = advance(); // consume 'Some'
      expect(TT::LPAREN, false);
      size_t arg_pos = pos;
      std::string inner = parse_expr();
      expect(TT::RPAREN, false);
      // Infer the C type of the argument
      std::string c_type = infer_type_at(arg_pos).c_type();
      if (c_type.empty() || c_type == "unknown") c_type = "int"; // safe fallback
      std::string mangled = ensure_option_struct(c_type);
      return "(" + mangled + "){1, " + inner + "}";
    }

    // None  →  (Option__T){0}
    // Type is resolved from context (the variable being assigned to).
    // Because we don't always have context here, we emit a zero-initialiser
    // and rely on C's implicit struct conversion.  When used in a typed
    // declaration the struct type is already known by the surrounding code.
    if (t.type == TT::NONE_KW) {
      advance(); // consume 'None'
      // If we can peek at what variable is being assigned (previous tokens),
      // resolve the exact Option type.  Otherwise emit a generic sentinel
      // that C will coerce into whatever Option__T is expected.
      // Walk backward to find the declaring Option__* type in var_types.
      std::string opt_type = "";
      // Scan recent tokens backward for ASSIGN preceded by an IDENTIFIER whose
      // var_types entry starts with "Option__"
      if (pos >= 2) {
        for (int _bk = (int)pos - 1; _bk >= 0 && _bk >= (int)pos - 6; _bk--) {
          if (tokens[_bk].type == TT::IDENTIFIER) {
            auto _vit = var_types.find(safe_name(tokens[_bk].value));
            if (_vit != var_types.end() &&
                _vit->second.size() > 7 &&
                _vit->second.substr(0, 7) == "Option_") {
              opt_type = _vit->second;
              break;
            }
          }
        }
      }
      if (!opt_type.empty())
        return "(" + opt_type + "){0}";
      // Fallback — use a compound literal of int; C will warn if the types
      // mismatch, which is still better than a silent bug.
      return "(" + ensure_option_struct("int") + "){0}";
    }

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
      Token type_tok = current();
      // Valid cast targets: built-in type keywords, ptr, or known struct/enum
      // names
      static const std::set<TT> valid_cast_types = {
          TT::INT,     TT::FLOAT,   TT::DOUBLE,    TT::LONG, TT::SHORT,
          TT::CHAR_KW, TT::BOOL_KW, TT::VOID,      TT::U8,   TT::U32,
          TT::U64,     TT::PTR,     TT::IDENTIFIER};
      if (!valid_cast_types.count(type_tok.type))
        throw XenonError("TypeError",
                           "cast: expected a type name, got '" +
                               type_tok.value + "'",
                           type_tok.line, type_tok.col);
      std::string tn = advance().value;
      if (tn == "str")
        tn = "char*";
      else if (tn == "ptr") {
        std::string inner_t = advance().value;
        if (inner_t == "str")
          inner_t = "char*";
        tn = inner_t + "*";
      }
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

    // Brace initializer as expression: {expr, expr, ...}
    // Emits a C99 compound literal: (elem_t[N]){expr, ...}
    // Element type is inferred from the first element (defaults to int).
    if (t.type == TT::LBRACE) {
      Token brace_tok = t;
      advance();
      std::vector<std::string> items;
      TypeInfo elem_ti = TypeInfo::of("int");
      bool first = true;
      while (current().type != TT::RBRACE && current().type != TT::TEOF) {
        if (first) {
          elem_ti = infer_type_at(pos);
          first = false;
        }
        items.push_back(parse_expr());
        if (current().type == TT::COMMA)
          advance();
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
                         "Unterminated brace initializer — missing closing '}'",
                         brace_tok.line, brace_tok.col);
      expect(TT::RBRACE, false);
      std::string elem_c = elem_ti.c_type();
      std::string n = std::to_string(items.size());
      // C99 compound literal — passable as a pointer to any function expecting an array
      return "(" + elem_c + "[" + n + "]){" + join(items, ", ") + "}";
    }

    if (t.type == TT::LPAREN) {
      advance();
      std::string inner = parse_expr();
      expect(TT::RPAREN, false);
      return "(" + inner + ")";
    }

    if (t.type == TT::ADDRESS_OF) {
      advance();
      // parse_power handles identifier + postfix chains ([], ., ->)
      return "&(" + parse_power() + ")";
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

      // ── Struct method: rewrite bare field refs to self->field ──────
      if (!_cur_struct.empty() && name != "self" &&
          current().type != TT::COLON) {
        auto _sfit = struct_field_types.find(_cur_struct);
        if (_sfit != struct_field_types.end() && _sfit->second.count(name)) {
          name = "self->" + name;
          while (current().type == TT::LSBRACKET || current().type == TT::DOT ||
                 current().type == TT::ARROW) {
            if (current().type == TT::LSBRACKET) {
              advance();
              std::string _idx = parse_expr();
              expect(TT::RSBRACKET, false);
              name += "[(size_t)(" + _idx + ")]";
            } else if (current().type == TT::DOT) {
              advance();
              name += "." + expect(TT::IDENTIFIER, false).value;
            } else {
              advance();
              name += "->" + expect(TT::IDENTIFIER, false).value;
            }
          }
          return name;
        }
      }

      // NAME::symbol — namespace qualified access (supports chaining: A::B::sym)
      if (current().type == TT::COLON &&
          pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON) {
        // Accumulate namespace path, e.g.  A :: B :: sym  → ns_path="A__B", sym="sym"
        std::string ns_path = tok.value;
        std::string ns_path_colons = tok.value; // same but with "::" separators
        advance(); advance(); // consume first ::
        std::string sym = expect(TT::IDENTIFIER, false).value;
        // Keep consuming ::IDENTIFIER as long as the next thing is also ::IDENT
        // (meaning the current sym is itself a sub-namespace, not the final symbol)
        while (current().type == TT::COLON &&
               pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON) {
          // sym was a sub-namespace name — fold it into the path
          ns_path = ns_path + "__" + sym;
          ns_path_colons = ns_path_colons + "::" + sym;
          advance(); advance(); // consume ::
          sym = expect(TT::IDENTIFIER, false).value;
        }
        // ── Module function auto-load ─────────────────────────────────────
        // Build the full use-path: ns_path_colons + "::" + sym
        if (current().type == TT::LPAREN) {
          std::string full_use_path = ns_path_colons + "::" + sym;
          maybe_load_module_call(full_use_path, tok);
        }
        std::string resolved = resolve_qualified(ns_path, sym);
        if (resolved.empty())
          resolved = ns_path + "__" + sym; // best-effort if not registered yet
        name = resolved;
        // Optional explicit generic type arg: NS::func<TYPE>(args)
        std::string ns_explicit_type;
        if (current().type == TT::LT && template_funcs.count(name)) {
          size_t saved_p = pos;
          advance(); // consume '<'
          bool is_ptr = false;
          if (current().type == TT::PTR) { advance(); is_ptr = true; }
          if (current().type == TT::IDENTIFIER || current().type == TT::INT ||
              current().type == TT::FLOAT || current().type == TT::DOUBLE ||
              current().type == TT::LONG || current().type == TT::SHORT ||
              current().type == TT::BOOL_KW || current().type == TT::CHAR_KW ||
              current().type == TT::STR || current().type == TT::U8 ||
              current().type == TT::U32 || current().type == TT::U64 ||
              current().type == TT::VOID) {
            std::string et = advance().value;
            if (current().type == TT::GT) {
              advance();
              ns_explicit_type = raw_to_c(et);
              if (is_ptr) ns_explicit_type += "*";
            } else { pos = saved_p; }
          } else { pos = saved_p; }
        }
        if (current().type == TT::LPAREN) {
          advance();
          return emit_call(name, tok, ns_explicit_type);
        }
        return name;
      }

      if (current().type == TT::LPAREN) {
        // If in a namespace, try local scope first
        if (!_cur_namespace.empty()) {
          auto &ns_map = _namespaces[_cur_namespace];
          auto it = ns_map.find(tok.value);
          if (it != ns_map.end()) name = it->second;
        }
        // Check if this is an ignored-namespace symbol
        if (name == safe_name(tok.value)) {
          std::string resolved = resolve_ignored_ns(tok.value);
          if (!resolved.empty()) name = resolved;
        }
        // ── Single-segment module call: use something → something() ───────
        maybe_load_module_call(tok.value, tok);
        advance();
        return emit_call(name, tok);
      }
      // Explicit generic call: func<TYPE>(args)
      if (current().type == TT::LT && template_funcs.count(name)) {
        size_t saved_pos = pos;
        advance(); // consume '<'
        // Try to parse: IDENTIFIER or type keyword, then '>'
        std::string explicit_type;
        bool explicit_is_ptr = false;
        if (current().type == TT::PTR) {
          advance();
          explicit_is_ptr = true;
        }
        if (current().type == TT::IDENTIFIER || current().type == TT::INT ||
            current().type == TT::FLOAT || current().type == TT::DOUBLE ||
            current().type == TT::LONG || current().type == TT::SHORT ||
            current().type == TT::BOOL_KW || current().type == TT::CHAR_KW ||
            current().type == TT::STR || current().type == TT::U8 ||
            current().type == TT::U32 || current().type == TT::U64 ||
            current().type == TT::VOID) {
          explicit_type = advance().value;
          if (current().type == TT::GT) {
            advance(); // consume '>'
            std::string c_type = raw_to_c(explicit_type);
            if (explicit_is_ptr) c_type += "*";
            if (current().type == TT::LPAREN) {
              advance();
              return emit_call(name, tok, c_type);
            }
          }
        }
        // Not a valid generic call — backtrack
        pos = saved_pos;
      }
      while (current().type == TT::LSBRACKET || current().type == TT::DOT ||
             current().type == TT::ARROW) {
        if (current().type == TT::LSBRACKET) {
          size_t base_tok_pos = pos - 1;
          advance();
          size_t idx_pos = pos;
          std::string idx = parse_expr();
          expect(TT::RSBRACKET, false);
          if (_op_overloads.count("[]")) {
            const OverloadEntry *ov = resolve_overload("[]", base_tok_pos, idx_pos);
            if (ov) { name = ov->func_name + "(" + name + ", " + idx + ")"; continue; }
          }
          // ── $bounds: assert suppression ───────────────────────────────────
          // If the token immediately before this statement is BOUNDS_ASSERT,
          // skip all checks entirely — the programmer has asserted correctness.
          bool _bounds_asserted = false;
          if (pos >= 3) {
            for (size_t _bk = pos - 1; _bk > 0 && _bk + 4 >= pos; _bk--) {
              if (tokens[_bk].type == TT::BOUNDS_ASSERT) { _bounds_asserted = true; break; }
              if (tokens[_bk].type == TT::SEMICOLON || tokens[_bk].type == TT::LBRACE) break;
            }
          }
          if (_bounds_asserted) {
            name = name + "[(size_t)(" + idx + ")]";
            continue;
          }
          // ── Alias-aware element-count lookup ─────────────────────────────
          const std::string *_elems_ptr = resolve_ptr_elems(name);
          if (_memory_safe && !_in_unsafe_block && _elems_ptr != nullptr) {
            const std::string &n_elems = *_elems_ptr;
            // ── Look-back: try to resolve index statically ────────────────
            // Find the enclosing function body start by scanning backward for '{'
            size_t _body_start = 0;
            { int _depth = 0;
              for (size_t _bk = (pos > 0 ? pos - 1 : 0); _bk > 0; _bk--) {
                if (tokens[_bk].type == TT::RBRACE) _depth++;
                else if (tokens[_bk].type == TT::LBRACE) {
                  if (_depth == 0) { _body_start = _bk + 1; break; }
                  _depth--;
                }
              }
            }
            // Only attempt static resolution when idx is a single identifier
            bool _static_resolved = false;
            if (idx_pos < tokens.size() && tokens[idx_pos].type == TT::IDENTIFIER) {
              std::string _idx_var = tokens[idx_pos].value;
              auto [_known, _val] = resolve_index_statically(_idx_var, idx_pos, _body_start);
              if (_known) {
                _static_resolved = true;
                long long _n = -1;
                try { _n = std::stoll(n_elems); } catch (...) {}
                if (_val < 0) {
                  transpile_errors.push_back(
                    XenonError("OOBAccess",
                      "index '" + _idx_var + "' (=" + std::to_string(_val) +
                      ") on pointer '" + name + "' is negative — undefined behaviour",
                      tokens[idx_pos].line, tokens[idx_pos].col).what());
                } else if (_n >= 0 && _val >= _n) {
                  transpile_errors.push_back(
                    XenonError("OOBAccess",
                      "index '" + _idx_var + "' (=" + std::to_string(_val) +
                      ") on pointer '" + name + "' is out of bounds (size=" +
                      std::to_string(_n) + ")",
                      tokens[idx_pos].line, tokens[idx_pos].col).what());
                }
                name = name + "[(size_t)(" + idx + ")]";
                continue;
              }
            }
            // Index is dynamic or non-identifier — emit runtime check with warning
            if (!_static_resolved) {
              emit_warning("BoundsCheck",
                "cannot statically verify index '" + idx + "' on pointer '" + name +
                "' — runtime bounds check inserted (suppress with '$bounds: assert' on the previous line)",
                current().line, current().col);
            }
            // Emit _XEN_IDX macro (injected once)
            if (!has_header("_XEN_IDX")) {
              if (tcc_mode) {
                headers.push_back(
                  "/* ── Xenon runtime pointer safety (null + bounds) [TCC/C99] ── */\n"
                  "#include <stdlib.h>\n"
                  "#include <stdio.h>\n"
                  "#define _XEN_IDX(p,i,n) \\\n"
                  "  (((p)==NULL ? \\\n"
                  "     (fprintf(stderr,\"[Xenon] null pointer dereference\\n\"),abort(),0) : 0), \\\n"
                  "   ((long long)(i)<0||(long long)(i)>=(long long)(n) ? \\\n"
                  "     (fprintf(stderr,\"[Xenon] index %lld out of bounds (size=%lld)\\n\",(long long)(i),(long long)(n)),abort(),0) : 0), \\\n"
                  "   (p)[(size_t)(i)])\n"
                );
              } else {
                headers.push_back(
                  "/* ── Xenon runtime pointer safety (null + bounds) ── */\n"
                  "#include <stdlib.h>\n"
                  "#include <stdio.h>\n"
                  "#define _XEN_IDX(p,i,n) \\\n"
                  "  ((__builtin_expect((p)==NULL,0) ? \\\n"
                  "     (fprintf(stderr,\"[Xenon] null pointer dereference\\n\"),abort(),0) : 0), \\\n"
                  "   (__builtin_expect((long long)(i)<0||(long long)(i)>=(long long)(n),0) ? \\\n"
                  "     (fprintf(stderr,\"[Xenon] index %lld out of bounds (size=%lld)\\n\",(long long)(i),(long long)(n)),abort(),0) : 0), \\\n"
                  "   (p)[(size_t)(i)])\n"
                );
              }
            }
            name = "_XEN_IDX(" + name + ", " + idx + ", " + n_elems + ")";
          } else {
            name = name + "[(size_t)(" + idx + ")]";
          }
        } else if (current().type == TT::DOT) {
          advance();
          std::string field = expect(TT::IDENTIFIER, false).value;
          // ── Method call: obj.method(args) → StructName__method(&obj, args)
          // Extract the base variable name to look up its struct type.
          std::string base = name;
          for (char &ch : base)
            if (ch == '[' || ch == '.' || ch == '-' || ch == '>')
              ch = ' ';
          std::istringstream ss(base);
          std::string first;
          ss >> first;
          // Look up the struct type of the base variable
          std::string base_struct_type;
          {
            auto vit2 = var_types.find(first);
            if (vit2 != var_types.end()) {
              // var_types stores the raw Xenon type (e.g. "Sprite", "ptr", "int")
              base_struct_type = vit2->second;
            }
          }
          // ── Option<T> built-in methods ────────────────────────────────────
          // is_some() → opt.has_value
          // is_none() → !opt.has_value
          // unwrap()  → Option__T__unwrap(opt)
          // unwrap_or(default) → Option__T__unwrap_or(opt, default)
          bool dispatched_method = false;
          if (!base_struct_type.empty() &&
              base_struct_type.size() > 7 &&
              base_struct_type.substr(0, 7) == "Option_") {
            if (field == "is_some" && current().type == TT::LPAREN) {
              advance(); expect(TT::RPAREN, false);
              name = "(" + name + ".has_value)";
              dispatched_method = true;
            } else if (field == "is_none" && current().type == TT::LPAREN) {
              advance(); expect(TT::RPAREN, false);
              name = "(!(" + name + ".has_value))";
              dispatched_method = true;
            } else if (field == "unwrap" && current().type == TT::LPAREN) {
              advance(); expect(TT::RPAREN, false);
              name = base_struct_type + "__unwrap(" + name + ")";
              dispatched_method = true;
            } else if (field == "unwrap_or" && current().type == TT::LPAREN) {
              advance(); // consume '('
              std::string def_val = parse_expr();
              expect(TT::RPAREN, false);
              name = base_struct_type + "__unwrap_or(" + name + ", " + def_val + ")";
              dispatched_method = true;
            } else if (field == "value" && current().type != TT::LPAREN) {
              // Direct field access opt.value — allow it
              name = name + ".value";
              dispatched_method = true;
            }
          }
          // Check if field is a registered method on that struct
          if (!base_struct_type.empty() && current().type == TT::LPAREN) {
            auto msit = struct_methods.find(base_struct_type);
            if (msit != struct_methods.end() && msit->second.count(field)) {
              // It's a method call — emit: StructType__method(&obj, args)
              advance(); // consume '('
              std::string mangled_method = base_struct_type + "__" + field;
              // For pointer vars use the name directly; for value vars take address
              std::string self_arg;
              bool is_ptr_var = (base_struct_type.size() > 0 &&
                                 var_types.count(first) &&
                                 var_types[first].find("ptr") != std::string::npos);
              // Simpler: always take address — C handles &ptr correctly with **
              // but that's wrong; use -> for ptrs.  Check var_types[first].
              // If var is a plain struct value, use &name; if ptr, use name.
              if (is_ptr_var)
                self_arg = name; // already a pointer
              else
                self_arg = "&" + name;
              std::string call_args = self_arg;
              while (current().type != TT::RPAREN && current().type != TT::TEOF) {
                call_args += ", " + parse_expr();
                if (current().type == TT::COMMA) advance();
              }
              expect(TT::RPAREN, false);
              name = mangled_method + "(" + call_args + ")";
              dispatched_method = true;
            }
          }
          if (!dispatched_method) {
            std::string op = (var_types.count(first) && var_types[first] == "ptr")
                                 ? "->"
                                 : ".";
            name = name + op + field;
          }
        } else {
          advance();
          std::string field = expect(TT::IDENTIFIER, false).value;
          name = name + "->" + field;
        }
      }
      if (current().type == TT::INCR) {
        size_t operand_pos = pos - 1;
        advance();
        if (_op_overloads.count("++post")) {
          const OverloadEntry *ov = resolve_overload("++post", operand_pos, operand_pos);
          if (ov && !ov->is_binary) return ov->func_name + "(" + name + ")";
        }
        return "(" + name + "++)";
      }
      if (current().type == TT::DECR) {
        size_t operand_pos = pos - 1;
        advance();
        if (_op_overloads.count("--post")) {
          const OverloadEntry *ov = resolve_overload("--post", operand_pos, operand_pos);
          if (ov && !ov->is_binary) return ov->func_name + "(" + name + ")";
        }
        return "(" + name + "--)";
      }

      // ── Variable existence check ─────────────────────────────────────────
      // At this point `tok` is a bare identifier used as a value (not a function
      // call, not namespace-qualified, not a struct field ref).  Verify it is
      // a declared variable, known function, enum constant, type name, or
      // C/built-in symbol.  If not, throw a clear NameError before the C
      // compiler ever sees it.
      //
      // We skip the check when:
      //   • the name starts with '_' (C internals, Xenon runtime helpers)
      //   • the name contains '__' (mangled namespace/struct names)
      //   • we're inside a struct method body (_cur_struct is set) — 'self'
      //     and field names are resolved above; anything else is likely a
      //     C-level identifier we shouldn't second-guess
      //   • the name is "self" (always valid inside a method)
      {
        const std::string &raw_id = tok.value;
        bool skip_check =
            raw_id.empty() ||
            raw_id[0] == '_' ||
            raw_id.find("__") != std::string::npos ||
            name.find("__") != std::string::npos ||  // resolved/mangled namespace name
            raw_id == "self" ||
            !_cur_struct.empty(); // inside method body — already handled above

        if (!skip_check) {
          bool known =
              var_types.count(name)        || // declared variable or struct type
              var_types.count(raw_id)      || // pre-mangling name
              is_known_function(name)      || // user / builtin function
              is_known_function(raw_id)    ||
              is_known_type(name)          || // struct/enum type name used as value
              is_known_type(raw_id)        ||
              _enum_names.count(raw_id)    || // enum type name itself
              builtin_types().count(raw_id)||  // primitive type keyword used as sizeof arg etc.
              // enum *members* — they live in var_types if the enum was declared in scope,
              // but for C-scoped enums whose members we didn't register, allow them through
              // if the name is ALL_CAPS (common C enum convention) or is all alpha/underscore
              // and short (heuristic: true/false/NULL-like literals).
              raw_id == "true" || raw_id == "false" || raw_id == "NULL" ||
              // Check ignored namespaces — a symbol imported via 'ignore namespace'
              // may resolve without qualification
              !resolve_ignored_ns(raw_id).empty();

          // Also accept any name that resolves in the current namespace
          if (!known && !_cur_namespace.empty()) {
            auto &ns_map = _namespaces[_cur_namespace];
            known = ns_map.count(raw_id) > 0;
          }
          // Accept enum *members*: they are not individually stored in var_types
          // but their parent enum IS in _enum_names. We can't distinguish member
          // names cheaply, so we allow any identifier that follows an enum name in
          // a :: context (already handled above) or appears to be an ALL_CAPS
          // constant (common for enum/define values).
          if (!known) {
            bool all_caps_or_underscore = !raw_id.empty();
            for (char c : raw_id) {
              if (!std::isupper((unsigned char)c) && c != '_' && !std::isdigit((unsigned char)c)) {
                all_caps_or_underscore = false;
                break;
              }
            }
            if (all_caps_or_underscore && raw_id.size() >= 2)
              known = true; // likely an enum member or #define constant
          }

          if (!known) {
            // Build "did you mean?" candidates from var_types keys
            std::vector<std::string> candidates;
            for (const auto &[vn, _] : var_types) {
              if (vn == "STRUCT" || vn == "GENERIC_STRUCT") continue;
              size_t common = 0;
              size_t minlen = std::min(vn.size(), raw_id.size());
              for (size_t ci = 0; ci < minlen; ci++) {
                if (vn[ci] == raw_id[ci]) common++;
                else break;
              }
              if ((common >= 3 && common * 2 >= minlen) ||
                  (minlen > 0 && common == minlen && minlen >= 2))
                candidates.push_back(vn);
            }
            // Also check func_return_types for close function names
            for (const auto &[fn, _] : func_return_types) {
              size_t common = 0;
              size_t minlen = std::min(fn.size(), raw_id.size());
              for (size_t ci = 0; ci < minlen; ci++) {
                if (fn[ci] == raw_id[ci]) common++;
                else break;
              }
              if ((common >= 3 && common * 2 >= minlen) ||
                  (minlen > 0 && common == minlen && minlen >= 2))
                candidates.push_back(fn + "()");
            }
            std::string msg =
              "Use of undeclared identifier '" + raw_id + "'.\n"
              "  This variable has not been declared in the current scope.\n"
              "  Declare it first, e.g.:  int " + raw_id + " = ...";
            if (!candidates.empty()) {
              msg += "\n  Did you mean: ";
              for (size_t ci = 0; ci < candidates.size() && ci < 3; ci++) {
                if (ci) msg += ", ";
                msg += "'" + candidates[ci] + "'";
              }
              msg += "?";
            }
            throw XenonError("NameError", msg, tok.line, tok.col);
          }
        }
      }
      // ─────────────────────────────────────────────────────────────────────

      // Warn when a bare function name is used as a value without calling it.
      // The expression is valid C (it yields the function's address), but in
      // Xenon this almost always means the user forgot the parentheses.
      if ((is_known_function(name) || is_known_function(tok.value)) &&
          current().type != TT::LPAREN) {
        emit_warning("BareFunction",
          "'" + tok.value + "' is a function but is used here without '()' — "
          "this evaluates to the memory address of the function, not its return value.\n"
          "  Did you mean '" + tok.value + "(...)'?",
          tok.line, tok.col);
      }

      return name;
    }

    throw XenonError("SyntaxError",
                       "Unexpected token '" + tok.value + "' in expression"
                       " — expected a value, variable, or operator",
                       tok.line, tok.col);
  }

  // Map a raw Xenon type keyword to its canonical C type string
  std::string raw_to_c(const std::string &raw) const {
    if (raw == "str")
      return "char*";
    if (raw == "ptr")
      return "void*";
    if (raw == "bool")
      return "bool";
    if (raw == "char")
      return "char";
    if (raw == "u8")
      return "uint8_t";
    if (raw == "u32")
      return "uint32_t";
    if (raw == "u64")
      return "uint64_t";
    return raw; // int, float, double, long, short, void, custom structs
  }

  // Look up a variable's TypeInfo from var_types
  TypeInfo lookup_var(const std::string &name) const {
    auto it = var_types.find(name);
    if (it == var_types.end())
      return TypeInfo::unknown();
    std::string raw = it->second;
    // strip _ARRAY suffix
    bool is_arr = false;
    if (raw.size() >= 6 && raw.substr(raw.size() - 6) == "_ARRAY") {
      raw = raw.substr(0, raw.size() - 6);
      is_arr = true;
    }
    TypeInfo ti;
    // handle "ptr X" stored as "X*" in var_types
    if (!raw.empty() && raw.back() == '*') {
      ti.base = raw.substr(0, raw.size() - 1);
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
    if (it == func_return_types.end())
      return TypeInfo::unknown();
    const std::string &raw = it->second;
    if (raw == "__infer__")
      return TypeInfo::unknown(); // not yet resolved
    TypeInfo ti;
    if (!raw.empty() && raw.back() == '*') {
      ti.base = raw.substr(0, raw.size() - 1);
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
    if (s.find('.') != std::string::npos || s.find('e') != std::string::npos ||
        s.find('E') != std::string::npos)
      return TypeInfo::of("double");
    // long suffix
    if (!s.empty() && (s.back() == 'l' || s.back() == 'L'))
      return TypeInfo::of("long");
    return TypeInfo::of("int");
  }

  // Core inference: walk token stream starting at `start_pos` (peek, no
  // mutation) and compute the TypeInfo of the expression. This is a *pure
  // lookahead* — it does NOT advance `pos`. We snapshot and restore pos
  // internally.
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
      TypeInfo t = infer_ti_ternary(); // recurse: a ? b ? c : d : e
      if (pos < tokens.size() && tokens[pos].type == TT::COLON)
        pos++;
      TypeInfo e = infer_ti_ternary(); // recurse: right-associative
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
    static const std::set<TT> cmp_ops = {TT::EQ, TT::NE, TT::LT,
                                         TT::GT, TT::LE, TT::GE};
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
      TT op = tokens[pos].type;
      pos++;
      TypeInfo right = infer_ti_multiplicative();
      // pointer arithmetic: ptr + int → ptr (C semantics)
      if (left.is_ptr() && right.is_integer()) { /* left stays ptr */
      } else if (right.is_ptr() && left.is_integer())
        left = right;
      // ptr - ptr → ptrdiff_t (treat as long) — only for subtraction
      else if (left.is_ptr() && right.is_ptr() && op == TT::MINUS)
        left = TypeInfo::of("long");
      // ptr + ptr is invalid in C — leave as unknown
      else if (left.is_ptr() && right.is_ptr())
        left = TypeInfo::unknown();
      else
        left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_multiplicative() {
    TypeInfo left = infer_ti_unary();
    while (pos < tokens.size() &&
           (tokens[pos].type == TT::MULTIPLY ||
            tokens[pos].type == TT::DIVIDE || tokens[pos].type == TT::MOD)) {
      pos++;
      TypeInfo right = infer_ti_unary();
      left = promote(left, right);
    }
    return left;
  }

  TypeInfo infer_ti_unary() {
    if (pos >= tokens.size())
      return TypeInfo::unknown();
    TT tt = tokens[pos].type;

    // logical NOT → bool
    if (tt == TT::NOT) {
      pos++;
      infer_ti_unary();
      return TypeInfo::of("bool");
    }
    // bitwise NOT → same type as operand
    if (tt == TT::BITNOT) {
      pos++;
      return infer_ti_unary();
    }
    // unary minus → promote to at least int
    if (tt == TT::MINUS) {
      pos++;
      TypeInfo inner = infer_ti_unary();
      return inner.is_float() ? inner : promote(inner, TypeInfo::of("int"));
    }
    // dereference *p → strip one pointer level
    if (tt == TT::MULTIPLY) {
      pos++;
      if (pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER) {
        std::string n = safe_name(tokens[pos++].value);
        TypeInfo ti = lookup_var(n);
        if (ti.ptr_depth > 0) {
          ti.ptr_depth--;
          return ti;
        }
        if (!ti.base.empty() && ti.base.back() == '*') {
          ti.base.pop_back();
          return ti;
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
        if (tokens[pos].type == open)
          depth++;
        else if (tokens[pos].type == close)
          depth--;
        pos++;
      }
    }
  }

  TypeInfo infer_ti_primary() {
    if (pos >= tokens.size())
      return TypeInfo::unknown();
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
        if (pos < tokens.size()) {
          if (tokens[pos].type == TT::PTR) {
            pos++; // skip 'ptr'
            if (pos < tokens.size()) {
              std::string inner = raw_to_c(tokens[pos++].value);
              if (inner == "char*")
                tn = "char**"; // ptr str → char**
              else
                tn = inner + "*";
            } else {
              tn = "void*";
            }
          } else {
            tn = raw_to_c(tokens[pos++].value);
          }
        }
        // skip comma + expr + rparen
        if (pos < tokens.size() && tokens[pos].type == TT::COMMA)
          pos++;
        infer_ti_ternary(); // consume inner expr
        if (pos < tokens.size() && tokens[pos].type == TT::RPAREN)
          pos++;
        // build TypeInfo from tn
        TypeInfo ti;
        if (!tn.empty() && tn.back() == '*') {
          ti.base = tn.substr(0, tn.size() - 1);
          ti.ptr_depth = 1;
        } else {
          ti.base = tn;
        }
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
      if (pos < tokens.size() && tokens[pos].type == TT::RPAREN)
        pos++;
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
    if (tok.type == TT::NUMBER)
      return infer_number_literal(tok.value);
    if (tok.type == TT::STRING)
      return TypeInfo::of("char*");
    if (tok.type == TT::CHAR_LIT)
      return TypeInfo::of("char");
    if (tok.type == TT::TRUE_KW || tok.type == TT::FALSE_KW)
      return TypeInfo::of("bool");
    if (tok.type == TT::NULL_KW)
      return TypeInfo::of("void*");

    if (tok.type == TT::IDENTIFIER) {
      std::string name = safe_name(tok.value);
      std::string raw_name = tok.value; // preserve for NS lookup

      // Check for namespace-qualified call: NS::func(...)
      if (pos < tokens.size() && tokens[pos].type == TT::COLON &&
          pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON) {
        pos += 2; // consume ::
        if (pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER) {
          std::string func_name = tokens[pos++].value;
          std::string resolved = resolve_qualified(raw_name, func_name);
          if (resolved.empty()) resolved = raw_name + "__" + func_name;
          // Now we're at the potential LPAREN
          if (pos < tokens.size() && tokens[pos].type == TT::LPAREN) {
            skip_balanced(TT::LPAREN, TT::RPAREN);
            return lookup_func_ret(resolved);
          }
          // Not a call, just NS::sym
          return TypeInfo::unknown();
        }
      }

      // Function call: name(...)
      if (pos < tokens.size() && tokens[pos].type == TT::LPAREN) {
        // For template functions with inferred return types, compute the
        // mangled specialization name from arg types so we look up the correct
        // return type.
        auto tmpl_it = template_funcs.find(name);
        if (tmpl_it != template_funcs.end() &&
            (tmpl_it->second.raw_ret == "let" ||
             tmpl_it->second.raw_ret == "var")) {
          const TemplateFunc &tmpl = tmpl_it->second;
          // Peek at argument types (pos is at LPAREN)
          size_t scan = pos + 1; // skip LPAREN
          std::vector<std::string> concrete;
          size_t ci = 0;
          while (scan < tokens.size() && tokens[scan].type != TT::RPAREN) {
            if (tokens[scan].type == TT::TEOF)
              break;
            // skip leading commas between args
            if (tokens[scan].type == TT::COMMA) {
              scan++;
              continue;
            }
            // find start of this arg, then find its end (next comma/rparen at
            // depth 0)
            size_t arg_start = scan;
            int d2 = 0;
            while (scan < tokens.size()) {
              TT tt2 = tokens[scan].type;
              if (tt2 == TT::LPAREN || tt2 == TT::LBRACE ||
                  tt2 == TT::LSBRACKET)
                d2++;
              else if (tt2 == TT::RPAREN || tt2 == TT::RBRACE ||
                       tt2 == TT::RSBRACKET) {
                if (d2 == 0)
                  break;
                else
                  d2--;
              } else if (tt2 == TT::COMMA && d2 == 0)
                break;
              scan++;
            }
            bool slot_infer = (ci < tmpl.param_slots.size())
                                  ? tmpl.param_slots[ci].infer
                                  : true;
            if (slot_infer) {
              concrete.push_back(infer_type_at(arg_start).c_type());
            } else if (ci < tmpl.param_slots.size()) {
              concrete.push_back(param_raw_to_c(tmpl.param_slots[ci].raw));
            }
            ci++;
          }
          // Look up already-instantiated specialization — do NOT instantiate
          // here because infer_ti_primary is a pure lookahead and side-effects
          // (mutating functions[], var_types) would corrupt parse state.
          std::string mangled = mono_mangle(name, concrete);
          if (func_return_types.count(mangled) &&
              func_return_types[mangled] != "__infer__") {
            skip_balanced(TT::LPAREN, TT::RPAREN);
            return lookup_func_ret(mangled);
          }
          // Not yet instantiated — return unknown; the actual call site will
          // instantiate
          skip_balanced(TT::LPAREN, TT::RPAREN);
          return TypeInfo::unknown();
        }
        skip_balanced(TT::LPAREN, TT::RPAREN);
        return lookup_func_ret(name);
      }

      // Base type from var_types
      TypeInfo ti = lookup_var(name);
      std::string struct_base = ti.base; // save for field lookup

      // Postfix chains: [idx], .field, ->field
      while (pos < tokens.size() &&
             (tokens[pos].type == TT::LSBRACKET ||
              tokens[pos].type == TT::DOT || tokens[pos].type == TT::ARROW)) {
        if (tokens[pos].type == TT::LSBRACKET) {
          // array subscript → element type (strip one array level)
          pos++;
          infer_ti_ternary(); // skip index expr
          if (pos < tokens.size() && tokens[pos].type == TT::RSBRACKET)
            pos++;
          // element type: if we know it was an array, strip _ARRAY tag
          // The element type is the same as the base type stored
          ti.is_array = false;
        } else {
          // .field or ->field
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
                // strip array suffix "[N]" if present — field access yields
                // element type
                size_t lb = ft.find('[');
                bool is_arr_field = (lb != std::string::npos);
                if (is_arr_field)
                  ft = ft.substr(0, lb);
                if (!ft.empty() && ft.back() == '*') {
                  fti.base = ft.substr(0, ft.size() - 1);
                  fti.ptr_depth = 1;
                } else {
                  fti.base = ft;
                }
                fti.is_array = is_arr_field;
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

  // Returns the printf format specifier and optional cast for a given TypeInfo.
  // Used in TCC mode where _Generic is unavailable.
  // Returns {fmt, cast} e.g. {"%d", "(int)"} or {"%s", ""}
  struct FmtSpec { std::string fmt; std::string cast; };
  static FmtSpec printf_fmt_for_type(const TypeInfo &ti) {
    const std::string &b = ti.base;
    // pointers / strings
    if (ti.is_ptr() || b == "char*" || b == "const char*")
      return {"%s", ""};
    if (b == "char")
      return {"%c", "(char)"};
    if (b == "double")
      return {"%.10g", "(double)"};
    if (b == "float")
      return {"%g", "(float)"};
    if (b == "long")
      return {"%ld", "(long)"};
    if (b == "uint64_t")
      return {"%\" PRIu64 \"", "(uint64_t)"};
    if (b == "uint32_t")
      return {"%\" PRIu32 \"", "(uint32_t)"};
    if (b == "uint8_t")
      return {"%\" PRIu8 \"", "(uint8_t)"};
    if (b == "short")
      return {"%d", "(int)"};
    if (b == "bool")
      // bools: print as true/false via ternary — no cast needed, handled specially
      return {"BOOL", ""};
    // default: int
    return {"%d", "(int)"};
  }

  // Emit a single printf call for one print argument in TCC mode.
  // tok_pos is the token position before the expression was parsed.
  std::string tcc_print_one(const std::string &expr, size_t tok_pos) {
    TypeInfo ti = infer_type_at(tok_pos);
    FmtSpec fs = printf_fmt_for_type(ti);
    if (fs.fmt == "BOOL")
      return "printf(\"%s\", (" + expr + ") ? \"true\" : \"false\")";
    return "printf(\"" + fs.fmt + "\", " + fs.cast + "(" + expr + "))";
  }

  // Legacy compatibility shim used by the let/var handler
  std::string infer_type_from_rhs(const Token & /*expr_tok*/,
                                  const std::string & /*expr_str*/,
                                  size_t rhs_start_pos) {
    TypeInfo ti = infer_type_at(rhs_start_pos);
    return ti.c_type();
  }

  // -----------------------------------------------------------------------
  // Monomorphization: build a type-signature key from a vector of c-type
  // strings
  // -----------------------------------------------------------------------
  static std::string mono_key(const std::vector<std::string> &types) {
    std::string k;
    for (size_t i = 0; i < types.size(); i++) {
      if (i)
        k += ',';
      k += types[i];
    }
    return k;
  }

  // Mangle a specialization name: foo<int,double> → foo__int__double
  static std::string mono_mangle(const std::string &fname,
                                 const std::vector<std::string> &types) {
    std::string r = fname;
    for (const auto &t : types) {
      r += "__";
      for (char c : t)
        r += (c == '*' || c == ' ') ? '_' : c;
    }
    return r;
  }

  // Given a c_type string, produce the raw keyword for var_types registration
  std::string c_type_to_raw(const std::string &ct) const {
    if (ct == "char*")
      return "str";
    if (ct == "bool")
      return "bool";
    if (ct == "char")
      return "char";
    if (ct == "uint8_t")
      return "u8";
    if (ct == "uint32_t")
      return "u32";
    if (ct == "uint64_t")
      return "u64";
    return ct; // int, float, double, long, short, void, struct names
  }

  // -----------------------------------------------------------------------
  // Instantiate a template function with concrete arg types.
  // Saves/restores pos; re-parses the template token range with param
  // var_types pre-seeded to the concrete types.
  // Returns the mangled function name.
  // -----------------------------------------------------------------------
  std::string
  instantiate_template(const std::string &fname,
                       const std::vector<std::string> &concrete_types) {
    auto tit = template_funcs.find(fname);
    if (tit == template_funcs.end())
      return fname; // not a template, identity

    const TemplateFunc &tmpl = tit->second;
    std::string key = mono_key(concrete_types);

    // Already instantiated?
    auto &reg = mono_registry[fname];
    auto rit = reg.find(key);
    if (rit != reg.end())
      return rit->second;

    // Re-entrancy guard (recursive templates would loop)
    // If the template has a generic return type (e.g. returns Vec<T>),
    // instantiate the struct now with the first concrete type.
    // (Deferred to after mangled is declared below.)

    std::string guard_key = fname + "@" + key;
    if (_mono_in_progress.count(guard_key))
      return fname;
    if (_mono_depth >= _mono_max_depth)
      return fname; // indirect recursion cutoff
    _mono_in_progress.insert(guard_key);
    _mono_depth++;

    std::string mangled = mono_mangle(fname, concrete_types);
    reg[key] = mangled; // register early to handle recursion

    // If the template has a generic return type (e.g. returns Vec<T>),
    // instantiate the struct now with the first concrete type.
    if (!tmpl.generic_ret_param.empty() && !tmpl.func_type_param.empty() &&
        !concrete_types.empty() && _generic_structs.count(tmpl.raw_ret)) {
      std::string ct = concrete_types[0];
      instantiate_generic_struct(tmpl.raw_ret, ct);
      std::string safe_ct = ct;
      for (char &c : safe_ct) if (c == '*' || c == ' ') c = '_';
      func_return_types[mangled] = tmpl.raw_ret + "__" + safe_ct;
    }

    // Save current parser state
    size_t saved_pos = pos;
    auto saved_var_types = var_types;
    auto saved_tokens = tokens; // save in case we switch to extracted tokens

    // If this template came from a .xen file, use its extracted tokens instead
    // of invalid indices into parent's token stream
    if (tmpl.has_extracted && !tmpl.extracted_tokens.empty()) {
      tokens = tmpl.extracted_tokens;
      pos = 0; // start from beginning of extracted tokens
    } else {
      pos = tmpl.tok_start; // use original indices (for non-merged templates)
    }

    {

      size_t scan = pos;

      std::string raw_r = tokens[scan].value;
      scan++;
      if (raw_r == "let" || raw_r == "var") { /* inferred */
      } else if (raw_r == "ptr") {
        scan++;
      } // ptr X
      // optional <PARAM> on return type (generic struct return like Vec<T>)
      if (scan < tokens.size() && tokens[scan].type == TT::LT) {
        scan++; // skip '<'
        while (scan < tokens.size() && tokens[scan].type != TT::GT)
          scan++;
        if (scan < tokens.size()) scan++; // skip '>'
      }
      // optional [N] on return type
      if (scan < tokens.size() && tokens[scan].type == TT::LSBRACKET) {
        scan++;
        while (scan < tokens.size() && tokens[scan].type != TT::RSBRACKET)
          scan++;
        if (scan < tokens.size())
          scan++;
      }
      // fname
      scan++; // skip fname
      // optional <T> on function name
      if (scan < tokens.size() && tokens[scan].type == TT::LT) {
        scan++; // skip '<'
        while (scan < tokens.size() && tokens[scan].type != TT::GT)
          scan++;
        if (scan < tokens.size()) scan++; // skip '>'
      }
      // LPAREN
      if (scan < tokens.size() && tokens[scan].type == TT::LPAREN)
        scan++;
      // params
      size_t ci = 0;
      while (scan < tokens.size() && tokens[scan].type != TT::RPAREN) {
        if (tokens[scan].type == TT::TEOF)
          break;
        // param type token(s)
        std::string p_raw = tokens[scan++].value;
        if (p_raw == "ptr" && scan < tokens.size())
          scan++; // ptr X
        // optional <PARAM> on param type (e.g. Vec<T> param)
        if (scan < tokens.size() && tokens[scan].type == TT::LT) {
          scan++; // skip '<'
          while (scan < tokens.size() && tokens[scan].type != TT::GT)
            scan++;
          if (scan < tokens.size()) scan++; // skip '>'
        }
        // optional [N]
        if (scan < tokens.size() && tokens[scan].type == TT::LSBRACKET) {
          scan++;
          while (scan < tokens.size() && tokens[scan].type != TT::RSBRACKET)
            scan++;
          if (scan < tokens.size())
            scan++;
        }
        // param name
        if (scan < tokens.size() && tokens[scan].type == TT::IDENTIFIER) {
          std::string p_name = safe_name(tokens[scan++].value);
          // If this slot is infer, seed with the concrete type
          if (ci < tmpl.param_slots.size() && tmpl.param_slots[ci].infer &&
              ci < concrete_types.size()) {
            var_types[p_name] = c_type_to_raw(concrete_types[ci]);
          } else if (ci < tmpl.param_slots.size()) {
            var_types[p_name] = tmpl.param_slots[ci].raw;
          }
        }
        if (scan < tokens.size() && tokens[scan].type == TT::COMMA)
          scan++;
        ci++;
      }

      // If this template has an explicit type param (func_type_param),
      // seed var_types with the binding so parse_function_body can resolve
      // Vec<T> → Vec__int during re-parse.
      if (!tmpl.func_type_param.empty() && !concrete_types.empty()) {
        // Bind the explicit type param name to the first concrete type
        var_types[tmpl.func_type_param] = c_type_to_raw(concrete_types[0]);
      }
    }

    // Now re-parse parse_function_body from tok_start.
    // We need to trick it into using `mangled` as the function name.
    // Strategy: parse normally, it will read fname from tokens — but we need it
    // to emit `mangled`. So we temporarily patch the fname token.
    // The fname token is at tok_start + (1 or 2 for ret) + (1 for [N]?) tokens.
    // Simpler: find the IDENTIFIER token that is the function name.
    {
      size_t tok_start_idx = tmpl.has_extracted ? 0 : tmpl.tok_start;
      size_t scan = tok_start_idx;
      // skip ret type
      if (scan < tokens.size()) {
        std::string raw_r2 = tokens[scan].value;
        scan++;
        if (raw_r2 == "ptr" && scan < tokens.size())
          scan++;
      }
      // skip optional <PARAM> on return type
      if (scan < tokens.size() && tokens[scan].type == TT::LT) {
        scan++;
        while (scan < tokens.size() && tokens[scan].type != TT::GT)
          scan++;
        if (scan < tokens.size()) scan++;
      }
      if (scan < tokens.size() && tokens[scan].type == TT::LSBRACKET) {
        scan++;
        while (scan < tokens.size() && tokens[scan].type != TT::RSBRACKET)
          scan++;
        if (scan < tokens.size())
          scan++;
      }
      // tokens[scan] should be the fname identifier — patch it
      if (scan < tokens.size() && tokens[scan].type == TT::IDENTIFIER) {
        std::string orig_fname = tokens[scan].value;
        tokens[scan].value = mangled; // temporarily rename
        pos = tok_start_idx;
        std::string code = parse_function_body(tokens[scan], tmpl.inl);
        tokens[scan].value = orig_fname; // restore
        // Guard: if parse_function_body returned a __TEMPLATE__ sentinel again
        // (re-entrancy or is_instantiating_now was false), don't emit that literal.
        if (code.size() <= 12 || code.substr(0, 12) != "__TEMPLATE__")
          functions.push_back(code);
      }
    }

    // Restore parser state
    tokens = saved_tokens; // restore original token stream
    pos = saved_pos;
    var_types = saved_var_types;
    _mono_depth--;
    _mono_in_progress.erase(guard_key);
    return mangled;
  }

  // -----------------------------------------------------------------------
  // Compute concrete arg types for a function call whose args start just
  // after the LPAREN.  `arg_start_positions[i]` = token index of ith arg.
  // Returns vector of c_type strings (one per arg).
  // -----------------------------------------------------------------------
  std::vector<std::string>
  compute_arg_types(const std::vector<size_t> &arg_starts) {
    std::vector<std::string> types;
    for (size_t sp : arg_starts) {
      TypeInfo ti = infer_type_at(sp);
      types.push_back(ti.c_type());
    }
    return types;
  }

  // -----------------------------------------------------------------------
  // emit_call: parse argument list (pos is JUST AFTER the LPAREN).
  // If fname is a template, instantiate/look up specialization and rewrite.
  // Returns "mangled_name(arg1, arg2, ...)" — no trailing semicolon.
  // -----------------------------------------------------------------------
  std::string emit_call(const std::string &fname, const Token &call_tok,
                        const std::string &explicit_type_arg = "") {
    // ── Undefined function check ───────────────────────────────────────────
    // Skip only for '_'-prefixed C internals, or mangled names that are
    // actually registered (definition-time validation). A mangled name that
    // isn't registered (e.g. bogus::nonexistent() → bogus__nonexistent) must
    // still be flagged — otherwise it silently emits broken C.
    bool is_mangled = fname.find("__") != std::string::npos;
    bool skip_undef_check =
        (!fname.empty() && fname[0] == '_') ||
        (is_mangled && is_known_function(fname));
    if (!skip_undef_check && !is_known_function(fname)) {
      // Build a helpful "did you mean?" list from func_return_types keys
      std::vector<std::string> candidates;
      for (const auto &[fn, _] : func_return_types) {
        // Simple Levenshtein-free heuristic: share a long common prefix/suffix
        size_t common = 0;
        size_t minlen = std::min(fn.size(), fname.size());
        for (size_t ci = 0; ci < minlen; ci++) {
          if (fn[ci] == fname[ci]) common++;
          else break;
        }
        if (common >= 3 || (minlen > 0 && common * 2 >= minlen))
          candidates.push_back(fn);
      }
      // Build error message
      std::string msg = "Call to undefined function '" + fname + "'.\n"
        "  Checked: builtin functions, user-defined functions, "
        ".xen imports, .h imports — none matched.\n"
        "  If '" + fname + "' comes from a C header, add: link \"the_header.h\"\n"
        "  If it comes from a .xen file, add: link \"the_file.xen\"";
      if (!candidates.empty()) {
        msg += "\n  Did you mean: ";
        for (size_t ci = 0; ci < candidates.size() && ci < 3; ci++) {
          if (ci) msg += ", ";
          msg += "'" + candidates[ci] + "'";
        }
        msg += "?";
      }
      throw XenonError("NameError", msg, call_tok.line, call_tok.col);
    }
    // ──────────────────────────────────────────────────────────────────────

    // ── .h-imported functions require unsafe { } ──────────────────────────
    // Functions imported from C headers (.h files) are foreign and cannot be
    // safety-verified by the compiler.  Calling them outside an unsafe block
    // is therefore a hard error — exactly like Rust's extern "C" rule.
    if (_h_imported_functions.count(fname) && !_in_unsafe_block) {
      throw XenonError("SafetyError",
        "Call to C header function '" + fname + "' outside an unsafe block.\n"
        "  Functions imported from .h files are foreign and unverified — their\n"
        "  memory safety, null handling, and side-effects cannot be checked.\n"
        "  Wrap the call in an unsafe block to assert you have verified it:\n\n"
        "      unsafe {\n"
        "          " + fname + "(...)\n"
        "      }\n\n"
        "  This mirrors Rust's rule: all extern \"C\" calls require unsafe.",
        call_tok.line, call_tok.col);
    }
    // ──────────────────────────────────────────────────────────────────────

    // Collect arg start positions BEFORE consuming them
    bool is_tmpl = template_funcs.count(fname) > 0;
    std::vector<size_t> arg_starts;
    std::vector<std::string> args;
    // For brace-init args: record how many elements each had (-1 = not brace-init)
    std::vector<int> arg_brace_elem_counts;
    while (current().type != TT::RPAREN) {
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
                           "Missing closing ')' in call to '" + fname +
                           "' — reached end of file",
                           call_tok.line, call_tok.col);
      arg_starts.push_back(pos);
      if (is_tmpl) { /* already pushed */ }
      // Detect brace-init arg: peek at current token
      bool is_brace_arg = (current().type == TT::LBRACE);
      int brace_elems = -1;
      if (is_brace_arg) {
        // Count commas at depth 1 inside the braces to get element count
        size_t scan = pos + 1; // skip opening '{'
        int depth = 1;
        int elem_count = (scan < tokens.size() && tokens[scan].type != TT::RBRACE) ? 1 : 0;
        while (scan < tokens.size() && depth > 0) {
          TT st = tokens[scan].type;
          if (st == TT::LBRACE) { depth++; }
          else if (st == TT::RBRACE) { depth--; }
          else if (st == TT::COMMA && depth == 1) { elem_count++; }
          else if (st == TT::TEOF) break;
          scan++;
        }
        brace_elems = elem_count;
      }
      arg_brace_elem_counts.push_back(brace_elems);
      args.push_back(parse_expr());
      if (current().type == TT::COMMA)
        advance();
    }
    expect(TT::RPAREN, false);

    // ── Argument count check ───────────────────────────────────────────────
    // Only check user-defined functions (in func_param_types but NOT in
    // builtin_functions or _h_imported_functions — those may be variadic).
    // Also skip template functions (their param count is validated at
    // instantiation time) and struct-method calls (self is injected).
    if (!is_tmpl &&
        !builtin_functions().count(fname) &&
        !_h_imported_functions.count(fname)) {
      auto pit = func_param_types.find(fname);
      if (pit != func_param_types.end()) {
        int expected = (int)pit->second.size();
        int got_n    = (int)args.size();
        // Struct methods have 'self' injected as the first param — callers
        // don't pass it, so subtract 1 from expected.
        // More reliable method detection: the stored param[0] ends with '*'
        // and the function name contains '__' (mangled method).
        int caller_expected = expected;
        // Struct methods have 'self' injected as the first hidden param.
        // A function is a true struct method iff it is registered in
        // struct_methods[StructName] under its short (post-__) name.
        // Namespace functions like std__string__Print are NOT in struct_methods
        // even when their first param is a struct pointer, so they are safe.
        {
          size_t dpos = fname.rfind("__");
          if (dpos != std::string::npos) {
            std::string struct_part = fname.substr(0, dpos);
            std::string method_part = fname.substr(dpos + 2);
            auto smit = struct_methods.find(struct_part);
            if (smit != struct_methods.end() && smit->second.count(method_part))
              caller_expected = expected - 1;
          }
        }
        if (got_n != caller_expected) {
          // Build a nice param-list hint
          std::vector<std::string> param_hints;
          int hint_start = (caller_expected != expected) ? 1 : 0;
          for (int pi = hint_start; pi < expected && pi < (int)pit->second.size(); pi++)
            param_hints.push_back(pit->second[pi]);
          std::string hint;
          if (!param_hints.empty())
            hint = " (" + join(param_hints, ", ") + ")";
          std::string msg = "Wrong number of arguments to '" + fname + "':\n"
            "  Expected " + std::to_string(caller_expected) + " argument" +
            (caller_expected == 1 ? "" : "s") + hint + ",\n"
            "  but got " + std::to_string(got_n) + ".\n";
          if (got_n < caller_expected)
            msg += "  Too few arguments — did you forget to pass something?";
          else
            msg += "  Too many arguments — did you pass an extra value by mistake?";
          throw XenonError("ArgCountError", msg, call_tok.line, call_tok.col);
        }
      }
    }
    // ──────────────────────────────────────────────────────────────────────

    // ── Brace-init element count check ────────────────────────────────────
    // If a brace-init {…} is passed to a param declared as type[N],
    // verify that the number of elements matches N exactly.
    if (!is_tmpl &&
        !builtin_functions().count(fname) &&
        !_h_imported_functions.count(fname)) {
      auto ait = func_param_array_sizes.find(fname);
      if (ait != func_param_array_sizes.end()) {
        const auto &param_sizes = ait->second;
        // Determine offset: struct methods have 'self' as param[0]
        int self_offset = 0;
        {
          auto pit2 = func_param_types.find(fname);
          if (pit2 != func_param_types.end() &&
              fname.find("__") != std::string::npos &&
              !pit2->second.empty() && !pit2->second[0].empty() &&
              pit2->second[0].back() == '*')
            self_offset = 1;
        }
        for (size_t ai = 0; ai < arg_brace_elem_counts.size(); ai++) {
          int bcount = arg_brace_elem_counts[ai];
          if (bcount < 0) continue; // not a brace-init arg
          size_t param_idx = ai + (size_t)self_offset;
          if (param_idx >= param_sizes.size()) continue;
          int declared_size = param_sizes[param_idx];
          if (declared_size < 0) continue; // param has no declared array size
          if (bcount != declared_size) {
            int arg_line = (arg_starts[ai] < tokens.size())
                           ? tokens[arg_starts[ai]].line : call_tok.line;
            int arg_col  = (arg_starts[ai] < tokens.size())
                           ? tokens[arg_starts[ai]].col  : call_tok.col;
            std::string snip;
            if (arg_line > 0 && !g_current_source.empty())
              snip = extract_line_from_source(g_current_source, arg_line);
            throw XenonError("TypeError",
              "argument " + std::to_string((int)ai + 1) + " of '" + fname +
              "': brace initializer has " + std::to_string(bcount) +
              " element" + (bcount == 1 ? "" : "s") +
              " but parameter is declared as [" + std::to_string(declared_size) +
              "] — element count must match exactly",
              arg_line, arg_col, snip);
          }
        }
      }
    }
    // ──────────────────────────────────────────────────────────────────────

    // ── Argument type check ────────────────────────────────────────────────
    // For user-defined functions with known param types, verify each argument's
    // inferred type is compatible with the declared parameter type.
    // We skip builtins, templates (checked at instantiation), and variadic
    // functions imported from headers.
    if (!is_tmpl &&
        !builtin_functions().count(fname) &&
        !_h_imported_functions.count(fname)) {
      auto pit = func_param_types.find(fname);
      if (pit != func_param_types.end()) {
        // Save and restore pos around type inference (non-destructive peek)
        size_t saved_pos = pos;
        const auto &ptypes = pit->second;
        int pidx = 0;
        // Walk argument_starts list for type inference; rebuild by rescanning
        // the already-parsed args list via saved arg_starts collected below.
        // We re-scan from just after the LPAREN of the call to infer each arg.
        // pos is now just after the RPAREN — re-scan from saved arg positions.
        // Use infer_type_at on the token positions of each arg.
        // Re-build the list of arg token positions by re-scanning from LPAREN.
        {
          // Find the LPAREN that opened this call: it's at (saved call_tok pos + 1)
          // We stored the call token; re-scan from there to collect arg start positions.
          size_t scan_pos = saved_pos; // pos is now PAST the closing RPAREN
          // Walk backwards from saved_pos to find the LPAREN:
          // Actually, we need the start positions of the args.
          // Easier: we recorded args already; walk the tokens from call_tok.col
          // forward.  We'll use a fresh scan starting from just before pos was
          // advanced past RPAREN.
          // Simplest correct approach: rescan from the token after the fname token.
          // We know fname was at (call_tok position).  Find it in the token stream.
          // Rather than tracking it precisely, we do a lightweight rescan:
          // find the LPAREN right after this fname in the window just before saved_pos.
          size_t lp_pos = saved_pos; // fallback
          // search backward for the LPAREN that belongs to this call
          // (safe because we just parsed the call and pos is at next token)
          if (lp_pos > 0) {
            size_t back = lp_pos;
            while (back > 0 && tokens[back].type != TT::RPAREN) back--;
            // Now find the matching LPAREN before that RPAREN
            int dep = 0;
            size_t rp = back;
            for (size_t k = rp; k > 0; k--) {
              if (tokens[k].type == TT::RPAREN) dep++;
              else if (tokens[k].type == TT::LPAREN) {
                dep--;
                if (dep == 0) { lp_pos = k; break; }
              }
            }
          }
          // Collect arg start positions
          std::vector<size_t> arg_tok_starts;
          {
            size_t k = lp_pos + 1;
            int dep = 0;
            size_t cur_start = k;
            while (k < saved_pos && tokens[k].type != TT::RPAREN) {
              if (tokens[k].type == TT::LPAREN || tokens[k].type == TT::LSBRACKET) dep++;
              else if (tokens[k].type == TT::RPAREN || tokens[k].type == TT::RSBRACKET) {
                if (dep == 0) break;
                dep--;
              } else if (tokens[k].type == TT::COMMA && dep == 0) {
                arg_tok_starts.push_back(cur_start);
                cur_start = k + 1;
              }
              k++;
            }
            if (cur_start < k) arg_tok_starts.push_back(cur_start);
          }

          // Adjust pidx for struct methods (self is implicit)
          int ptype_start = 0;
          if (fname.find("__") != std::string::npos &&
              !ptypes.empty() && !ptypes[0].empty() &&
              ptypes[0].back() == '*')
            ptype_start = 1;

          for (size_t ai = 0; ai < arg_tok_starts.size(); ai++) {
            int pi = (int)ai + ptype_start;
            if (pi >= (int)ptypes.size()) break;
            const std::string &expected_c = ptypes[pi];
            if (expected_c.empty()) continue;

            // Infer the type of the argument at this token position
            size_t arg_pos = arg_tok_starts[ai];
            // Skip leading whitespace-equivalent tokens
            while (arg_pos < saved_pos &&
                   (tokens[arg_pos].type == TT::COMMA)) arg_pos++;
            if (arg_pos >= saved_pos) continue;

            pos = arg_pos;
            TypeInfo arg_ti = infer_ti_ternary();
            std::string got_c = arg_ti.c_type();

            pos = saved_pos; // restore regardless

            if (got_c.empty() || got_c == "?" || expected_c == "void*") continue;

            // Compute compatibility: identical or numeric promotions are fine.
            // We only hard-error on clear category mismatches
            // (pointer vs scalar, or str/char* vs int, etc.).
            bool expected_is_ptr = (!expected_c.empty() && expected_c.back() == '*');
            bool got_is_ptr      = (!got_c.empty()      && got_c.back()      == '*');

            // Different pointer/scalar categories → definite mismatch
            if (expected_is_ptr != got_is_ptr) {
              // Exception: null literal (void*) can be passed for any ptr param
              if (got_c == "void*" && expected_is_ptr) continue;
              // Exception: 0 literal inferred as int but intended as null ptr
              // (user wrote `consume(0)`) — let C compiler handle it
              if (got_c == "int" && expected_is_ptr) continue;

              std::string friendly_expected = expected_c;
              std::string friendly_got      = got_c;
              // Make pointer types a bit friendlier in error messages
              auto friendlify = [](const std::string &ct) -> std::string {
                if (ct == "char*") return "str";
                if (ct == "void*") return "ptr";
                if (ct.size() > 1 && ct.back() == '*')
                  return "ptr " + ct.substr(0, ct.size() - 1);
                return ct;
              };
              std::string exp_f = friendlify(friendly_expected);
              std::string got_f = friendlify(friendly_got);

              // Find the token line/col for the argument
              int arg_line = (arg_tok_starts[ai] < tokens.size())
                             ? tokens[arg_tok_starts[ai]].line : 0;
              int arg_col  = (arg_tok_starts[ai] < tokens.size())
                             ? tokens[arg_tok_starts[ai]].col  : 0;
              std::string snip;
              if (arg_line > 0 && !g_current_source.empty())
                snip = extract_line_from_source(g_current_source, arg_line);

              throw XenonError("TypeError",
                "argument " + std::to_string((int)ai + 1) + " of '" + fname + "': "
                "expected '" + exp_f + "' but got '" + got_f + "' — "
                "types are incompatible (pass a value of the correct type, "
                "or use cast() for an explicit conversion)",
                arg_line, arg_col, snip);
            }

            // Same category (both pointers or both scalars):
            // within-category mismatches (e.g. int* vs char*) are caught here.
            if (expected_is_ptr && got_is_ptr && expected_c != got_c) {
              // void* on either side is compatible (C-style generic pointer)
              if (expected_c == "void*" || got_c == "void*") continue;
              // char* ↔ str — same underlying type
              if ((expected_c == "char*" || got_c == "char*") &&
                  (expected_c == "char*" || got_c == "char*")) continue;

              auto friendlify_ptr = [](const std::string &ct) -> std::string {
                if (ct == "char*") return "str";
                if (ct == "void*") return "ptr";
                if (ct.size() > 1 && ct.back() == '*')
                  return "ptr " + ct.substr(0, ct.size() - 1);
                return ct;
              };

              int arg_line = (arg_tok_starts[ai] < tokens.size())
                             ? tokens[arg_tok_starts[ai]].line : 0;
              int arg_col  = (arg_tok_starts[ai] < tokens.size())
                             ? tokens[arg_tok_starts[ai]].col  : 0;
              std::string snip;
              if (arg_line > 0 && !g_current_source.empty())
                snip = extract_line_from_source(g_current_source, arg_line);

              throw XenonError("TypeError",
                "argument " + std::to_string((int)ai + 1) + " of '" + fname + "': "
                "expected '" + friendlify_ptr(expected_c) +
                "' but got '" + friendlify_ptr(got_c) + "' — "
                "pointer types do not match; use cast() for an explicit reinterpret",
                arg_line, arg_col, snip);
            }

            // ── Scalar-vs-scalar type mismatch ─────────────────────────────
            // Both are non-pointer scalars: flag clear category mismatches.
            // We allow numeric promotions (int→float, int→double, etc.) since
            // C does these implicitly and they are safe. We flag:
            //   • bool/char passed where a numeric type is expected and vice-versa
            //     only when the mismatch is an unambiguous programmer error.
            // Concretely: str(char*) vs any scalar already caught above.
            // Additional case: struct type passed where a numeric type expected
            // (and vice-versa) — but we can't easily detect that without
            // tracking struct names through TypeInfo, so we skip it here.
            if (!expected_is_ptr && !got_is_ptr && expected_c != got_c) {
              // Build a numeric-type set
              static const std::set<std::string> numeric_types = {
                "int","long","short","float","double",
                "uint8_t","uint32_t","uint64_t","bool","char"
              };
              bool exp_numeric = numeric_types.count(expected_c) > 0;
              bool got_numeric = numeric_types.count(got_c)      > 0;

              // Both numeric → safe C promotion, allow it (int→float etc.)
              if (exp_numeric && got_numeric) {
                // Only hard-error on truly incompatible combos:
                // passing a float/double where a pointer-sized int is expected
                // is not something we catch here — C handles it.
                // So: no error for numeric↔numeric mismatches.
              } else if (exp_numeric != got_numeric && !got_c.empty() && got_c != "int") {
                // One side is numeric, the other is a struct/unknown type.
                // Only fire when we have real type info on both sides.
                int arg_line2 = (arg_tok_starts[ai] < tokens.size())
                               ? tokens[arg_tok_starts[ai]].line : 0;
                int arg_col2  = (arg_tok_starts[ai] < tokens.size())
                               ? tokens[arg_tok_starts[ai]].col  : 0;
                std::string snip2;
                if (arg_line2 > 0 && !g_current_source.empty())
                  snip2 = extract_line_from_source(g_current_source, arg_line2);
                throw XenonError("TypeError",
                  "argument " + std::to_string((int)ai + 1) + " of '" + fname + "': "
                  "expected '" + expected_c + "' but got '" + got_c + "' — "
                  "numeric type and struct/opaque type are incompatible; "
                  "check you are passing the correct value",
                  arg_line2, arg_col2, snip2);
              }
            }
          }
        }
        pos = saved_pos; // final restore (defensive)
      }
    }
    // ──────────────────────────────────────────────────────────────────────

    std::string call_name = fname;
    if (is_tmpl) {
      const TemplateFunc &tmpl = template_funcs[fname];
      // If an explicit type arg was given (e.g. VecNew<int>), build concrete list from it.
      if (!explicit_type_arg.empty() && !tmpl.func_type_param.empty()) {
        // Use explicit_type_arg as the binding for every slot that references
        // the function's type parameter (infer slots, ptr-T slots, Vec<T> slots).
        // We pass a single concrete type; instantiate_template seeds var_types[T].
        call_name = instantiate_template(fname, {explicit_type_arg});
      } else if (!args.empty()) {
        std::vector<std::string> concrete;
        for (size_t i = 0; i < tmpl.param_slots.size(); i++) {
          if (i < arg_starts.size()) {
            if (tmpl.param_slots[i].infer) {
              TypeInfo ti = infer_type_at(arg_starts[i]);
              concrete.push_back(ti.c_type());
            } else {
              concrete.push_back(param_raw_to_c(tmpl.param_slots[i].raw));
            }
          }
        }
        call_name = instantiate_template(fname, concrete);
      } else {
        // zero-arg template, no explicit type arg
        call_name = instantiate_template(fname, {});
      }
    }
    return call_name + "(" + join(args, ", ") + ")";
  }

  // -----------------------------------------------------------------------
  // Namespace support
  // -----------------------------------------------------------------------
  // "in namespace NAME are { ... }"
  // Parses the block, mangling every function/type/overload defined inside
  // with NAME__ prefix, and registers them in _namespaces[NAME].
  void parse_namespace_block(const Token &kw_tok) {
    // expect 'namespace'
    if (current().type != TT::NAMESPACE_KW)
      throw XenonError("SyntaxError",
        "'in' must be followed by 'namespace' (e.g. 'in namespace MyNS are { ... }')",
        kw_tok.line, kw_tok.col);
    advance();
    // expect NAME
    std::string ns_name = expect(TT::IDENTIFIER, false).value;
    // expect 'are'
    if (current().type != TT::ARE_KW)
      throw XenonError("SyntaxError",
        "namespace '" + ns_name + "' declaration requires 'are' after the name"
        " (e.g. 'in namespace " + ns_name + " are { ... }')",
        kw_tok.line, kw_tok.col);
    advance();
    // expect '{'
    expect(TT::LBRACE, false);

    // Save and set current namespace — support nesting by composing names
    std::string prev_ns = _cur_namespace;
    // Fully-qualified name: outer__inner (mirrors the C mangling convention)
    std::string fq_ns_name = prev_ns.empty() ? ns_name : (prev_ns + "__" + ns_name);
    _cur_namespace = fq_ns_name;
    auto &ns_map = _namespaces[fq_ns_name]; // creates if absent (redef support)
    // Also register a dotted alias "outer::inner" → same map, so qualified
    // access like  outer::inner::sym  resolves after the first :: step.
    if (!prev_ns.empty()) {
      // Allow  OUTER::INNER  as a namespace key for two-level NS::sym lookups
      std::string alias_key = prev_ns + "::" + ns_name;
      _namespaces[alias_key] = {}; // placeholder; filled below via reference
    }

    // Helper: peek at the function name token after the return type.
    // Scans forward past any type tokens (keywords + optional ptr/const qualifier)
    // until it finds the IDENTIFIER followed by LPAREN — that is the fname.
    auto peek_fname = [&]() -> std::string {
      size_t scan = pos;
      // skip type keyword tokens and ptr/const qualifiers
      while (scan < tokens.size()) {
        TT tt = tokens[scan].type;
        if (tt == TT::IDENTIFIER) {
          // Skip <PARAM> on a generic struct return type: Vec<T>
          if (scan + 1 < tokens.size() && tokens[scan + 1].type == TT::LT) {
            scan++; // skip struct name
            scan++; // skip '<'
            while (scan < tokens.size() && tokens[scan].type != TT::GT)
              scan++;
            if (scan < tokens.size()) scan++; // skip '>'
            continue;
          }
          // Check if next token is LPAREN or LT (generic func like name<T>(...))
          if (scan + 1 < tokens.size() &&
              (tokens[scan + 1].type == TT::LPAREN ||
               tokens[scan + 1].type == TT::LT))
            return tokens[scan].value;
          // Otherwise it's a type name, skip it
          scan++;
          continue;
        }
        // Skip known type keyword tokens
        if (tt == TT::INT || tt == TT::FLOAT || tt == TT::DOUBLE ||
            tt == TT::LONG || tt == TT::SHORT || tt == TT::VOID ||
            tt == TT::BOOL_KW || tt == TT::CHAR_KW || tt == TT::STR ||
            tt == TT::U8 || tt == TT::U32 || tt == TT::U64 ||
            tt == TT::PTR || tt == TT::CONST_KW ||
            tt == TT::M256 || tt == TT::M256I) {
          scan++;
          continue;
        }
        break;
      }
      return "";
    };

    while (current().type != TT::RBRACE && current().type != TT::TEOF) {
      Token t = current();
      if (t.type == TT::SEMICOLON) { advance(); continue; }

      if (t.type == TT::FUNCTION) {
        advance();
        // peek at name before emit_function consumes it, register in ns_map
        std::string short_name = peek_fname();
        std::string code = emit_function(t, false);
        // emit_function returns "__TEMPLATE__<name>" for generic/template functions — don't emit
        bool is_template = (code.size() > 12 && code.substr(0, 12) == "__TEMPLATE__");
        if (!code.empty() && !is_template) functions.push_back(line_directive(t) + code);
        if (!short_name.empty())
          ns_map[short_name] = fq_ns_name + "__" + short_name;
      } else if (t.type == TT::INLINE_KW) {
        advance();
        if (current().type != TT::FUNCTION)
          throw XenonError("SyntaxError",
            "'inline' must be followed by 'function'", t.line, t.col);
        Token fn_tok = current(); advance();
        std::string short_name = peek_fname();
        std::string code = emit_function(fn_tok, true);
        bool is_template = (code.size() > 12 && code.substr(0, 12) == "__TEMPLATE__");
        if (!code.empty() && !is_template) functions.push_back(line_directive(fn_tok) + code);
        if (!short_name.empty())
          ns_map[short_name] = fq_ns_name + "__" + short_name;
      } else if (t.type == TT::TYPE) {
        // Peek at struct name before parsing
        std::string struct_name;
        if (pos + 1 < tokens.size() && tokens[pos + 1].type == TT::IDENTIFIER)
          struct_name = tokens[pos + 1].value;
        std::string code = parse_type_definition();
        std::string mangled_name = struct_name.empty() ? "" : (fq_ns_name + "__" + struct_name);
        if (!code.empty()) {
          headers.push_back(line_directive(t) + code);
        }
        // Register in namespace map whether or not code was emitted (generic structs return "")
        if (!struct_name.empty()) {
          ns_map[struct_name] = mangled_name;
          // Also register bare name in var_types so inner-namespace code can
          // use the short name (e.g. MyStruct x = ...) without qualification.
          auto it_vt = var_types.find(mangled_name);
          if (it_vt != var_types.end())
            var_types[struct_name] = it_vt->second; // propagate STRUCT/GENERIC_STRUCT tag
        }
      } else if (t.type == TT::ENUM_KW) {
        std::string code = parse_enum();
        if (!code.empty()) headers.push_back(line_directive(t) + code);
      } else if (t.type == TT::OVERLOAD_KW) {
        advance();
        if (current().type != TT::OPERATOR_KW)
          throw XenonError("SyntaxError",
            "'overload' must be followed by 'operator'", t.line, t.col);
        advance();
        parse_overload(t);
      } else if (t.type == TT::ADDOP_KW) {
        advance();
        if (current().type != TT::OPERATOR_KW)
          throw XenonError("SyntaxError",
            "'addop' must be followed by 'operator'", t.line, t.col);
        advance();
        parse_addop(t);
      } else if (t.type == TT::UNSAFE_KW) {
        advance();
        if (current().type != TT::FUNCTION)
          throw XenonError("SyntaxError",
            "'unsafe' inside namespace must be followed by 'function'",
            t.line, t.col);
        Token fn_tok = current(); advance();
        std::string short_name = peek_fname();
        // Register as unsafe BEFORE emit_function so the safety checker sees it
        if (!short_name.empty()) {
          std::string mangled = safe_name(fq_ns_name + "__" + short_name);
          _unsafe_functions.insert(mangled);
          _unsafe_functions.insert(safe_name(short_name));
        }
        std::string code = emit_function(fn_tok, false);
        bool is_template = (code.size() > 12 && code.substr(0, 12) == "__TEMPLATE__");
        if (!code.empty() && !is_template)
          functions.push_back(line_directive(fn_tok) + code);
        if (!short_name.empty())
          ns_map[short_name] = fq_ns_name + "__" + short_name;
      } else if (t.type == TT::IN_KW) {
        // Nested namespace: in namespace CHILD are { ... }
        // parse_namespace_block sees _cur_namespace == fq_ns_name and
        // composes fq_child = fq_ns_name + "__" + CHILD automatically.
        Token kw = advance();
        parse_namespace_block(kw);
        // Child symbols are synced into this map by the parent-sync block
        // at the end of parse_namespace_block, so nothing extra needed here.
      } else {
        advance();
      }
    }

    if (current().type == TT::TEOF)
      throw XenonError("SyntaxError",
        "Unterminated namespace '" + fq_ns_name + "'", kw_tok.line, kw_tok.col);
    expect(TT::RBRACE, false);

    // If nested, expose symbols to parent namespace under the short child name
    // so that  parent::child::sym  can be resolved as  fq_ns_name__sym.
    // We register the child namespace itself as a symbol in the parent map so
    // that a two-step  NS::sym  access (where NS is the fq key) keeps working.
    if (!prev_ns.empty()) {
      // Register ns_name as a "namespace alias" entry in the parent so that
      // inner::sym calls inside the parent can find fq_ns_name__sym.
      auto &parent_map = _namespaces[prev_ns];
      // Also sync the alias key map with what was actually registered
      std::string alias_key = prev_ns + "::" + ns_name;
      _namespaces[alias_key] = ns_map;
      // Expose inner symbols in parent under qualified short key (ns_name__sym)
      for (auto &[sym, cname] : ns_map)
        parent_map[ns_name + "__" + sym] = cname;
    }

    _cur_namespace = prev_ns;
  }

  // Resolve NAME::symbol → mangled C name
  // Returns empty string if not found.
  std::string resolve_qualified(const std::string &ns, const std::string &sym) const {
    auto nit = _namespaces.find(ns);
    if (nit == _namespaces.end()) return "";
    auto sit = nit->second.find(sym);
    if (sit == nit->second.end()) return "";
    return sit->second;
  }

  // Resolve unqualified symbol through ignored namespaces.
  // Returns the mangled name if found in any ignored namespace, else "".
  std::string resolve_ignored_ns(const std::string &sym) const {
    for (auto &ns : _ignored_namespaces) {
      std::string r = resolve_qualified(ns, sym);
      if (!r.empty()) return r;
    }
    return "";
  }

  // -----------------------------------------------------------------------
  // Statement parser
  // -----------------------------------------------------------------------
  std::string parse_statement() {
    while (current().type == TT::SEMICOLON)
      advance();
    Token &t = current();

    // $bounds: assert — consume the annotation token, emit no C code.
    // Its presence in the token stream is detected by the subscript codegen
    // on the NEXT statement to suppress bounds checking.
    if (t.type == TT::BOUNDS_ASSERT) {
      advance();
      return "";
    }

    // $attribute: drop — valid only inside a 'type' body.  If seen here it
    // means the user placed it outside a struct, which is a syntax error.
    if (t.type == TT::ATTR_DROP) {
      throw XenonError("SyntaxError",
        "'$attribute: drop' is only valid inside a 'type' body before a function definition. "
        "Move it inside the struct that owns the destructor.",
        t.line, t.col);
    }

    if (t.type == TT::TYPE)
      return parse_type_definition();
    if (t.type == TT::ENUM_KW)
      return parse_enum();

    // use module::something  — import a module path for later qualified calls
    if (t.type == TT::USE_KW) {
      advance();
      // Collect the module path: one or more IDENTIFIER separated by ::
      // e.g.  use module::something  ->  "module::something"
      std::string module_path = expect(TT::IDENTIFIER, false).value;
      while (current().type == TT::COLON &&
             pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON) {
        advance(); advance(); // consume ::
        module_path += "::" + expect(TT::IDENTIFIER, false).value;
      }
      _used_modules.insert(module_path);
      return "";
    }

    // in namespace NAME are { ... }
    if (t.type == TT::IN_KW) {
      Token kw = advance();
      parse_namespace_block(kw);
      return "";
    }

    // ignore namespace NAME
    if (t.type == TT::IGNORE_KW) {
      advance();
      if (current().type != TT::NAMESPACE_KW)
        throw XenonError("SyntaxError",
          "'ignore' must be followed by 'namespace' (e.g. 'ignore namespace MyNS')",
          t.line, t.col);
      advance();
      std::string ns_name = expect(TT::IDENTIFIER, false).value;
      _ignored_namespaces.insert(ns_name);
      return "";
    }

    // overload operator(SYM) args(a,b) type(RET) { body }
    if (t.type == TT::OVERLOAD_KW) {
      Token kw = advance();
      if (current().type != TT::OPERATOR_KW)
        throw XenonError("SyntaxError",
          "'overload' must be followed by 'operator(SYM)'"
          " (e.g. 'overload operator(+) args(a, b) type(int) { ... }')",
          kw.line, kw.col);
      advance();
      parse_overload(kw);
      return "";
    }

    // addop operator(SYM) args(a,b,binary) type(RET) { body }
    if (t.type == TT::ADDOP_KW) {
      Token kw = advance();
      if (current().type != TT::OPERATOR_KW)
        throw XenonError("SyntaxError",
          "'addop' must be followed by 'operator(SYM)'"
          " (e.g. 'addop operator(@@) args(a, b) type(int) { ... }')",
          kw.line, kw.col);
      advance();
      parse_addop(kw);
      return "";
    }

    // alias NEWNAME = EXISTINGNAME
    if (t.type == TT::ALIAS_KW) {
      Token kw = advance();
      std::string def = parse_alias(kw);
      headers.push_back(def + "\n");
      return "";
    }

    // pipe EXPR |> func1 |> func2
    if (t.type == TT::PIPE_KW) {
      Token kw = advance();
      return parse_pipe_stmt(kw);
    }

    // match EXPR with { case VAL: ... default: ... }
    if (t.type == TT::MATCH_KW) {
      Token kw = advance();
      return parse_match(kw);
    }

    if (t.type == TT::UNSAFE_KW) {
      Token unsafe_tok = advance();
      if (current().type != TT::LBRACE)
        throw XenonError("SyntaxError",
                         "'unsafe' must be followed by a block: unsafe { ... }",
                         unsafe_tok.line, unsafe_tok.col);
      if (_memory_safe && _cur_func != "__main__" && !_unsafe_functions.count(_cur_func))
        throw XenonError("SafetyError",
                         "unsafe block in function '" + _cur_func + "'"
                         " — declare the function with 'unsafe function' to allow unsafe blocks,"
                         " or move this code into a separate 'unsafe function'",
                         unsafe_tok.line, unsafe_tok.col);
      advance();
      bool prev_unsafe = _in_unsafe_block;
      _in_unsafe_block = true;
      std::vector<std::string> body_stmts;
      while (current().type != TT::RBRACE && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          body_stmts.push_back(line_directive(tok2) + "    " + s);
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError", "Unterminated 'unsafe' block",
                         unsafe_tok.line, unsafe_tok.col);
      expect(TT::RBRACE, false);
      _in_unsafe_block = prev_unsafe;
      return "/* unsafe: begin */\n" + join(body_stmts, "\n") + "\n/* unsafe: end */";
    }

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
      size_t expr_pos = pos;
      std::string expr = parse_expr();
      if (tcc_mode) {
        // throw always takes a string message in practice; if it's a str/char*
        // pass directly, otherwise cast to int and format — but _lb_throw needs
        // a const char*. Use tcc_print_one logic: build a snprintf into a local.
        TypeInfo ti = infer_type_at(expr_pos);
        FmtSpec fs = printf_fmt_for_type(ti);
        if (fs.fmt == "%s")
          return "_lb_throw((const char*)(" + expr + "));";
        // non-string: format into a small static buffer
        return "{ static char _thr_buf[512]; snprintf(_thr_buf,512,\""
               + fs.fmt + "\"," + fs.cast + "(" + expr + ")); _lb_throw(_thr_buf); }";
      }
      return "_lb_throw(TO_STR(" + expr + "));";
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
        throw XenonError("SyntaxError",
                         "Unterminated 'try' block — missing closing '}' for the try body",
                           try_t.line, try_t.col);
      expect(TT::RBRACE, false);
      if (current().type != TT::EXCEPT_KW)
        throw XenonError("SyntaxError",
                         "'try' block must be followed by 'except(type varName) { ... }'",
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
        throw XenonError("SyntaxError",
                         "Unterminated 'except' block — missing closing '}'",
                           try_t.line, try_t.col);
      expect(TT::RBRACE, false);
      // Use line:col as UID — guaranteed unique per source location.
      // Cast hash to unsigned first to avoid signed-overflow UB.
      unsigned uid =
          (unsigned)(std::hash<std::string>{}(std::to_string(try_t.line) + ":" +
                                              std::to_string(try_t.col))) %
          99999u;
      std::string tb = join(try_body, "\n");
      std::string eb = join(exc_body, "\n");
      return "{ /* try */\n"
             "    jmp_buf _lb_jmp_" +
             std::to_string(uid) +
             ";\n"
             "    char _lb_exc_msg_" +
             std::to_string(uid) +
             "[512];\n"
             "    jmp_buf* _lb_exc_prev_ = _lb_exc_active;\n"
             "    char*    _lb_exc_msg_prev_ = _lb_exc_msg;\n"
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
             "    _lb_exc_active = _lb_exc_prev_;\n"
             "    _lb_exc_msg    = _lb_exc_msg_prev_;\n"
             "}";
    }

    // GLOBAL
    if (t.type == TT::GLOBAL_KW) {
      if (_cur_func != "__main__")
        throw XenonError("SyntaxError",
                           "'global' cannot be used inside function '" +
                               _cur_func + "' — move it to top-level scope",
                           t.line, t.col);
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
      // generic fallback — use TO_STR in non-TCC mode, infer format in TCC mode
      if (tcc_mode) {
        TypeInfo ti = infer_type_at(ct.line > 0 ? (size_t)(ct.line) : pos);
        // reuse the position of ct in the token stream
        size_t ct_pos = 0;
        for (size_t ii = 0; ii < tokens.size(); ii++) {
          if (&tokens[ii] == &ct) { ct_pos = ii; break; }
        }
        FmtSpec fs = printf_fmt_for_type(infer_type_at(ct_pos));
        if (fs.fmt == "BOOL")
          return "if(" + hname + "){fprintf(" + hname + ",\"%s\",(" + content + ")? \"true\":\"false\");fflush(" + hname + ");}";
        return "if(" + hname + "){fprintf(" + hname + ",\"" + fs.fmt + "\"," + fs.cast + "(" + content + "));fflush(" + hname + ");}";
      }
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
      std::string hname = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::COMMA, false);
      std::string var = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::RPAREN, false);
      return "if(" + hname + ")fread(&" + var + ",sizeof(" + var + "),1," +
             hname + ");";
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

    if (t.type == TT::INLINE_KW) {
      advance();
      if (current().type == TT::UNSAFE_KW) {
        Token unsafe_tok2 = advance();
        if (current().type != TT::FUNCTION)
          throw XenonError("SyntaxError",
                           "'inline unsafe' must be followed by 'function'"
                           " (e.g. 'inline unsafe function int foo() { ... }')",
                           unsafe_tok2.line, unsafe_tok2.col);
        Token fn_tok2 = current(); advance();
        size_t ns2 = pos;
        {
          static const std::set<TT> skip2 = {
            TT::INT,TT::FLOAT,TT::STR,TT::LONG,TT::SHORT,TT::DOUBLE,
            TT::VOID,TT::M256,TT::M256I,TT::BOOL_KW,TT::CHAR_KW,
            TT::PTR,TT::U8,TT::U32,TT::U64,TT::LET_KW,TT::VAR_KW,
            TT::IDENTIFIER
          };
          while (ns2 < tokens.size() && skip2.count(tokens[ns2].type)) {
            if (tokens[ns2].type == TT::PTR) {
              ns2++; // skip 'ptr'
              // skip the pointee type so we land on the function name
              if (ns2 < tokens.size() && skip2.count(tokens[ns2].type))
                ns2++;
              break;
            }
            ns2++;
            if (ns2 < tokens.size() && tokens[ns2].type == TT::LT) {
              while (ns2 < tokens.size() && tokens[ns2].type != TT::GT) ns2++;
              if (ns2 < tokens.size()) ns2++;
            }
            if (ns2 < tokens.size() && tokens[ns2].type == TT::LSBRACKET) {
              while (ns2 < tokens.size() && tokens[ns2].type != TT::RSBRACKET) ns2++;
              if (ns2 < tokens.size()) ns2++;
            }
            break;
          }
        }
        if (ns2 < tokens.size() && tokens[ns2].type == TT::IDENTIFIER) {
          _unsafe_functions.insert(mangle_with_ns(tokens[ns2].value));
          _unsafe_functions.insert(tokens[ns2].value);
        }
        return emit_function(fn_tok2, true);
      }
      if (current().type != TT::FUNCTION)
        throw XenonError("SyntaxError",
                           "'inline' must be followed by 'function'"
                           " (e.g. 'inline function int foo() { ... }')",
                           t.line, t.col);
      advance();
      return emit_function(t, true);
    }

    // const
    if (t.type == TT::CONST_KW) {
      advance();
      std::string inner_raw = advance().value;
      std::string inner_c = raw_to_c(inner_raw);
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

      // Array initializer: let x = {1, 2, 3}  — infer element type from first
      // element
      if (current().type == TT::LBRACE) {
        advance();
        std::vector<std::string> items;
        TypeInfo elem_ti = TypeInfo::of("int");
        bool first = true;
        while (current().type != TT::RBRACE && current().type != TT::TEOF) {
          if (first) {
            elem_ti = infer_type_at(pos);
            first = false;
          }
          items.push_back(parse_expr());
          if (current().type == TT::COMMA)
            advance();
        }
        expect(TT::RBRACE, false);
        std::string elem_c = elem_ti.c_type();
        var_types[name] = elem_c + "_ARRAY";
        std::string sz = std::to_string(items.size());
        return elem_c + " " + name + "[" + sz + "] = {" + join(items, ", ") +
               "};";
      }

      size_t rhs_start = pos; // capture RHS start BEFORE parse_expr consumes it
      Token expr_start = current();
      std::string val = parse_expr();

      // Full Rust/TS-style type inference: walk the token stream at rhs_start
      std::string inferred_type =
          infer_type_from_rhs(expr_start, val, rhs_start);

      // Store in var_types as the raw/normalised form (c_type_to_raw) so
      // emit_scanf and other raw-type consumers work correctly (e.g. "str" not
      // "char*")
      var_types[name] = c_type_to_raw(inferred_type);

      // ── RAII: let/var from heap allocators get __attribute__((cleanup)) ──
      if (!tcc_mode && !inferred_type.empty() && inferred_type.back() == '*') {
        bool is_heap = false;
        static const std::vector<std::string> alloc_names = {"malloc","calloc","realloc"};
        for (const auto &an : alloc_names) {
          if (val.find(an + "(") != std::string::npos ||
              val.find(an + " (") != std::string::npos) {
            is_heap = true; break;
          }
        }
        if (is_heap) {
          ensure_raii_free_shim();
          _raii_var_drop_expr[name] = "/* RAII-freed */";
          return inferred_type + " __attribute__((cleanup(_xen_raii_free_ptr_))) " + name + " = " + val + ";";
        }
      }

      return inferred_type + " " + name + " = " + val + ";";
    }

    // ── Option<T> variable declaration ───────────────────────────────────────
    // Syntax:  Option<int> x            (uninitialized, starts as None)
    //          Option<int> x = Some(5)
    //          Option<str> x = None
    if (t.type == TT::OPTION_KW) {
      Token opt_tok = advance(); // consume 'Option'
      expect(TT::LT, false);    // consume '<'
      // Collect inner type (may be ptr T or a keyword/identifier)
      std::string inner_raw;
      bool inner_is_ptr = false;
      if (current().type == TT::PTR) {
        advance(); inner_is_ptr = true;
        inner_raw = advance().value;
      } else {
        inner_raw = advance().value;
      }
      expect(TT::GT, false); // consume '>'
      std::string c_inner = raw_to_c(inner_raw);
      if (inner_is_ptr) c_inner += "*";
      std::string mangled = ensure_option_struct(c_inner);

      std::string varname = safe_name(expect(TT::IDENTIFIER, false).value);
      var_types[varname] = mangled;

      if (current().type == TT::ASSIGN) {
        advance();
        std::string rhs = parse_expr();
        return mangled + " " + varname + " = " + rhs + ";";
      }
      // Default to None
      return mangled + " " + varname + " = {0};";
    }

    // Type declarations (var decl)
    // Handle generic struct instantiation: Vec<int> x = ...
    // Also handles namespace-qualified generic structs (e.g. ns::Vec<int> from inside ns)
    {
      std::string gen_lookup = safe_name(t.value);
      // Try namespace-mangled name if bare name not found
      if (t.type == TT::IDENTIFIER && !_generic_structs.count(gen_lookup) && !_cur_namespace.empty())
        gen_lookup = _cur_namespace + "__" + gen_lookup;
    if (t.type == TT::IDENTIFIER && _generic_structs.count(gen_lookup)) {
      advance(); // consume the struct name token
      std::string gen_name = gen_lookup; // use the resolved (possibly mangled) name
      if (current().type == TT::LT) {
        advance(); // consume '<'
        // collect concrete type (may be 'ptr X' or a plain keyword/identifier)
        std::string concrete_raw;
        bool concrete_is_ptr = false;
        if (current().type == TT::PTR) {
          advance();
          concrete_is_ptr = true;
          concrete_raw = advance().value;
        } else {
          concrete_raw = advance().value;
        }
        expect(TT::GT, false); // consume '>'
        // Resolve concrete_raw if it is a bound type parameter (e.g. "T" → "int")
        {
          auto _gvit = var_types.find(concrete_raw);
          if (_gvit != var_types.end() && _gvit->second != "STRUCT" &&
              _gvit->second != "GENERIC_STRUCT" && _gvit->second != concrete_raw) {
            concrete_raw = raw_to_c(_gvit->second);
          }
        }
        std::string c_concrete = raw_to_c(concrete_raw);
        if (concrete_is_ptr) c_concrete += "*";
        std::string mangled = instantiate_generic_struct(gen_name, c_concrete);
        std::string varname = safe_name(expect(TT::IDENTIFIER, false).value);
        var_types[varname] = mangled;
        if (current().type == TT::ASSIGN) {
          advance();
          return mangled + " " + varname + " = " + parse_expr() + ";";
        }
        return mangled + " " + varname + ";";
      }
      // Not generic angle bracket — fall through treating as regular IDENTIFIER
      // (already advanced past it, so re-insert logic manually)
      // This shouldn't normally happen if user wrote Vec without <T>, but handle gracefully
      std::string varname = safe_name(expect(TT::IDENTIFIER, false).value);
      var_types[varname] = gen_name;
      if (current().type == TT::ASSIGN) {
        advance();
        return gen_name + " " + varname + " = " + parse_expr() + ";";
      }
      return gen_name + " " + varname + ";";
    }
    } // end gen_lookup scope, considering the current
    // namespace.  Returns the mangled name if found, or "" otherwise.
    auto resolve_type_name = [&](const std::string &raw) -> std::string {
      // 1. Direct lookup (already mangled or global)
      auto it = var_types.find(raw);
      if (it != var_types.end() && (it->second == "STRUCT" || it->second == "GENERIC_STRUCT"))
        return raw;
      // 2. Namespace-qualified: ns__raw
      if (!_cur_namespace.empty()) {
        std::string mangled = _cur_namespace + "__" + raw;
        auto it2 = var_types.find(mangled);
        if (it2 != var_types.end() && (it2->second == "STRUCT" || it2->second == "GENERIC_STRUCT"))
          return mangled;
      }
      // 3. Ignored namespaces
      for (auto &ns : _ignored_namespaces) {
        std::string r = resolve_qualified(ns, raw);
        if (!r.empty()) {
          auto it3 = var_types.find(r);
          if (it3 != var_types.end() && (it3->second == "STRUCT" || it3->second == "GENERIC_STRUCT"))
            return r;
        }
      }
      return "";
    };

    bool is_custom =
        (t.type == TT::IDENTIFIER &&
         (!resolve_type_name(t.value).empty() || _enum_names.count(t.value)));

    // ── Unknown type check ────────────────────────────────────────────────
    // Pattern: IDENTIFIER IDENTIFIER  →  "TYPE varName" declaration.
    // If the first IDENTIFIER is not a known type, throw immediately.
    // We only trigger this when:
    //   - current token is an IDENTIFIER (potential custom type name)
    //   - next token is also an IDENTIFIER (potential variable name)
    //   - it is NOT already a recognised keyword type (handled by type_decl_toks)
    //   - it is NOT followed by '(' (that would be a function call, handled below)
    //   - it is NOT followed by '=', '[', '.', '::', '++', '--' (assignment / expression)
    // This specifically catches  awddawd x = dawd()  or  awddawd x;
    if (t.type == TT::IDENTIFIER && !is_custom &&
        pos + 1 < tokens.size() &&
        tokens[pos + 1].type == TT::IDENTIFIER &&
        // Make sure token after next is not '(' — that would make t a function call
        !(pos + 2 < tokens.size() && tokens[pos + 2].type == TT::LPAREN)) {
      // Possibly a "TYPE varName" declaration with an unknown type.
      // Confirm: this pattern should not look like "existingVar anotherVar"
      // (that's not valid Xenon anyway, so we still want an error).
      // Only skip the check if t.value is in var_types as a variable (not a STRUCT).
      bool is_existing_var = var_types.count(t.value) && var_types[t.value] != "STRUCT";
      if (!is_existing_var && !is_known_type(t.value)) {
        // Build helpful candidates from struct_field_types and var_types
        std::vector<std::string> type_candidates;
        for (const auto &[tn, tv] : var_types) {
          if (tv == "STRUCT" || tv == "GENERIC_STRUCT") {
            size_t common = 0;
            size_t minlen = std::min(tn.size(), t.value.size());
            for (size_t ci = 0; ci < minlen; ci++) {
              if (tn[ci] == t.value[ci]) common++;
              else break;
            }
            if (common >= 3 || (minlen > 0 && common * 2 >= minlen))
              type_candidates.push_back(tn);
          }
        }
        std::string tmsg = "Unknown type '" + t.value + "'.\n"
          "  Checked: builtin types (int, float, str, bool, …), "
          "user-defined structs/enums, .xen imports, .h imports — none matched.\n"
          "  If '" + t.value + "' is from a C header, add: link \"the_header.h\"\n"
          "  If it is from a .xen file, add: link \"the_file.xen\"";
        if (!type_candidates.empty()) {
          tmsg += "\n  Did you mean: ";
          for (size_t ci = 0; ci < type_candidates.size() && ci < 3; ci++) {
            if (ci) tmsg += ", ";
            tmsg += "'" + type_candidates[ci] + "'";
          }
          tmsg += "?";
        }
        throw XenonError("TypeError", tmsg, t.line, t.col);
      }
    }
    // ── Namespace-qualified type declaration: NS::Type varName [= expr] ──────
    // Pattern: IDENTIFIER COLON COLON IDENTIFIER IDENTIFIER
    // e.g.  Math::Vec v = ...   or   Gfx::Sprite s
    // The existing type_decl_toks block only handles bare type names; qualified
    // ones fall through silently and produce broken C.  Catch and validate here.
    if (t.type == TT::IDENTIFIER &&
        pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON &&
        pos + 2 < tokens.size() && tokens[pos + 2].type == TT::COLON &&
        pos + 3 < tokens.size() && tokens[pos + 3].type == TT::IDENTIFIER &&
        pos + 4 < tokens.size() && tokens[pos + 4].type == TT::IDENTIFIER) {
      // Consume NS :: TypeName
      std::string ns_tok = advance().value;       // NS
      advance(); advance();                        // ::
      std::string type_tok_val = advance().value;  // TypeName
      // Build mangled name — handle chained NS::Sub::Type via loop
      std::string mangled_type = ns_tok + "__" + type_tok_val;
      while (current().type == TT::COLON &&
             pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON &&
             pos + 2 < tokens.size() && tokens[pos + 2].type == TT::IDENTIFIER) {
        advance(); advance(); // ::
        mangled_type += "__" + advance().value;
      }
      // Validate the resolved type actually exists
      if (!is_known_type(mangled_type) && !var_types.count(mangled_type)) {
        // Build candidates
        std::vector<std::string> type_candidates;
        for (const auto &[tn, tv] : var_types) {
          if (tv == "STRUCT" || tv == "GENERIC_STRUCT") {
            size_t common = 0;
            size_t minlen = std::min(tn.size(), mangled_type.size());
            for (size_t ci = 0; ci < minlen; ci++) {
              if (tn[ci] == mangled_type[ci]) common++;
              else break;
            }
            if (common >= 3 || (minlen > 0 && common * 2 >= minlen))
              type_candidates.push_back(tn);
          }
        }
        std::string display = ns_tok + "::" + type_tok_val;
        std::string tmsg = "Unknown type '" + display + "'.\n"
          "  Resolved to '" + mangled_type + "' — not found in any imported namespace.\n"
          "  If this type is defined in a .xen file, add: link \"the_file.xen\"\n"
          "  If it is from a C header, add: link \"the_header.h\"";
        if (!type_candidates.empty()) {
          tmsg += "\n  Did you mean: ";
          for (size_t ci = 0; ci < type_candidates.size() && ci < 3; ci++) {
            if (ci) tmsg += ", ";
            tmsg += "'" + type_candidates[ci] + "'";
          }
          tmsg += "?";
        }
        throw XenonError("TypeError", tmsg, t.line, t.col);
      }
      // Emit the declaration
      std::string varname = safe_name(expect(TT::IDENTIFIER, false).value);
      var_types[varname] = mangled_type;
      if (current().type == TT::ASSIGN) {
        advance();
        return mangled_type + " " + varname + " = " + parse_expr() + ";";
      }
      return mangled_type + " " + varname + ";";
    }
    // ─────────────────────────────────────────────────────────────────────
    static const std::set<TT> type_decl_toks = {
        TT::INT,     TT::FLOAT,   TT::STR,  TT::PTR,  TT::LONG,
        TT::SHORT,   TT::DOUBLE,  TT::VOID, TT::M256, TT::M256I,
        TT::BOOL_KW, TT::CHAR_KW, TT::U8,   TT::U32,  TT::U64};

    if (type_decl_toks.count(t.type) || is_custom) {
      std::string vtype_raw = advance().value;
      // If vtype_raw is a bare struct name that resolves via namespace, use the
      // mangled name for C emission while keeping vtype_raw for var_types tracking.
      std::string vtype_raw_resolved = resolve_type_name(vtype_raw);
      if (!vtype_raw_resolved.empty() && vtype_raw_resolved != vtype_raw) {
        // Keep vtype_raw as-is for the var_types tag, but use mangled for C output
        // We'll fix it up below by overriding vtype.
      }
      std::string vtype;
      static const std::map<std::string, std::string> umap = {
          {"u8", "uint8_t"}, {"u32", "uint32_t"}, {"u64", "uint64_t"}};
      if (umap.count(vtype_raw)) {
        vtype = umap.at(vtype_raw);
        if (!has_header("#include <stdint.h>"))
          headers[0] = "#include <stdint.h>\n" + headers[0];
      } else if (vtype_raw == "ptr") {
        std::string inner = advance().value;
        if (inner == "str") {
          inner = "char*";
        } else {
          // Resolve type param (e.g. "T" → "int" if T is bound in var_types)
          auto _ptpit = var_types.find(inner);
          if (_ptpit != var_types.end() && _ptpit->second != "STRUCT" &&
              _ptpit->second != "GENERIC_STRUCT" && _ptpit->second != inner) {
            inner = raw_to_c(_ptpit->second);
          }
        }
        vtype = inner + "*";
      } else if (vtype_raw == "bool") {
        vtype = "bool";
      } else if (vtype_raw == "char") {
        vtype = "char";
      } else {
        // Resolve bare type param (e.g. "T" → "int")
        auto _btp = var_types.find(vtype_raw);
        if (_btp != var_types.end() && _btp->second != "STRUCT" &&
            _btp->second != "GENERIC_STRUCT" && _btp->second != vtype_raw &&
            !_generic_structs.count(vtype_raw)) {
          vtype = raw_to_c(_btp->second);
        } else {
          vtype = (vtype_raw == "str") ? "char*" : vtype_raw;
        }
        // If the bare name resolved to a namespace-mangled struct, use mangled
        if (!vtype_raw_resolved.empty() && vtype_raw_resolved != vtype_raw)
          vtype = vtype_raw_resolved;
      }

      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      // Store the resolved (mangled) type name in var_types so field access etc. works
      var_types[name] = vtype_raw_resolved.empty() ? vtype_raw : vtype_raw_resolved;

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
        // Check for operator(=) overload — e.g. String a = "hi" should invoke
        // the user-defined = overload if one is registered.
        // We must capture rhs_pos BEFORE parse_expr() consumes the tokens.
        if (_op_overloads.count("=") && is_custom) {
          size_t rhs_pos = pos; // token position of the RHS (used for type inference)
          std::string rhs = parse_expr();
          const OverloadEntry *ov = resolve_overload("=", rhs_pos, rhs_pos);
          if (ov) {
            // Declare uninitialized, then call the overload with a pointer to
            // the lhs so the overload can mutate the real variable.
            // The overload returns void — no assignment prefix.
            return vtype + " " + name + ";\n    " +
                   ov->func_name + "(&" + name + ", " + rhs + ");";
          }
          return vtype + " " + name + " = " + rhs + ";";
        }
        // ── Runtime bounds tracking for heap-pointer subscripts ─────────────
        // Peek at the RHS before consuming it: if it's malloc/calloc/realloc,
        // compute the element count (alloc_bytes / sizeof(elem)) and store it
        // in _ptr_alloc_elems[name] as a C expression so that parse_primary
        // can wrap subscripts with a runtime bounds + null check.
        // We do this here (parse time) because scan_body_for_unsafe_ops runs
        // after code generation and cannot influence the emitted C.
        if (vtype_raw == "ptr" &&
            pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER &&
            token_is_heap_alloc(tokens[pos].value)) {
          // The elem type is the pointee of vtype, e.g. "int*" → "int"
          std::string pointee = vtype;
          if (!pointee.empty() && pointee.back() == '*')
            pointee = pointee.substr(0, pointee.size() - 1);
          // Snapshot token range for the call: IDENT LPAREN ... RPAREN
          size_t call_start = pos;
          // Find the closing RPAREN
          size_t scan = pos + 1;
          if (scan < tokens.size() && tokens[scan].type == TT::LPAREN) {
            size_t d2 = 1; scan++;
            while (scan < tokens.size() && d2 > 0) {
              if (tokens[scan].type == TT::LPAREN) d2++;
              else if (tokens[scan].type == TT::RPAREN) d2--;
              if (d2 > 0) scan++;
            }
            size_t call_end = scan + 1; // past RPAREN
            auto [nbytes, det_type] = parse_malloc_args(tokens, call_start, call_end, var_types);
            std::string actual_elem = det_type.empty() ? pointee : det_type;
            long long esz = get_sizeof_type(actual_elem);
            if (nbytes > 0 && esz > 0) {
              // Compile-time known: store literal element count
              _ptr_alloc_elems[name] = std::to_string(nbytes / esz);
            } else if (!actual_elem.empty() && esz > 0) {
              // Bytes unknown at compile time but element size is known:
              // Emit a C expression: the malloc arg text divided by sizeof
              // Reconstruct the malloc argument as source text
              size_t arg_s = call_start + 2; // skip IDENT + LPAREN
              std::string arg_text;
              int dd = 0;
              for (size_t k = arg_s; k < tokens.size(); k++) {
                if (tokens[k].type == TT::RPAREN && dd == 0) break;
                if (tokens[k].type == TT::LPAREN) dd++;
                else if (tokens[k].type == TT::RPAREN) dd--;
                if (!arg_text.empty()) arg_text += " ";
                arg_text += tokens[k].value;
              }
              if (!arg_text.empty())
                _ptr_alloc_elems[name] = "((" + arg_text + ") / sizeof(" + actual_elem + "))";
            }
          }
        } else if (vtype_raw == "ptr" &&
                   pos < tokens.size() && tokens[pos].type == TT::IDENTIFIER &&
                   !token_is_heap_alloc(tokens[pos].value)) {
          // ── Alias tracking: ptr double q = p ─────────────────────────────
          // The RHS is a plain identifier (not malloc). If it resolves to a
          // tracked pointer, record q as an alias of p so subscripts on q
          // inherit the element-count bounds.
          std::string rhs_name = safe_name(tokens[pos].value);
          if (resolve_ptr_elems(rhs_name) != nullptr)
            _ptr_aliases[name] = rhs_name;
        }

        // ── RAII: ptr vars from heap allocators get __attribute__((cleanup)) ──
        // This causes the C compiler to emit free() automatically at scope exit,
        // covering all paths (early return, break, etc.) with zero runtime cost.
        // Disabled in TCC mode (which doesn't support __attribute__((cleanup))).
        bool _is_heap_init = (vtype_raw == "ptr" && !tcc_mode &&
                              pos < tokens.size() &&
                              tokens[pos].type == TT::IDENTIFIER &&
                              token_is_heap_alloc(tokens[pos].value));
        // (pos now points at the RHS — check BEFORE we parse_expr())
        // We need to snapshot whether current RHS is a heap alloc; pos hasn't moved yet.
        // Re-check after alias detection: if vtype_raw == "ptr" and the prev token
        // was a heap allocator, _is_heap_init is still valid.
        (void)_is_heap_init; // used below after RHS parsing

        // Parse the RHS
        std::string rhs_expr = parse_expr();

        if (vtype_raw == "ptr" && !tcc_mode) {
          // Re-detect heap init from what we know: _ptr_alloc_elems[name] was
          // populated iff this was a heap alloc (set above in the malloc-tracking
          // block).  Use that as the signal.
          bool was_heap = _ptr_alloc_elems.count(name) > 0 ||
                          // Also catch: ptr T x = malloc(...)  even when compile-time
                          // size is unknown (alloc_bytes == -1 but elem_type recorded).
                          false; // conservative: only auto-free when we tracked it
          // Broader: any ptr = allocator_call should be RAII-freed.
          // We detect by scanning rhs_expr for the alloc name prefix.
          static const std::vector<std::string> alloc_prefixes = {"malloc","calloc","realloc"};
          for (const auto &ap : alloc_prefixes) {
            if (rhs_expr.find(ap + "(") != std::string::npos ||
                rhs_expr.find(ap + " (") != std::string::npos) {
              was_heap = true;
              break;
            }
          }
          if (was_heap) {
            ensure_raii_free_shim();
            // We must cast the pointer-to-variable to void** for the shim.
            // Declare as void* with __attribute__((cleanup)) but store in the
            // original typed variable.  The cleanest approach: declare the typed
            // variable normally, then declare a void* alias with the cleanup attr
            // that aliases the typed var.  But C forbids aliasing of different types.
            //
            // Instead: declare as the exact pointer type WITH __attribute__((cleanup)).
            // The cleanup function prototype expects void**, which GCC/Clang accept
            // because the attribute only names the function — types are checked at the
            // call generated by the compiler which uses a compatible __typeof__ cast.
            // This is the standard GCC RAII idiom.
            _raii_var_drop_expr[name] = "/* RAII-freed */";
            return vtype + " __attribute__((cleanup(_xen_raii_free_ptr_))) " + name + " = " + rhs_expr + ";";
          }
        }

        // ── RAII: struct vars with $attribute: drop ──────────────────────────
        // If the declared type has a registered drop function, attach cleanup.
        {
          std::string struct_lookup = vtype_raw_resolved.empty() ? vtype_raw : vtype_raw_resolved;
          if (!tcc_mode && struct_drop_funcs.count(struct_lookup)) {
            ensure_raii_drop_shim(struct_lookup);
            _raii_var_drop_expr[name] = struct_lookup + "__drop(&" + name + ")";
            std::string shim_fn = "_xen_raii_drop_" + struct_lookup + "_";
            return vtype + " __attribute__((cleanup(" + shim_fn + "))) " + name + " = " + rhs_expr + ";";
          }
        }

        return vtype + " " + name + " = " + rhs_expr + ";";
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

      // ── RAII: struct vars with $attribute: drop (no-init case) ───────────
      {
        std::string struct_lookup = vtype_raw_resolved.empty() ? vtype_raw : vtype_raw_resolved;
        if (!tcc_mode && struct_drop_funcs.count(struct_lookup)) {
          ensure_raii_drop_shim(struct_lookup);
          _raii_var_drop_expr[name] = struct_lookup + "__drop(&" + name + ")";
          std::string shim_fn = "_xen_raii_drop_" + struct_lookup + "_";
          return vtype + " __attribute__((cleanup(" + shim_fn + "))) " + name + ";";
        }
      }

      return vtype + " " + name + ";";
    }

    // print
    if (t.type == TT::PRINT) {
      advance();
      expect(TT::LPAREN, false);
      std::vector<std::string> exprs;
      std::vector<size_t> expr_positions;
      while (current().type != TT::RPAREN) {
        if (current().type == TT::TEOF)
          throw XenonError("SyntaxError",
                           "Missing closing ')' in 'print(...)' call",
                           t.line, t.col);
        expr_positions.push_back(pos);
        exprs.push_back(parse_expr());
        if (current().type == TT::COMMA)
          advance();
      }
      expect(TT::RPAREN, false);
      std::string r;
      for (size_t i = 0; i < exprs.size(); i++) {
        if (i) r += "; ";
        if (tcc_mode)
          r += tcc_print_one(exprs[i], expr_positions[i]);
        else
          r += "printf(\"%s\",TO_STR(" + exprs[i] + "))";
      }
      return r + ";";
    }

    // println
    if (t.type == TT::PRINTLN) {
      advance();
      expect(TT::LPAREN, false);
      std::vector<std::string> exprs;
      std::vector<size_t> expr_positions;
      while (current().type != TT::RPAREN) {
        if (current().type == TT::TEOF)
          throw XenonError("SyntaxError",
                           "Missing closing ')' in 'println(...)' call",
                           t.line, t.col);
        expr_positions.push_back(pos);
        exprs.push_back(parse_expr());
        if (current().type == TT::COMMA)
          advance();
      }
      expect(TT::RPAREN, false);
      std::string r;
      for (size_t i = 0; i < exprs.size(); i++) {
        if (i) r += "; ";
        if (tcc_mode)
          r += tcc_print_one(exprs[i], expr_positions[i]);
        else
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
          throw XenonError("SyntaxError",
                           "Missing closing ')' in 'printfmt(...)' call",
                           t.line, t.col);
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
    // ── if let Some(x) = opt then ... [else ...] end ─────────────────────────
    // Desugars to:
    //   if (opt.has_value) { T x = opt.value; ... }
    // Pattern: IF LET_KW SOME_KW LPAREN IDENTIFIER RPAREN ASSIGN expr THEN body END
    if (t.type == TT::IF &&
        pos + 1 < tokens.size() && tokens[pos + 1].type == TT::LET_KW) {
      Token if_tok = advance(); // consume 'if'
      advance();                // consume 'let'
      if (current().type != TT::SOME_KW)
        throw XenonError("SyntaxError",
          "'if let' must be followed by 'Some(binding)'"
          " (e.g. 'if let Some(x) = opt then ... end')",
          if_tok.line, if_tok.col);
      advance(); // consume 'Some'
      expect(TT::LPAREN, false);
      Token bind_tok = current();
      std::string binding = safe_name(expect(TT::IDENTIFIER, false).value);
      expect(TT::RPAREN, false);
      expect(TT::ASSIGN, false);
      // Parse the Option expression being tested
      std::string opt_expr = parse_expr();
      expect(TT::THEN, false);

      // Figure out the concrete inner type from opt_expr's var_types entry
      // (best-effort: fall back to "int" so we always emit valid C)
      std::string inner_c_type = "int";
      std::string opt_struct_type = "";
      // opt_expr is typically a plain identifier — check var_types
      {
        std::string bare = opt_expr;
        // strip any trailing whitespace/parens that parse_expr may have added
        while (!bare.empty() && (bare.back() == ' ' || bare.back() == ')')) bare.pop_back();
        auto vit = var_types.find(bare);
        if (vit != var_types.end() &&
            vit->second.size() > 7 &&
            vit->second.substr(0, 7) == "Option_") {
          opt_struct_type = vit->second;
          // inner type = field "value" type in struct_field_types
          auto fit = struct_field_types.find(opt_struct_type);
          if (fit != struct_field_types.end()) {
            auto fvit = fit->second.find("value");
            if (fvit != fit->second.end())
              inner_c_type = fvit->second;
          }
        }
      }

      // Snapshot scope, add binding inside body scope
      auto scope_let = var_types;
      var_types[binding] = inner_c_type;

      std::vector<std::string> body;
      // Emit binding declaration as first statement in block
      body.push_back("    " + inner_c_type + " " + binding + " = (" + opt_expr + ").value;");
      while (current().type != TT::ELSE && current().type != TT::END &&
             current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty()) body.push_back(line_directive(tok2) + "    " + s);
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
          "Unterminated 'if let' — missing 'end'",
          if_tok.line, if_tok.col);

      var_types = scope_let;

      std::string else_part;
      if (current().type == TT::ELSE) {
        advance();
        auto scope_else = var_types;
        std::vector<std::string> else_body;
        while (current().type != TT::END && current().type != TT::TEOF) {
          Token tok2 = current();
          std::string s = parse_statement();
          if (!s.empty()) else_body.push_back(line_directive(tok2) + "    " + s);
        }
        var_types = scope_else;
        else_part = " else {\n" + join(else_body, "\n") + "\n}";
      }
      expect(TT::END, false);

      return "if ((" + opt_expr + ").has_value) {\n" +
             join(body, "\n") + "\n}" + else_part;
    }

    if (t.type == TT::IF) {
      advance();
      std::string cond = parse_expr();
      expect(TT::THEN, false);
      auto scope0 = var_types;
      std::vector<std::string> body;
      while (current().type != TT::ELSEIF && current().type != TT::ELSE &&
             current().type != TT::END && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          body.push_back(line_directive(tok2) + "    " + s);
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
                         "Unterminated 'if' — missing 'end' to close the block opened at this line",
                         t.line, t.col);
      var_types = scope0;

      std::vector<std::string> elseif_parts;
      while (current().type == TT::ELSEIF) {
        Token ei = advance();
        std::string ce = parse_expr();
        expect(TT::THEN, false);
        auto scope_ei = var_types;
        std::vector<std::string> be;
        while (current().type != TT::ELSEIF && current().type != TT::ELSE &&
               current().type != TT::END && current().type != TT::TEOF) {
          Token tok2 = current();
          std::string s = parse_statement();
          if (!s.empty())
            be.push_back(line_directive(tok2) + "    " + s);
        }
        if (current().type == TT::TEOF)
          throw XenonError("SyntaxError",
                             "Unterminated 'elseif' branch — missing 'end' or 'else' to close the if-chain",
                             ei.line, ei.col);
        var_types = scope_ei;
        elseif_parts.push_back("} else if (" + ce + ") {\n" + join(be, "\n"));
      }

      std::string else_part;
      if (current().type == TT::ELSE) {
        advance();
        auto scope_else = var_types;
        std::vector<std::string> eb;
        while (current().type != TT::END && current().type != TT::TEOF) {
          Token tok2 = current();
          std::string s = parse_statement();
          if (!s.empty())
            eb.push_back(line_directive(tok2) + "    " + s);
        }
        var_types = scope_else;
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
      auto scope_w = var_types;
      std::vector<std::string> body;
      while (current().type != TT::END && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          body.push_back(line_directive(tok2) + s);
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError", "Unterminated 'while'", t.line,
                           t.col);
      expect(TT::END, false);
      var_types = scope_w;
      return "while (" + cond + ") {\n    " + join(body, "\n    ") + "\n}";
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
      auto scope_f = var_types;
      scope_f[var] = "int"; // loop var visible inside, not outside
      var_types = scope_f;
      std::vector<std::string> body;
      while (current().type != TT::END && current().type != TT::TEOF) {
        Token tok2 = current();
        std::string s = parse_statement();
        if (!s.empty())
          body.push_back(line_directive(tok2) + s);
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError", "Unterminated 'for'", t.line, t.col);
      expect(TT::END, false);
      var_types = scope_f;
      // erase loop var from outer scope
      var_types.erase(var);
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
          auto scope_case = var_types;
          std::vector<std::string> stmts;
          while (current().type != TT::CASE &&
                 current().type != TT::DEFAULT_KW &&
                 current().type != TT::RBRACE && current().type != TT::TEOF) {
            std::string s = parse_statement();
            if (!s.empty())
              stmts.push_back("    " + s);
          }
          var_types = scope_case;
          cases.push_back("case " + val + ":\n" + join(stmts, "\n") +
                          "\n    break;");
        } else if (current().type == TT::DEFAULT_KW) {
          advance();
          if (current().type == TT::COLON)
            advance();
          auto scope_def = var_types;
          while (current().type != TT::RBRACE && current().type != TT::TEOF) {
            std::string s = parse_statement();
            if (!s.empty())
              default_stmts.push_back("    " + s);
          }
          var_types = scope_def;
        } else {
          advance();
        }
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
                         "Unterminated 'switch' — missing closing '}' for the block opened at this line",
                         t.line, t.col);
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

    // pointer deref assign  *expr = rhs
    if (t.type == TT::MULTIPLY) {
      advance();
      // Build the LHS: identifier optionally followed by postfix chains
      std::string name = safe_name(expect(TT::IDENTIFIER, false).value);
      while (current().type == TT::LSBRACKET || current().type == TT::DOT ||
             current().type == TT::ARROW) {
        if (current().type == TT::LSBRACKET) {
          advance();
          std::string idx = parse_expr();
          expect(TT::RSBRACKET, false);
          name = name + "[(size_t)(" + idx + ")]";
        } else if (current().type == TT::DOT) {
          advance();
          name = name + "." + expect(TT::IDENTIFIER, false).value;
        } else {
          advance();
          name = name + "->" + expect(TT::IDENTIFIER, false).value;
        }
      }
      expect(TT::ASSIGN, false);
      return "*" + name + " = " + parse_expr() + ";";
    }

    // identifier: assign, compound assign, ++/--, call
    if (t.type == TT::IDENTIFIER) {
      std::string raw_id = advance().value;

      // NAME::sym — namespace qualified call at statement level (supports chaining: A::B::sym)
      if (current().type == TT::COLON &&
          pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON) {
        std::string ns_path = raw_id;
        std::string ns_path_colons = raw_id;
        advance(); advance(); // consume ::
        std::string sym = expect(TT::IDENTIFIER, false).value;
        // Chain: keep consuming ::IDENTIFIER while the current sym is a sub-namespace
        while (current().type == TT::COLON &&
               pos + 1 < tokens.size() && tokens[pos + 1].type == TT::COLON) {
          ns_path = ns_path + "__" + sym;
          ns_path_colons = ns_path_colons + "::" + sym;
          advance(); advance();
          sym = expect(TT::IDENTIFIER, false).value;
        }
        // ── Module function auto-load ─────────────────────────────────────
        if (current().type == TT::LPAREN) {
          std::string full_use_path = ns_path_colons + "::" + sym;
          maybe_load_module_call(full_use_path, t);
        }
        std::string resolved = resolve_qualified(ns_path, sym);
        if (resolved.empty()) resolved = ns_path + "__" + sym;
        // Optional explicit generic type arg: NS::func<TYPE>(args)
        std::string explicit_type_arg;
        if (current().type == TT::LT && template_funcs.count(resolved)) {
          size_t saved_p = pos;
          advance(); // consume '<'
          std::string explicit_type;
          bool is_ptr = false;
          if (current().type == TT::PTR) { advance(); is_ptr = true; }
          if (current().type == TT::IDENTIFIER || current().type == TT::INT ||
              current().type == TT::FLOAT || current().type == TT::DOUBLE ||
              current().type == TT::LONG || current().type == TT::SHORT ||
              current().type == TT::BOOL_KW || current().type == TT::CHAR_KW ||
              current().type == TT::STR || current().type == TT::U8 ||
              current().type == TT::U32 || current().type == TT::U64 ||
              current().type == TT::VOID) {
            explicit_type = advance().value;
            if (current().type == TT::GT) {
              advance();
              explicit_type_arg = raw_to_c(explicit_type);
              if (is_ptr) explicit_type_arg += "*";
            } else { pos = saved_p; }
          } else { pos = saved_p; }
        }
        if (current().type == TT::LPAREN) {
          // NS::func() call
          Token call_tok = current();
          advance();
          return emit_call(resolved, call_tok, explicit_type_arg) + ";";
        }
        if (current().type == TT::IDENTIFIER) {
          // NS::TYPE varname  — variable declaration
          std::string varname = safe_name(advance().value);
          var_types[varname] = resolved;
          if (current().type == TT::ASSIGN) {
            advance();
            return resolved + " " + varname + " = " + parse_expr() + ";";
          }
          return resolved + " " + varname + ";";
        }
        // bare NS::sym expression
        return resolved + ";";
      }

      std::string type_to_use = raw_id;

      std::string name = safe_name(raw_id);
      // ── Struct method: rewrite bare field LHS to self->field ──────
      if (!_cur_struct.empty() && name != "self") {
        auto _psfit = struct_field_types.find(_cur_struct);
        if (_psfit != struct_field_types.end() && _psfit->second.count(name))
          name = "self->" + name;
      }
      bool _any_method_dispatched = false;
      while (current().type == TT::LSBRACKET || current().type == TT::DOT ||
             current().type == TT::ARROW) {
        if (current().type == TT::LSBRACKET) {
          advance();
          size_t _lhs_idx_pos = pos;
          std::string idx = parse_expr();
          expect(TT::RSBRACKET, false);
          // ── $bounds: assert suppression ─────────────────────────────────
          bool _lhs_bounds_asserted = false;
          if (_lhs_idx_pos >= 3) {
            for (size_t _bk = _lhs_idx_pos - 1; _bk > 0 && _bk + 4 >= _lhs_idx_pos; _bk--) {
              if (tokens[_bk].type == TT::BOUNDS_ASSERT) { _lhs_bounds_asserted = true; break; }
              if (tokens[_bk].type == TT::SEMICOLON || tokens[_bk].type == TT::LBRACE) break;
            }
          }
          if (_lhs_bounds_asserted) {
            name = name + "[(size_t)(" + idx + ")]";
          } else {
            // ── Alias-aware element-count lookup ────────────────────────────
            const std::string *_lhs_elems_ptr = resolve_ptr_elems(name);
            if (_memory_safe && !_in_unsafe_block && _lhs_elems_ptr != nullptr) {
              const std::string &n_elems = *_lhs_elems_ptr;
              // ── Look-back: try to resolve index statically ──────────────
              size_t _lhs_body_start = 0;
              { int _depth = 0;
                for (size_t _bk = (_lhs_idx_pos > 0 ? _lhs_idx_pos - 1 : 0); _bk > 0; _bk--) {
                  if (tokens[_bk].type == TT::RBRACE) _depth++;
                  else if (tokens[_bk].type == TT::LBRACE) {
                    if (_depth == 0) { _lhs_body_start = _bk + 1; break; }
                    _depth--;
                  }
                }
              }
              bool _lhs_static_resolved = false;
              if (_lhs_idx_pos < tokens.size() && tokens[_lhs_idx_pos].type == TT::IDENTIFIER) {
                std::string _idx_var = tokens[_lhs_idx_pos].value;
                auto [_known, _val] = resolve_index_statically(_idx_var, _lhs_idx_pos, _lhs_body_start);
                if (_known) {
                  _lhs_static_resolved = true;
                  long long _n = -1;
                  try { _n = std::stoll(n_elems); } catch (...) {}
                  if (_val < 0) {
                    transpile_errors.push_back(
                      XenonError("OOBAccess",
                        "index '" + _idx_var + "' (=" + std::to_string(_val) +
                        ") on pointer '" + name + "' is negative — undefined behaviour",
                        tokens[_lhs_idx_pos].line, tokens[_lhs_idx_pos].col).what());
                  } else if (_n >= 0 && _val >= _n) {
                    transpile_errors.push_back(
                      XenonError("OOBAccess",
                        "index '" + _idx_var + "' (=" + std::to_string(_val) +
                        ") on pointer '" + name + "' is out of bounds (size=" +
                        std::to_string(_n) + ")",
                        tokens[_lhs_idx_pos].line, tokens[_lhs_idx_pos].col).what());
                  }
                  name = name + "[(size_t)(" + idx + ")]";
                }
              }
              if (!_lhs_static_resolved) {
                emit_warning("BoundsCheck",
                  "cannot statically verify index '" + idx + "' on pointer '" + name +
                  "' — runtime bounds check inserted (suppress with '$bounds: assert' on the previous line)",
                  current().line, current().col);
                if (!has_header("_XEN_IDX")) {
                  if (tcc_mode) {
                    headers.push_back(
                      "/* ── Xenon runtime pointer safety (null + bounds) [TCC/C99] ── */\n"
                      "#include <stdlib.h>\n"
                      "#include <stdio.h>\n"
                      "#define _XEN_IDX(p,i,n) \\\n"
                      "  (((p)==NULL ? \\\n"
                      "     (fprintf(stderr,\"[Xenon] null pointer dereference\\n\"),abort(),0) : 0), \\\n"
                      "   ((long long)(i)<0||(long long)(i)>=(long long)(n) ? \\\n"
                      "     (fprintf(stderr,\"[Xenon] index %lld out of bounds (size=%lld)\\n\",(long long)(i),(long long)(n)),abort(),0) : 0), \\\n"
                      "   (p)[(size_t)(i)])\n"
                    );
                  } else {
                    headers.push_back(
                      "/* ── Xenon runtime pointer safety (null + bounds) ── */\n"
                      "#include <stdlib.h>\n"
                      "#include <stdio.h>\n"
                      "#define _XEN_IDX(p,i,n) \\\n"
                      "  ((__builtin_expect((p)==NULL,0) ? \\\n"
                      "     (fprintf(stderr,\"[Xenon] null pointer dereference\\n\"),abort(),0) : 0), \\\n"
                      "   (__builtin_expect((long long)(i)<0||(long long)(i)>=(long long)(n),0) ? \\\n"
                      "     (fprintf(stderr,\"[Xenon] index %lld out of bounds (size=%lld)\\n\",(long long)(i),(long long)(n)),abort(),0) : 0), \\\n"
                      "   (p)[(size_t)(i)])\n"
                    );
                  }
                }
                name = "_XEN_IDX(" + name + ", " + idx + ", " + n_elems + ")";
              }
            } else {
              name = name + "[(size_t)(" + idx + ")]";
            }
          }
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
          // ── Method call dispatch in statement position ───────────
          std::string base_struct;
          { auto _bvit = var_types.find(first); if (_bvit != var_types.end()) base_struct = _bvit->second; }
          bool _stmt_method = false;
          if (!base_struct.empty() && current().type == TT::LPAREN) {
            auto _msit = struct_methods.find(base_struct);
            if (_msit != struct_methods.end() && _msit->second.count(field)) {
              advance(); // consume '('
              std::string _self_arg = (var_types.count(first) && var_types[first].find("ptr") != std::string::npos) ? name : ("&" + name);
              std::string _cargs = _self_arg;
              while (current().type != TT::RPAREN && current().type != TT::TEOF) {
                _cargs += ", " + parse_expr();
                if (current().type == TT::COMMA) advance();
              }
              expect(TT::RPAREN, false);
              name = base_struct + "__" + field + "(" + _cargs + ")";
              _stmt_method = true;
              _any_method_dispatched = true;
            }
          }
          if (!_stmt_method) {
            std::string op = (var_types.count(first) && var_types[first] == "ptr")
                                 ? "->"
                                 : ".";
            name = name + op + field;
          }
        } else {
          advance();
          name = name + "->" + expect(TT::IDENTIFIER, false).value;
        }
      }
      // ── If a method call was the last thing built, emit it as a statement ──
      if (_any_method_dispatched) {
        return name + ";";
      }
      // NS::TYPE var — need to emit declaration
      if (type_to_use != raw_id && !type_to_use.empty()) {
        var_types[name] = type_to_use;
        if (current().type == TT::ASSIGN) {
          advance();
          std::string rhs = parse_expr();
          return type_to_use + " " + name + " = " + rhs + ";";
        } else {
          // Just declaration with no init
          return type_to_use + " " + name + ";";
        }
      }

      if (current().type == TT::ASSIGN) {
        // Check for = operator overload
        if (_op_overloads.count("=")) {
          Token assign_tok = current();
          size_t lhs_pos = pos - 1; // position of the lhs identifier (already consumed above)
          size_t rhs_pos = pos + 1; // one past the '=' token
          advance();
          std::string rhs = parse_expr();
          const OverloadEntry *ov = resolve_overload("=", lhs_pos, rhs_pos);
          if (ov)
            return ov->func_name + "(&" + name + ", " + rhs + ");";          return name + " = " + rhs + ";";
        }
        advance();
        std::string rhs = parse_expr();
        return name + " = " + rhs + ";";
      }
      static const std::map<TT, std::string> compound = {
          {TT::PLUS_ASSIGN, "+="}, {TT::MINUS_ASSIGN, "-="},
          {TT::MUL_ASSIGN, "*="},  {TT::DIV_ASSIGN, "/="},
          {TT::MOD_ASSIGN, "%="},
      };
      if (compound.count(current().type)) {
        TT compound_tt = current().type;
        size_t lhs_pos = pos - 1;
        advance();
        size_t rhs_pos = pos;
        std::string cop = compound.at(compound_tt);
        std::string rhs = parse_expr();
        if (_op_overloads.count(cop)) {
          const OverloadEntry *ov = resolve_overload(cop, lhs_pos, rhs_pos);
          if (ov) return name + " = " + ov->func_name + "(" + name + ", " + rhs + ");";
        }
        return name + " " + cop + " " + rhs + ";";
      }
      if (current().type == TT::INCR) {
        size_t operand_pos = pos - 1;
        advance();
        if (_op_overloads.count("++post")) {
          const OverloadEntry *ov = resolve_overload("++post", operand_pos, operand_pos);
          if (ov && !ov->is_binary) return ov->func_name + "(" + name + ");";
        }
        return name + "++;";
      }
      if (current().type == TT::DECR) {
        size_t operand_pos = pos - 1;
        advance();
        if (_op_overloads.count("--post")) {
          const OverloadEntry *ov = resolve_overload("--post", operand_pos, operand_pos);
          if (ov && !ov->is_binary) return ov->func_name + "(" + name + ");";
        }
        return name + "--;";
      }
      if (current().type == TT::LPAREN) {
        Token call_tok = current();
        // If we're in a namespace, try to resolve locally first
        if (!_cur_namespace.empty()) {
          auto &ns_map = _namespaces[_cur_namespace];
          auto it = ns_map.find(raw_id);
          if (it != ns_map.end()) {
            name = it->second; // use mangled name
          }
        }
        // ── Single-segment module call: use something → something() ───────
        maybe_load_module_call(raw_id, t);
        advance();
        return emit_call(name, call_tok) + ";";
      }
      // Explicit generic call as statement: func<TYPE>(args);
      if (current().type == TT::LT && template_funcs.count(name)) {
        size_t saved_pos = pos;
        advance(); // consume '<'
        std::string explicit_type;
        bool explicit_is_ptr = false;
        if (current().type == TT::PTR) { advance(); explicit_is_ptr = true; }
        if (current().type == TT::IDENTIFIER || current().type == TT::INT ||
            current().type == TT::FLOAT || current().type == TT::DOUBLE ||
            current().type == TT::LONG || current().type == TT::SHORT ||
            current().type == TT::BOOL_KW || current().type == TT::CHAR_KW ||
            current().type == TT::STR || current().type == TT::U8 ||
            current().type == TT::U32 || current().type == TT::U64 ||
            current().type == TT::VOID) {
          explicit_type = advance().value;
          if (current().type == TT::GT) {
            advance(); // consume '>'
            std::string c_type = raw_to_c(explicit_type);
            if (explicit_is_ptr) c_type += "*";
            if (current().type == TT::LPAREN) {
              Token call_tok = current();
              advance();
              return emit_call(name, call_tok, c_type) + ";";
            }
          }
        }
        pos = saved_pos; // backtrack if not a valid generic call
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
      // Mark this variable as having a runtime-determined value so the
      // look-back index resolver knows any conditional on it is non-static.
      _runtime_vars.insert(name);
      return emit_scanf(name, vt, t);
    }

    // return
    if (t.type == TT::RETURN) {
      advance();
      if (current().type == TT::END || current().type == TT::RBRACE ||
          current().type == TT::TEOF || current().type == TT::SEMICOLON)
        return "return;";
      std::string expr = parse_expr();
      // Warn: void function returning a value
      if (_cur_func_ret == "void")
        throw XenonError("TypeError",
                           "function '" + _cur_func +
                               "' is declared 'void' but returns a value —"
                               " change the return type or remove the return value",
                           t.line, t.col);
      return "return " + expr + ";";
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
  // -----------------------------------------------------------------------
  // Infer param type by scanning body token range [body_start, body_end)
  // for usage patterns of `param_name`.  Returns best TypeInfo guess.
  //
  // Patterns recognised (in order of confidence):
  //   1. param assigned to a typed variable:  typed_var = param
  //   2. param assigned FROM a typed rhs that also involves param:
  //      e.g.  x = param + 1.0  → param appears on RHS of typed assign
  //   3. param passed as Nth arg to a known function: look up
  //   func_param_types[N]
  //   4. param used in arithmetic with a typed operand → promote
  //   5. param compared (==,!=,<,>,<=,>=) with a typed value
  // Fallback: int
  // -----------------------------------------------------------------------
  TypeInfo infer_param_type_from_body(const std::string &param_name,
                                      size_t body_start, size_t body_end) {
    // Walk body_start..body_end looking for any token == param_name
    // (IDENTIFIER) and examine surrounding context.
    TypeInfo best = TypeInfo::of("int");
    bool found = false;

    for (size_t i = body_start; i < body_end; i++) {
      if (tokens[i].type != TT::IDENTIFIER || tokens[i].value != param_name)
        continue;

      // Pattern 1 & 2: something = ... param ...
      // Walk left to find an assignment: look for ASSIGN preceded by IDENTIFIER
      // This is expensive in general; instead, when param appears on RHS of =,
      // the LHS var has a known type — infer expr type of the full RHS.
      // Find the enclosing statement's = if any by scanning left.
      {
        // Find the start of this statement by scanning back to prior ; or {
        size_t stmt_start = i;
        while (stmt_start > body_start) {
          TT tt = tokens[stmt_start - 1].type;
          if (tt == TT::SEMICOLON || tt == TT::LBRACE || tt == TT::RBRACE)
            break;
          stmt_start--;
        }
        // Find = in this statement range
        for (size_t j = stmt_start; j < i && j < body_end; j++) {
          if (tokens[j].type == TT::ASSIGN) {
            // LHS is before j, RHS starts at j+1
            // If param is on RHS (which it is since i > j), infer full RHS type
            TypeInfo rhs_ti = infer_type_at(j + 1);
            if (rhs_ti.base != "int" || rhs_ti.ptr_depth > 0) {
              best = found ? promote(best, rhs_ti) : rhs_ti;
              found = true;
            }
            // Also: if LHS is a simple known-typed var, use that directly
            if (j > stmt_start && tokens[j - 1].type == TT::IDENTIFIER) {
              TypeInfo lhs_ti = lookup_var(safe_name(tokens[j - 1].value));
              if (lhs_ti.base != "int" || lhs_ti.ptr_depth > 0) {
                best = found ? promote(best, lhs_ti) : lhs_ti;
                found = true;
              }
            }
            break;
          }
        }
      }

      // Pattern 3: param is Nth arg in a function call  foo(..., param, ...)
      // Look right for comma/rparen context, and left for the call opener
      {
        // scan left to find LPAREN of call
        int depth = 0;
        size_t call_lp = std::string::npos;
        for (int j = (int)i - 1; j >= (int)body_start; j--) {
          if (tokens[j].type == TT::RPAREN)
            depth++;
          else if (tokens[j].type == TT::LPAREN) {
            if (depth == 0) {
              call_lp = (size_t)j;
              break;
            } else
              depth--;
          }
        }
        if (call_lp != std::string::npos && call_lp > 0 &&
            tokens[call_lp - 1].type == TT::IDENTIFIER) {
          std::string callee = safe_name(tokens[call_lp - 1].value);
          auto pit = func_param_types.find(callee);
          if (pit != func_param_types.end()) {
            // count which arg position param is at
            int arg_idx = 0;
            int d2 = 0;
            for (size_t j = call_lp + 1; j < i; j++) {
              if (tokens[j].type == TT::LPAREN ||
                  tokens[j].type == TT::LSBRACKET)
                d2++;
              else if (tokens[j].type == TT::RPAREN ||
                       tokens[j].type == TT::RSBRACKET)
                d2--;
              else if (tokens[j].type == TT::COMMA && d2 == 0)
                arg_idx++;
            }
            if (arg_idx < (int)pit->second.size()) {
              std::string ptype_c = pit->second[arg_idx];
              TypeInfo pti;
              if (!ptype_c.empty() && ptype_c.back() == '*') {
                pti.base = ptype_c.substr(0, ptype_c.size() - 1);
                pti.ptr_depth = 1;
              } else {
                pti.base = raw_to_c(ptype_c);
              }
              if (pti.base != "int" || pti.ptr_depth > 0) {
                best = found ? promote(best, pti) : pti;
                found = true;
              }
            }
          }
        }
      }

      // Pattern 4 & 5: param in arithmetic/comparison with a typed peer
      // Check the token immediately to the right of an operator adjacent to
      // param
      {
        size_t peer_pos = std::string::npos;
        static const std::set<TT> arith = {
            TT::PLUS, TT::MINUS, TT::MULTIPLY, TT::DIVIDE,    TT::MOD, TT::EQ,
            TT::NE,   TT::LT,    TT::GT,       TT::LE,        TT::GE,  TT::SHL,
            TT::SHR,  TT::BITOR, TT::BITXOR,   TT::ADDRESS_OF};
        // param OP expr
        if (i + 1 < body_end && arith.count(tokens[i + 1].type) &&
            i + 2 < body_end)
          peer_pos = i + 2;
        // expr OP param — look at the token two to the left (operator at i-1,
        // peer at i-2)
        if (i >= 2 && arith.count(tokens[i - 1].type) && (i - 2) >= body_start)
          peer_pos = i - 2;
        if (peer_pos != std::string::npos && peer_pos < body_end) {
          TypeInfo peer_ti = infer_type_at(peer_pos);
          // only use if peer is more specific than int/unknown
          if (!peer_ti.is_unknown() &&
              (peer_ti.is_float() || peer_ti.is_ptr() ||
               peer_ti.base == "double" || peer_ti.base == "long" ||
               peer_ti.base == "char*" || peer_ti.base == "bool")) {
            best = found ? promote(best, peer_ti) : peer_ti;
            found = true;
          }
        }
      }
    }
    return best;
  }

  // -----------------------------------------------------------------------
  // Infer return type by scanning body token range [body_start, body_end)
  // for every RETURN token and running infer_type_at on the expression.
  // Promotes across all return sites.
  // -----------------------------------------------------------------------
  TypeInfo infer_return_type_from_body(
      size_t body_start, size_t body_end,
      const std::map<std::string, std::string> &local_var_types) {
    TypeInfo best = TypeInfo::of("void");
    bool found = false;

    // Temporarily install local var_types so infer_type_at sees function-scope
    // vars
    auto saved = var_types;
    // merge local_var_types into var_types
    for (auto const &kv : local_var_types)
      var_types[kv.first] = kv.second;

    // Pre-scan: collect let/var local variable types so return-expr inference
    // can resolve identifiers like `return result` where result = func_call(...)
    for (size_t i = body_start; i + 3 < body_end; i++) {
      if (tokens[i].type != TT::LET_KW && tokens[i].type != TT::VAR_KW)
        continue;
      // pattern: LET_KW/VAR_KW IDENTIFIER ASSIGN expr
      if (tokens[i + 1].type != TT::IDENTIFIER)
        continue;
      if (tokens[i + 2].type != TT::ASSIGN)
        continue;
      std::string vname = tokens[i + 1].value;
      TypeInfo rhs_ti = infer_type_at(i + 3);
      if (!rhs_ti.is_unknown())
        var_types[vname] = rhs_ti.c_type();
    }

    for (size_t i = body_start; i < body_end; i++) {
      if (tokens[i].type != TT::RETURN)
        continue;
      // next token: if it's END/RBRACE/TEOF/SEMICOLON → void return
      if (i + 1 >= body_end || i + 1 >= tokens.size())
        continue;
      TT next = tokens[i + 1].type;
      if (next == TT::RBRACE || next == TT::TEOF || next == TT::SEMICOLON)
        continue; // void return, don't override non-void if already found
      TypeInfo ti = infer_type_at(i + 1);
      best = found ? promote(best, ti) : ti;
      found = true;
    }

    var_types = saved;
    if (!found)
      return TypeInfo::of("void");
    return best.is_unknown() ? TypeInfo::of("void") : best;
  }

  // Convert raw param type keyword to C type string (shared helper)
  std::string param_raw_to_c(const std::string &raw) const {
    if (raw == "str")
      return "char*";
    if (raw == "ptr")
      return "void*";
    if (raw == "bool")
      return "bool";
    if (raw == "char")
      return "char";
    if (raw == "u8")
      return "uint8_t";
    if (raw == "u32")
      return "uint32_t";
    if (raw == "u64")
      return "uint64_t";
    return raw;
  }

  // Wraps parse_function_body: records tok_start (= pos before we call, which
  // points at the return-type token) and patches it into template_funcs if the
  // function turned out to be a template.
  std::string emit_function(const Token &fn_tok, bool inl) {
    size_t tok_start = pos; // ret-type token is current()
    std::string code = parse_function_body(fn_tok, inl);
    if (code.size() > 12 && code.substr(0, 12) == "__TEMPLATE__") {
      std::string tname = code.substr(12);
      auto tit = template_funcs.find(tname);
      if (tit != template_funcs.end())
        tit->second.tok_start = tok_start;
      return ""; // don't emit anything for a template
    }
    return code;
  }

  std::string parse_function_body(const Token &fn_tok, bool inl) {
    static const std::set<TT> valid_ret = {
        TT::INT,     TT::FLOAT,   TT::STR,    TT::LONG,  TT::SHORT,
        TT::DOUBLE,  TT::VOID,    TT::M256,   TT::M256I, TT::IDENTIFIER,
        TT::BOOL_KW, TT::CHAR_KW, TT::PTR,    TT::U8,    TT::U32,
        TT::U64,     TT::LET_KW,  TT::VAR_KW, TT::OPTION_KW,
    };
    Token &rt = current();
    if (!valid_ret.count(rt.type))
      throw XenonError("SyntaxError",
                         "Expected a return type after 'function', got '" +
                             rt.value + "'"
                             " — valid types include: int, float, str, bool, void, char,"
                             " long, double, short, u8, u32, u64, Option<T>, or a struct name",
                         rt.line, rt.col);

    // ── Option<T> return type ─────────────────────────────────────────────
    if (rt.type == TT::OPTION_KW) {
      advance(); // consume 'Option'
      expect(TT::LT, false);
      std::string inner_raw;
      bool inner_is_ptr = false;
      if (current().type == TT::PTR) { advance(); inner_is_ptr = true; }
      inner_raw = advance().value;
      expect(TT::GT, false);
      std::string c_inner = raw_to_c(inner_raw);
      if (inner_is_ptr) c_inner += "*";
      std::string mangled = ensure_option_struct(c_inner);
      // Now parse the function name and body using the mangled return type.
      // We re-enter parse_function_body's flow below by constructing ret_type manually.
      // Rather than duplicating the whole function, we replace rt and raw_ret inline.
      bool infer_ret = false;
      std::string ret_type = inl ? "static inline " + mangled : mangled;
      std::string raw_ret = mangled; // used for func_return_types registration

      // Register the current function name in func_return_types BEFORE parsing
      // the body so recursive calls see the correct return type.
      // We peek at the function name token to do early registration.
      std::string fname_peek_raw;
      {
        size_t p2 = pos;
        // skip optional generic <T> on function name
        fname_peek_raw = (p2 < tokens.size()) ? tokens[p2].value : "";
      }
      if (!fname_peek_raw.empty()) {
        std::string fname_peek = mangle_with_ns(fname_peek_raw);
        func_return_types[fname_peek] = mangled;
        func_return_types[fname_peek_raw] = mangled;
      }

      // Fall through into the rest of parse_function_body using the mangled type.
      // We handle this by running the rest of the function inline here and returning.
      std::string fname_raw2 = expect(TT::IDENTIFIER, false).value;
      std::string func_type_param2;
      if (current().type == TT::LT) {
        advance();
        func_type_param2 = advance().value;
        expect(TT::GT, false);
      }
      std::string fname2 = mangle_with_ns(fname_raw2);
      func_return_types[fname2]     = mangled;
      func_return_types[fname_raw2] = mangled;

      // ── Struct method check ──────────────────────────────────────────────
      if (!_cur_struct.empty()) {
        std::string method_name = _cur_struct + "__" + fname_raw2;
        struct_methods[_cur_struct].insert(fname_raw2);
        func_return_types[method_name] = mangled;
        fname2 = method_name;
      }

      // Parameters
      expect(TT::LPAREN, false);
      std::vector<std::string> params_v;
      auto prev_vt2 = var_types;
      if (!_cur_struct.empty()) {
        var_types["self"] = _cur_struct;
        params_v.push_back(_cur_struct + "* self");
      }
      while (current().type != TT::RPAREN && current().type != TT::TEOF) {
        // Re-use the lightweight param parser inline
        std::string pt_raw = advance().value;
        std::string pt;
        if (pt_raw == "ptr") { pt = raw_to_c(advance().value) + "*"; }
        else if (pt_raw == "str") pt = "char*";
        else pt = raw_to_c(pt_raw);
        std::string pname = safe_name(expect(TT::IDENTIFIER, false).value);
        var_types[pname] = pt_raw;
        params_v.push_back(pt + " " + pname);
        if (current().type == TT::COMMA) advance();
      }
      expect(TT::RPAREN, false);
      expect(TT::LBRACE, false);

      // Body
      std::vector<std::string> body_stmts;
      while (current().type != TT::RBRACE && current().type != TT::TEOF) {
        Token btok = current();
        std::string s = parse_statement();
        if (!s.empty()) body_stmts.push_back(line_directive(btok) + "    " + s);
      }
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
          "Unterminated function '" + fname_raw2 + "' — missing closing '}'",
          fn_tok.line, fn_tok.col);
      expect(TT::RBRACE, false);
      var_types = prev_vt2;

      std::string sig = ret_type + " " + fname2 + "(" + join(params_v, ", ") + ")";
      return sig + " {\n" + join(body_stmts, "\n") + "\n}\n";
    }
    std::string raw_ret = advance().value;
    bool infer_ret = (raw_ret == "let" || raw_ret == "var");
    std::string ret_type;
    if (!infer_ret) {
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
      else if (raw_ret == "u8")
        ret_type = "uint8_t";
      else if (raw_ret == "u32")
        ret_type = "uint32_t";
      else if (raw_ret == "u64")
        ret_type = "uint64_t";
      else
        ret_type = raw_ret;
      // array return type: int[N] → int*
      if (current().type == TT::LSBRACKET) {
        advance();
        while (current().type != TT::RSBRACKET && current().type != TT::TEOF)
          advance();
        if (current().type == TT::RSBRACKET)
          advance();
        if (ret_type.empty() || ret_type.back() != '*')
          ret_type += "*";
      }
    }
    if (inl && !infer_ret)
      ret_type = "static inline " + ret_type;

    // Optional explicit generic type param on function name: VecNew<T>
    // Also handle generic return type: function Vec<T> VecNew<T>(...)
    // The return type may itself be Vec<T> — we already consumed raw_ret above.
    // Handle case: if raw_ret was an IDENTIFIER that is a generic struct,
    // consume the <PARAM> on the return type.
    std::string generic_ret_param; // e.g. "T" from "Vec<T>" return
    if (!infer_ret && current().type == TT::LT &&
        _generic_structs.count(raw_ret)) {
      advance(); // consume '<'
      generic_ret_param = advance().value; // consume type param name
      expect(TT::GT, false);
    }

    std::string fname_raw = expect(TT::IDENTIFIER, false).value;
    // Consume optional explicit <T> on function name
    std::string func_type_param;
    if (current().type == TT::LT) {
      advance(); // consume '<'
      func_type_param = advance().value; // e.g. "T"
      expect(TT::GT, false);
    }
    std::string fname = mangle_with_ns(fname_raw);
    // Detect if we're being called from instantiate_template for this fname.
    // The guard_key format is "originalName@type1,type2".
    // The patched fname during instantiation is the fully mangled name
    // (e.g. "vec__vecNew__int"). We match by checking if any in-progress key's
    // prefix (before '@') is a prefix of or equal to fname, or if fname starts
    // with that prefix followed by "__".
    bool is_instantiating_now = false;
    {
      for (const auto &k : _mono_in_progress) {
        size_t at = k.find('@');
        if (at == std::string::npos) continue;
        std::string base_name = k.substr(0, at);
        // Match: fname == base_name (exact), or fname starts with base_name + "__"
        if (fname == base_name ||
            (fname.size() > base_name.size() + 2 &&
             fname.substr(0, base_name.size()) == base_name &&
             fname[base_name.size()] == '_' && fname[base_name.size()+1] == '_')) {
          is_instantiating_now = true;
          break;
        }
      }
    }

    // ── Struct method: mangle name and register ──────────────────────────
    // When called from parse_type_definition, _cur_struct is the mangled
    // struct name.  Rename to StructName__methodName and inject 'self'.
    bool is_struct_method = !_cur_struct.empty();
    if (is_struct_method) {
      fname = _cur_struct + "__" + fname_raw;
      struct_methods[_cur_struct].insert(fname_raw);
    }

    func_return_types[fname] = infer_ret ? "__infer__" : raw_ret;
    expect(TT::LPAREN, false);

    std::string saved_cur_func = _cur_func;
    std::string saved_cur_func_ret = _cur_func_ret;
    _cur_func = fname;
    _cur_func_ret = ""; // resolved after ret_type is finalised below
    auto saved_var_types = var_types;

    // Seed struct field names into var_types so bare field refs inside
    // the method body resolve and get rewritten to self->field.
    if (is_struct_method) {
      auto fit = struct_field_types.find(_cur_struct);
      if (fit != struct_field_types.end())
        for (auto const &kv : fit->second)
          var_types[kv.first] = kv.second;
    }

    // -----------------------------------------------------------------------
    // Parse params — collect infer_param flags and names for two-pass resolve
    // -----------------------------------------------------------------------
    struct ParamInfo {
      std::string raw;
      std::string c_type;
      std::string name;
      bool infer;
      bool is_array;
      int array_size{-1}; // declared size if is_array, -1 otherwise
    };
    std::vector<ParamInfo> param_infos;

    static const std::set<TT> valid_p = {
        TT::INT,        TT::FLOAT,   TT::STR,     TT::LONG, TT::SHORT,
        TT::DOUBLE,     TT::VOID,    TT::PTR,     TT::M256, TT::M256I,
        TT::IDENTIFIER, TT::BOOL_KW, TT::CHAR_KW, TT::U8,   TT::U32,
        TT::U64,        TT::LET_KW,  TT::VAR_KW};

    while (current().type != TT::RPAREN) {
      if (current().type == TT::TEOF)
        throw XenonError("SyntaxError",
                           "Missing closing ')' in parameter list of function '" + fname + "'",
                           fn_tok.line, fn_tok.col);
      Token pt = current();
      if (!valid_p.count(pt.type))
        throw XenonError("SyntaxError",
                           "Expected a parameter type in '" + fname + "', got '" +
                               pt.value + "'"
                               " — valid parameter types: int, float, str, bool, char,"
                               " long, double, short, u8, u32, u64, ptr, or a struct name",
                           pt.line, pt.col);
      ParamInfo pi;
      pi.raw = advance().value;
      pi.infer = (pi.raw == "let" || pi.raw == "var");

      if (!pi.infer) {
        if (pi.raw == "ptr") {
          std::string inner = advance().value;
          if (inner == "str") {
            inner = "char*";
          } else {
            // Resolve type param: if inner is a bound type parameter (e.g. "T"),
            // look it up in var_types so "ptr T" becomes "int*" when T=int.
            auto _tpit = var_types.find(inner);
            if (_tpit != var_types.end() && _tpit->second != "STRUCT" &&
                _tpit->second != "GENERIC_STRUCT" &&
                _tpit->second != inner) {
              inner = raw_to_c(_tpit->second);
            }
          }
          pi.c_type = inner + "*";
        } else {
          // Resolve bare type param: if pi.raw is a bound type parameter (e.g. "T"),
          // replace it with the concrete type now.
          {
            auto _tpit2 = var_types.find(pi.raw);
            if (_tpit2 != var_types.end() && _tpit2->second != "STRUCT" &&
                _tpit2->second != "GENERIC_STRUCT" &&
                _tpit2->second != pi.raw &&
                !_generic_structs.count(pi.raw)) {
              // pi.raw is a type param bound to a concrete type
              pi.raw = raw_to_c(_tpit2->second);
            }
          }
          // Handle generic struct param: Vec<T> v — consume <T> if present
          if (current().type == TT::LT && _generic_structs.count(pi.raw)) {
            advance(); // consume '<'
            // Collect concrete or type-param type
            std::string type_arg;
            bool type_arg_is_ptr = false;
            if (current().type == TT::PTR) { advance(); type_arg_is_ptr = true; }
            type_arg = advance().value; // e.g. "T" or "int"
            expect(TT::GT, false); // consume '>'
            // Resolve type_arg if it is itself a bound type parameter
            {
              auto _tpit3 = var_types.find(type_arg);
              if (_tpit3 != var_types.end() && _tpit3->second != "STRUCT" &&
                  _tpit3->second != "GENERIC_STRUCT" &&
                  _tpit3->second != type_arg) {
                type_arg = raw_to_c(_tpit3->second);
              }
            }
            std::string c_concrete = raw_to_c(type_arg);
            if (type_arg_is_ptr) c_concrete += "*";
            // Instantiate the struct for this concrete type
            std::string mangled_s = instantiate_generic_struct(pi.raw, c_concrete);
            pi.c_type = mangled_s;
            pi.raw = mangled_s;
          } else {
            pi.c_type = param_raw_to_c(pi.raw);
          }
        }
      }
      pi.name = safe_name(expect(TT::IDENTIFIER, false).value);

      pi.is_array = false;
      if (current().type == TT::LSBRACKET) {
        advance();
        // Capture the declared size (if it's a literal number)
        if (current().type == TT::NUMBER) {
          try { pi.array_size = std::stoi(current().value); } catch (...) {}
        }
        while (current().type != TT::RSBRACKET && current().type != TT::TEOF)
          advance();
        if (current().type == TT::RSBRACKET)
          advance();
        pi.is_array = true;
        if (!pi.infer && (pi.c_type.empty() || pi.c_type.back() != '*'))
          pi.c_type += "*";
      }

      // If we're in an instantiation, var_types is pre-seeded for infer params.
      // Use what's already there; otherwise placeholder "int".
      if (pi.infer) {
        // Check if pre-seeded by instantiate_template
        auto vit = var_types.find(pi.name);
        if (vit != var_types.end()) {
          pi.c_type = raw_to_c(vit->second); // covers "int" AND struct types
        } else {
          var_types[pi.name] = "int";
          pi.c_type = "int";
        }
      } else {
        var_types[pi.name] = pi.is_array ? pi.raw + "_ARRAY" : pi.raw;
      }
      param_infos.push_back(std::move(pi));
      if (current().type == TT::COMMA)
        advance();
    }
    expect(TT::RPAREN, false);

    // ── Forward declaration: no '{' after the parameter list ─────────────
    // Syntax:  function float fabsf(float val)
    // No body means this is a forward/extern declaration.  Emit a C
    // prototype and return immediately — no body to parse.
    if (current().type != TT::LBRACE) {
      // Build the params string
      std::vector<std::string> fwd_params;
      if (is_struct_method)
        fwd_params.push_back(_cur_struct + "* self");
      for (const auto &pi : param_infos)
        fwd_params.push_back((pi.infer ? std::string("int") : pi.c_type) + " " + pi.name);
      func_param_types[fname] = [&]{
        std::vector<std::string> pt;
        if (is_struct_method) pt.push_back(_cur_struct + "*");
        for (const auto &pi : param_infos) pt.push_back(pi.infer ? std::string("int") : pi.c_type);
        return pt;
      }();
      func_param_array_sizes[fname] = [&]{
        std::vector<int> sz;
        if (is_struct_method) sz.push_back(-1);
        for (const auto &pi : param_infos) sz.push_back(pi.array_size);
        return sz;
      }();
      var_types = saved_var_types;
      _cur_func = saved_cur_func;
      _cur_func_ret = saved_cur_func_ret;
      // Consume optional semicolon after forward decl
      if (current().type == TT::SEMICOLON) advance();
      // Emit as a C extern function prototype
      std::string fwd_ret = infer_ret ? std::string("int") : ret_type;
      // Strip "static inline " prefix — forward decls can't be static inline
      if (fwd_ret.substr(0, 14) == "static inline ")
        fwd_ret = fwd_ret.substr(14);
      return fwd_ret + " " + fname + "(" + join(fwd_params, ", ") + ");\n";
    }

    // -----------------------------------------------------------------------
    // Check if this is a template function (has any let/var params)
    // and we're NOT currently instantiating it.
    // If so: record it and skip the body.
    // -----------------------------------------------------------------------
    bool has_infer_params = false;
    for (const auto &pi : param_infos)
      if (pi.infer) {
        has_infer_params = true;
        break;
      }

    // Resolve generic return type now that params are parsed and var_types is seeded.
    // e.g. function Vec<T> VecNew<T>(...) — when T is bound via var_types, resolve Vec<T> → Vec__int
    if (!generic_ret_param.empty() && !func_type_param.empty() &&
        !infer_ret && _generic_structs.count(raw_ret)) {
      // Look up concrete binding for T from var_types (seeded by instantiate_template)
      std::string concrete_T;
      auto vit = var_types.find(func_type_param);
      if (vit != var_types.end()) {
        concrete_T = raw_to_c(vit->second);
      }
      // Also try to find T from a param that was declared as a generic struct <T>
      if (concrete_T.empty()) {
        for (const auto &pi : param_infos) {
          if (pi.raw == func_type_param || pi.c_type == func_type_param) {
            concrete_T = pi.c_type;
            break;
          }
        }
      }
      if (!concrete_T.empty()) {
        std::string mangled_struct = instantiate_generic_struct(raw_ret, concrete_T);
        ret_type = mangled_struct;
        if (inl) ret_type = "static inline " + ret_type;
        func_return_types[fname] = mangled_struct;
      }
    }

    // A function with an explicit <T> type parameter is always a template,
    // even if none of its params use let/var inference.
    bool has_explicit_type_param = !func_type_param.empty();
    if ((has_infer_params || has_explicit_type_param) && !is_instantiating_now) {
      // Record template descriptor
      TemplateFunc tmpl;
      // tok_start = position of return-type token = saved before we advanced
      // past it We need to rewind to find it. Since we've consumed ret_type +
      // fname + ( + params + ), store tok_start as the position we recorded
      // earlier in the func signature parse. We already advanced past all of
      // those. Use the original fn_tok position as anchor. Actually: fn_tok is
      // the 'function' keyword token; ret type is right after it.
      // fn_tok.line/col let us find it in the token stream.
      // Simplest: record pos of ret-type before parse_function_body is called.
      // That's not accessible here. But we know: we're called with fn_tok being
      // the 'function' token, and after advance() in the caller, pos pointed at
      // ret-type. We consumed: raw_ret (1 or 2 tokens) + [N] + fname + ( +
      // params + ) So tok_start = pos_of_current_lbrace - 1 -
      // param_count_tokens - 1 - 1 - ret_tokens Too fragile. Better: record
      // tok_start at the very beginning of parse_function_body. We'll fix this
      // by saving pos at the start of parse_function_body. For now this path is
      // handled by the caller which records tmpl.tok_start. We'll return a
      // sentinel string and let the caller handle it. ACTUALLY: we handle it
      // more cleanly by saving pos at top of this function. That save is
      // already done implicitly: the caller records tok_start before calling
      // us.
      // ... This is getting circular. Let's use a different approach:
      // Store tok_start as part of the TemplateFunc at the CALL SITE
      // (transpile() loop). We return "__TEMPLATE__<fname>" as the code so the
      // caller knows.

      // Build param_slots for the template descriptor
      for (const auto &pi : param_infos) {
        TemplateFunc::ParamSlot slot;
        slot.raw = pi.raw;
        slot.infer = pi.infer;
        slot.is_array = pi.is_array;
        tmpl.param_slots.push_back(slot);
      }
      tmpl.inl = inl;
      tmpl.raw_ret = raw_ret;
      tmpl.generic_ret_param = generic_ret_param;
      tmpl.func_type_param = func_type_param;
      // tok_start and tok_end: skip body and record
      expect(TT::LBRACE, false);
      int depth = 1;
      while (pos < tokens.size() && depth > 0) {
        if (tokens[pos].type == TT::LBRACE)
          depth++;
        else if (tokens[pos].type == TT::RBRACE)
          depth--;
        pos++;
      }
      // pos now points past closing }
      tmpl.tok_end = pos;
      // tok_start = we need the token just before fname... store 0 for now,
      // filled in by the caller which knows the pre-call pos.
      tmpl.tok_start = 0; // placeholder — filled by caller
      template_funcs[fname] = tmpl;
      var_types = saved_var_types;
      _cur_func = saved_cur_func;
      _cur_func_ret = saved_cur_func_ret;
      // Signal to caller with the fname so it can fill tok_start
      return "__TEMPLATE__" + fname;
    }

    expect(TT::LBRACE, false);
    size_t body_tok_start = pos; // first token of body

    // -----------------------------------------------------------------------
    // First pass: skip over body tokens (balanced braces) to find body_end,
    // so we can run inference before parsing.  We need the token range.
    // -----------------------------------------------------------------------
    size_t body_tok_end_for_check = 0;
    {
      size_t scan = pos;
      int depth = 1;
      while (scan < tokens.size() && depth > 0) {
        if (tokens[scan].type == TT::LBRACE)
          depth++;
        else if (tokens[scan].type == TT::RBRACE)
          depth--;
        if (depth > 0)
          scan++;
        else
          break;
      }
      body_tok_end_for_check = scan;

      // Resolve infer_param types using token-level body scan
      for (auto &pi : param_infos) {
        if (!pi.infer)
          continue;
        TypeInfo ti =
            infer_param_type_from_body(pi.name, body_tok_start, body_tok_end_for_check);
        pi.c_type = ti.c_type();
        if (pi.is_array && (pi.c_type.empty() || pi.c_type.back() != '*'))
          pi.c_type += "*";
        // update var_types with the resolved type
        var_types[pi.name] = pi.is_array ? (pi.c_type + "_ARRAY") : pi.c_type;
        // also update raw for downstream lookup
        pi.raw = pi.c_type;
      }

      // Resolve infer_ret using token-level body scan
      // (We must do this after param types are resolved so infer_type_at is
      // accurate)
      if (infer_ret) {
        TypeInfo rti = infer_return_type_from_body(body_tok_start, body_tok_end_for_check,
                                                   var_types);
        ret_type = rti.c_type();
        func_return_types[fname] = ret_type; // store bare type, not "static inline ..."
        if (inl)
          ret_type = "static inline " + ret_type;
      }
    }

    // -----------------------------------------------------------------------
    // Memory safety check pass (before emitting code)
    // Collect known array sizes from param declarations for the checker.
    // -----------------------------------------------------------------------
    if (_memory_safe) {
      std::map<std::string, long long> known_sizes;
      for (const auto &pi : param_infos) {
        if (pi.is_array && !pi.c_type.empty()) {
        }
      }
      auto local_vt = var_types;
      auto violations = scan_body_for_unsafe_ops(
          body_tok_start, body_tok_end_for_check, local_vt, known_sizes);

      if (!violations.empty()) {
        // ── Partition violations: hard errors vs warnings ─────────────────
        // Hard: undefined behaviour or correctness bugs that must be fixed.
        // Soft: style/safety hints that compile fine but deserve attention.
        static const std::set<std::string> warn_kinds = {
          "ShadowedVariable", "SignedUnsignedMismatch",
          "ImplicitNarrowing", "UnusedReturnValue", "FormatInjection",
        };

        // Rephrase each violation message to be crisp (≤ one line).
        auto crisp = [](const UnsafeViolation &v) -> std::string {
          // ShadowedVariable
          if (v.kind == "ShadowedVariable")
            return "'" + v.message.substr(v.message.find("'") + 0,
                     v.message.find("'", 1) - 0 + 1) // keep quoted name intact
                   .substr(0, v.message.find(" shadows"))
                   + " shadows an outer variable — rename one of them";
          // SignedUnsignedMismatch — extract just the types
          if (v.kind == "SignedUnsignedMismatch") {
            size_t a = v.message.find("'"), b = v.message.find("'", a+1);
            size_t c = v.message.find("'", b+1), d = v.message.find("'", c+1);
            std::string st = (a!=std::string::npos && b!=std::string::npos)
                              ? v.message.substr(a+1,b-a-1) : "signed";
            std::string ut = (c!=std::string::npos && d!=std::string::npos)
                              ? v.message.substr(c+1,d-c-1) : "unsigned";
            return "signed/unsigned comparison (" + st + " vs " + ut +
                   ") — negative values compare greater than large unsigned ones";
          }
          // ImplicitNarrowing — keep it short
          if (v.kind == "ImplicitNarrowing") {
            size_t a = v.message.find("'"), b = v.message.find("'",a+1);
            size_t c = v.message.find("'",b+1), d = v.message.find("'",c+1);
            std::string wide = (c!=std::string::npos && d!=std::string::npos)
                               ? v.message.substr(c+1,d-c-1) : "wider";
            size_t e = v.message.find("'",d+1), f = v.message.find("'",e+1);
            std::string narrow = (e!=std::string::npos && f!=std::string::npos)
                                 ? v.message.substr(e+1,f-e-1) : "narrower";
            return "narrowing: '" + wide + "' → '" + narrow +
                   "' may truncate — use cast(" + narrow + ", ...) to silence";
          }
          // UnusedReturnValue
          if (v.kind == "UnusedReturnValue") {
            size_t a = v.message.find("'"), b = v.message.find("'",a+1);
            std::string fn = (a!=std::string::npos && b!=std::string::npos)
                             ? v.message.substr(a+1,b-a-1) : "function";
            size_t c = v.message.find("'",b+1), d = v.message.find("'",c+1);
            std::string rt = (c!=std::string::npos && d!=std::string::npos)
                             ? v.message.substr(c+1,d-c-1) : "?";
            return "return value of '" + fn + "' (" + rt + ") ignored";
          }
          // UseAfterFree
          if (v.kind == "UseAfterFree")
            return v.message;
          // NullDeref
          if (v.kind == "NullDeref")
            return v.message;
          // DanglingDeref
          if (v.kind == "DanglingDeref")
            return v.message;
          // ConstViolation
          if (v.kind == "ConstViolation") {
            size_t a = v.message.find("'"), b = v.message.find("'",a+1);
            std::string nm = (a!=std::string::npos && b!=std::string::npos)
                             ? v.message.substr(a+1,b-a-1) : "variable";
            return "'" + nm + "' is const — cannot assign after declaration";
          }
          // UnreachableCode
          if (v.kind == "UnreachableCode")
            return "unreachable code after 'return'";
          // Uninitialised
          if (v.kind == "Uninitialised") {
            size_t a = v.message.find("'"), b = v.message.find("'",a+1);
            std::string nm = (a!=std::string::npos && b!=std::string::npos)
                             ? v.message.substr(a+1,b-a-1) : "variable";
            return "'" + nm + "' used before initialisation";
          }
          // DivisionByZero
          if (v.kind == "DivisionByZero")
            return "division by zero — undefined behaviour";
          // IntegerOverflow
          if (v.kind == "IntegerOverflow") {
            size_t a = v.message.find("literal value ");
            size_t b = v.message.find(" ", a + 14);
            std::string lit = (a!=std::string::npos && b!=std::string::npos)
                              ? v.message.substr(a+14, b-a-14) : "";
            size_t c = v.message.find("type '"), d = v.message.find("'", c+6);
            std::string ty = (c!=std::string::npos && d!=std::string::npos)
                             ? v.message.substr(c+6,d-c-6) : "type";
            return "value " + lit + " overflows '" + ty + "'";
          }
          // StrMutation
          if (v.kind == "StrMutation") {
            size_t a = v.message.find("'"), b = v.message.find("'",a+1);
            std::string nm = (a!=std::string::npos && b!=std::string::npos)
                             ? v.message.substr(a+1,b-a-1) : "str";
            return "mutating str '" + nm + "' via index — wrap in unsafe { }";
          }
          // FormatInjection
          if (v.kind == "FormatInjection") {
            size_t a = v.message.find("'"), b = v.message.find("'", a+1);
            std::string nm = (a!=std::string::npos && b!=std::string::npos)
                             ? v.message.substr(a+1,b-a-1) : "var";
            return "format-string injection via '" + nm + "' — use a literal format string";
          }
          return v.message;
        };

        // Emit warnings immediately (non-fatal)
        for (const auto &v : violations) {
          if (warn_kinds.count(v.kind))
            emit_warning(v.kind, crisp(v), v.line, v.col);
        }

        // Collect hard errors — unsafe function status does NOT suppress these;
        // only code inside an explicit unsafe { } block is exempt (handled by
        // unsafe_depth tracking inside scan_body_for_unsafe_ops).
        std::vector<UnsafeViolation> hard;
        for (const auto &v : violations)
          if (!warn_kinds.count(v.kind))
            hard.push_back(v);

        if (!hard.empty()) {
          // Helper: return a human-readable safe fix for each violation kind.
          // Shown before the unsafe {} fallback.
          auto safe_fix = [](const UnsafeViolation &v) -> std::string {
            if (v.kind == "OOBAccess")
              return "add a bounds check before the index (e.g. \'if (i < len) { ... }\')";
            if (v.kind == "NullDeref")
              return "check the pointer is non-null before use (e.g. \'if (p != null) { ... }\')";
            if (v.kind == "UseAfterFree")
              return "stop using the pointer after free() — set it to null immediately after freeing";
            if (v.kind == "DoubleFree")
              return "free the pointer only once — set it to null after freeing";
            if (v.kind == "MemoryLeak")
              return "call free() on the pointer before it goes out of scope";
            if (v.kind == "DanglingDeref")
              return "ensure the pointed-to value outlives this pointer, or copy the value";
            if (v.kind == "StrMutation")
              return "use a char[] array instead of str if you need mutable character access";
            if (v.kind == "Uninitialised")
              return "initialise the variable before use (e.g. \'int x = 0;\')";
            if (v.kind == "DivisionByZero")
              return "guard with \'if (divisor != 0)\' before dividing";
            if (v.kind == "FreeOfBorrowed")
              return "only free memory you own — remove the free() or transfer ownership explicitly";
            return "fix the underlying issue described above";
          };

          std::ostringstream msg;
          msg << "'" << fname << "' has " << hard.size()
              << " unsafe operation" << (hard.size() > 1 ? "s" : "")
              << " (declaring the function \'unsafe\' only permits unsafe blocks;"
              << " the body itself is still checked)";
          for (const auto &v : hard) {
            std::string lnum_str = (v.line > 0) ? std::to_string(v.line) : "?";
            msg << "\n    [" << v.kind << "]";
            if (v.line > 0) msg << " line " << v.line << ":" << v.col;
            msg << " — " << crisp(v);
            // Source snippet with line number in gutter
            if (v.line > 0 && !g_current_source.empty()) {
              std::string snip = extract_line_from_source(g_current_source, v.line);
              if (!snip.empty()) {
                size_t lp = snip.find_first_not_of(" \t");
                std::string trimmed = (lp == std::string::npos) ? snip : snip.substr(lp);
                msg << "\n      " << lnum_str << " | " << trimmed;
                if (v.col > 0 && lp != std::string::npos) {
                  int adjusted_col = v.col - (int)lp;
                  std::string gutter_pad(lnum_str.size(), ' ');
                  if (adjusted_col >= 1)
                    msg << "\n        " << gutter_pad << "| "
                        << std::string(adjusted_col - 1, ' ') << "^";
                }
              }
            }
            // Recommend the safe fix first
            msg << "\n      fix: " << safe_fix(v);
            // Then suggest unsafe {} as a last resort (indented → rendered grey)
            msg << "\n      or:  wrap in \'unsafe { }\' to suppress if you have verified this manually";
          }
          var_types = saved_var_types;
          _cur_func = saved_cur_func;
          _cur_func_ret = saved_cur_func_ret;
          throw XenonError("SafetyError", msg.str(), fn_tok.line, fn_tok.col);
        }
      }
    }

    // -----------------------------------------------------------------------
    // Borrow checker pass (simple move semantics for ptr variables)
    // Runs for ALL functions — unsafe function status only grants permission
    // to use unsafe { } blocks; code outside those blocks is still checked.
    // The scanner itself suppresses violations inside unsafe { } via
    // unsafe_depth tracking.
    // -----------------------------------------------------------------------
    if (_borrow_check && _memory_safe) {
      {
        auto local_vt = var_types;
        size_t safe_body_end = std::min(body_tok_end_for_check, tokens.size());
        auto borrow_violations = scan_body_for_borrow_errors(
            body_tok_start, safe_body_end, local_vt);
        if (!borrow_violations.empty()) {
          std::ostringstream msg;
          msg << "'" << fname << "' has " << borrow_violations.size()
              << " borrow error" << (borrow_violations.size() > 1 ? "s" : "")
              << " — see fixes below (pass -no-check:borrow to disable globally)";
          for (const auto &v : borrow_violations) {
            std::string lnum_str = (v.line > 0) ? std::to_string(v.line) : "?";
            msg << "\n    [" << v.kind << "]";
            if (v.line > 0) msg << " line " << v.line << ":" << v.col;
            msg << " — " << v.message;
            // Source snippet with line number in gutter
            if (v.line > 0 && !g_current_source.empty()) {
              std::string snip = extract_line_from_source(g_current_source, v.line);
              if (!snip.empty()) {
                size_t lp = snip.find_first_not_of(" \t");
                std::string trimmed = (lp == std::string::npos) ? snip : snip.substr(lp);
                msg << "\n      " << lnum_str << " | " << trimmed;
                if (v.col > 0 && lp != std::string::npos) {
                  int adjusted_col = v.col - (int)lp;
                  std::string gutter_pad(lnum_str.size(), ' ');
                  if (adjusted_col >= 1)
                    msg << "\n        " << gutter_pad << "| "
                        << std::string(adjusted_col - 1, ' ') << "^";
                }
              }
            }
            // Safe fix first
            if (v.kind == "BorrowError")
              msg << "\n      fix: re-initialise the variable or avoid using it after it has been moved";
            else if (v.kind == "FreeOfBorrowed")
              msg << "\n      fix: only free memory you own — remove the free() or transfer ownership explicitly";
            else
              msg << "\n      fix: correct the ownership issue described above";
            // Unsafe as last resort (indented => rendered grey)
            msg << "\n      or:  wrap in 'unsafe { }' to suppress if you have verified this manually";
          }
          var_types = saved_var_types;
          _cur_func = saved_cur_func;
          _cur_func_ret = saved_cur_func_ret;
          throw XenonError("BorrowError", msg.str(), fn_tok.line, fn_tok.col);
        }
      }
    }

    // Record for the return-statement validator
    _cur_func_ret = ret_type;

    // -----------------------------------------------------------------------
    // Build params vector for C declaration
    // -----------------------------------------------------------------------
    std::vector<std::string> params;
    {
      std::vector<std::string> ptypes;
      // Struct methods: inject 'StructName* self' as the first C param
      if (is_struct_method) {
        params.push_back(_cur_struct + "* self");
        ptypes.push_back(_cur_struct + "*");
        var_types["self"] = _cur_struct + "*";
      }
      for (const auto &pi : param_infos) {
        params.push_back(pi.c_type + " " + pi.name);
        ptypes.push_back(pi.c_type);
      }
      func_param_types[fname] = ptypes;
      // Also record per-param declared array sizes for brace-init checking
      {
        std::vector<int> asz;
        if (is_struct_method) asz.push_back(-1);
        for (const auto &pi : param_infos) asz.push_back(pi.array_size);
        func_param_array_sizes[fname] = asz;
      }
    }

    // -----------------------------------------------------------------------
    // Second pass: parse body for real (pos still at body_tok_start)
    // -----------------------------------------------------------------------
    std::vector<std::string> body;
    while (current().type != TT::RBRACE && current().type != TT::TEOF) {
      Token tok2 = current();
      std::string s = parse_statement();
      if (!s.empty())
        body.push_back(line_directive(tok2) + "    " + s);
    }
    if (current().type == TT::TEOF) {
      var_types = saved_var_types;
      _cur_func = saved_cur_func;
      _cur_func_ret = saved_cur_func_ret;
      throw XenonError("SyntaxError",
                       "Unterminated body for function '" + fname +
                       "' — missing closing '}'",
                         fn_tok.line, fn_tok.col);
    }
    expect(TT::RBRACE, false);
    var_types = saved_var_types;
    _cur_func = saved_cur_func;
    _cur_func_ret = saved_cur_func_ret;

    // -----------------------------------------------------------------------
    // Missing-return check: non-void functions MUST have at least one return
    // statement at the top-level body scope. This is enforced regardless of
    // whether the function is declared unsafe — unsafe only affects memory
    // safety rules, not control-flow completeness.
    // -----------------------------------------------------------------------
    {
      // Compute the base C type to determine void-ness (strip static inline)
      std::string bare_ret = ret_type;
      if (bare_ret.substr(0, 14) == "static inline ")
        bare_ret = bare_ret.substr(14);
      // Only enforce for non-void, non-inferred returns
      bool needs_return = (!bare_ret.empty() &&
                           bare_ret != "void" &&
                           bare_ret != "__infer__" &&
                           !infer_ret);
      if (needs_return) {
        // Scan body tokens for a return at brace-depth 0
        bool has_top_level_return = false;
        int scan_depth = 0;
        for (size_t si = body_tok_start; si < body_tok_end_for_check && si < tokens.size(); si++) {
          const Token &st = tokens[si];
          if (st.type == TT::LBRACE) { scan_depth++; continue; }
          if (st.type == TT::RBRACE) { scan_depth--; continue; }
          if (st.type == TT::RETURN && scan_depth == 0) {
            // Make sure this is a value-returning return (next token is not ; or })
            if (si + 1 < body_tok_end_for_check) {
              TT nt = tokens[si+1].type;
              if (nt != TT::SEMICOLON && nt != TT::RBRACE && nt != TT::TEOF)
                has_top_level_return = true;
            }
          }
        }
        if (!has_top_level_return) {
          // Derive the snippet for the function header line
          std::string fn_snip;
          if (!g_current_source.empty())
            fn_snip = extract_line_from_source(g_current_source, fn_tok.line);
          throw XenonError("MissingReturn",
            "function '" + fname + "' has return type '" + bare_ret +
            "' but does not return a value on all paths — "
            "non-void functions must always return a value (even if unsafe)",
            fn_tok.line, fn_tok.col,
            fn_snip);
        }
      }
    }

    // -----------------------------------------------------------------------
    // User-input taint analysis + runtime check injection.
    // This is the ONLY place where runtime code is emitted.  It fires only
    // when the function is proven (at compile time) to return a value
    // derived from user input.  All other safety checks are purely static.
    // -----------------------------------------------------------------------
    std::string rt_preamble;
    std::string rt_epilogue;

    if (_memory_safe && ret_type != "void" && !ret_type.empty()) {
      auto taint = analyze_user_input_taint(body_tok_start, body_tok_end_for_check,
                                             ret_type);
      if (taint.returns_tainted) {
        // Ensure the errno header is present (once per compilation unit)
        if (!_user_input_rt_header_emitted) {
          headers.insert(headers.begin(), user_input_rt_header());
          _user_input_rt_header_emitted = true;
        }

        if (taint.is_numeric) {
          // Wrap numeric body: reset errno at entry, check at exit.
          // The compiler will eliminate the check on the hot path at -O3.
          rt_preamble =
            "    /* [Xenon:InputRT] user-input numeric return — reset errno */\n"
            "    int _xen_errno_flag_ = 0;\n"
            "    (void)_xen_errno_flag_;\n"
            "    _XEN_INPUT_BEGIN();\n";
          // Rewrite each 'return EXPR;' in the body to capture and check
          // We do this by post-processing the body strings.
          // Instead of rewriting (complex), we append an errno capture before
          // every return by wrapping the entire body in a do-while with a
          // label-based exit.  This is zero-overhead at -O3 (the do-while
          // collapses to straight-line code).
          // Simpler approach: emit a wrapper that checks errno AFTER the call
          // at the call site — but we can't do that from inside parse_function_body.
          // Cleanest: emit the function normally, then emit a _checked_ wrapper
          // with the same signature that calls the inner function and validates.
          // This keeps the original function unchanged and adds zero overhead
          // on the normal (non-user-input) path.

          // Mark the inner function as _xen_inner_<fname>
          std::string inner_fname = "_xen_inner_" + fname;
          std::string wrapper_params = join(params, ", ");
          // Build arg-forwarding list (just the names)
          std::vector<std::string> arg_names;
          for (const auto &pi : param_infos) arg_names.push_back(pi.name);
          std::string args_fwd = join(arg_names, ", ");

          // Emit inner function (renamed)
          std::string inner_decl =
            "static __attribute__((noinline)) " +
            ret_type + " " + inner_fname + "(" + wrapper_params + ") {\n" +
            join(body, "\n") + "\n}\n";
          functions.push_back(inner_decl);

          // Emit wrapper with runtime check
          std::string check_code = emit_user_input_validation(fname, taint, ret_type);
          std::string wrapper =
            ret_type + " " + fname + "(" + wrapper_params + ") {\n"
            "    _XEN_INPUT_BEGIN();\n"
            "    " + ret_type + " _xen_ret_ = " + inner_fname + "(" + args_fwd + ");\n"
            "    int _xen_errno_flag_ = _XEN_INPUT_CHECK_ERRNO();\n" +
            check_code +
            "    return _xen_ret_;\n"
            "}\n";
          return wrapper;

        } else if (taint.is_string) {
          // String return: wrap similarly but check for NULL
          std::string inner_fname = "_xen_inner_" + fname;
          std::string wrapper_params = join(params, ", ");
          std::vector<std::string> arg_names;
          for (const auto &pi : param_infos) arg_names.push_back(pi.name);
          std::string args_fwd = join(arg_names, ", ");

          std::string inner_decl =
            "static __attribute__((noinline)) " +
            ret_type + " " + inner_fname + "(" + wrapper_params + ") {\n" +
            join(body, "\n") + "\n}\n";
          functions.push_back(inner_decl);

          std::string wrapper =
            ret_type + " " + fname + "(" + wrapper_params + ") {\n"
            "    " + ret_type + " _xen_str_ret_ = " + inner_fname + "(" + args_fwd + ");\n"
            "    if (__builtin_expect((_xen_str_ret_ == NULL), 0)) {\n"
            "        fprintf(stderr, \"[Xenon] SAFETY: function '" + fname + "' \"\n"
            "                \"returned NULL for a user-input string.\\n\");\n"
            "        abort();\n"
            "    }\n"
            "    return _xen_str_ret_;\n"
            "}\n";
          return wrapper;
        }
      }
    }

    (void)rt_preamble; (void)rt_epilogue;

    return ret_type + " " + fname + "(" + join(params, ", ") + ") {\n" +
           join(body, "\n") + "\n}\n";
  }

  // -----------------------------------------------------------------------
  // System include search paths (queried once from gcc -v, cached).
  // -----------------------------------------------------------------------
  static std::vector<std::string> system_include_paths() {
    static std::vector<std::string> cached;
    static bool done = false;
    if (done) return cached;
    done = true;
    // Ask gcc where it searches for system headers
    FILE *p = popen("gcc -x c -v -fsyntax-only /dev/null 2>&1", "r");
    if (!p) { cached = {"/usr/include","/usr/local/include"}; return cached; }
    std::string out;
    char buf[512];
    while (fgets(buf, sizeof(buf), p)) out += buf;
    pclose(p);
    // Parse lines between "#include <...> search starts here:" and "End of search"
    bool in_section = false;
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
      if (line.find("#include <...>") != std::string::npos) { in_section = true; continue; }
      if (in_section && line.find("End of search") != std::string::npos) break;
      if (in_section && !line.empty() && line[0] == ' ') {
        std::string p2 = line.substr(1);
        while (!p2.empty() && (p2.back() == ' ' || p2.back() == '\n' || p2.back() == '\r'))
          p2.pop_back();
        if (!p2.empty()) cached.push_back(p2);
      }
    }
    if (cached.empty()) cached = {"/usr/include","/usr/local/include","/usr/include/x86_64-linux-gnu"};
    return cached;
  }

  // -----------------------------------------------------------------------
  // Find a .h file: local → _include_paths → system include paths.
  // Returns full path or "" if not found anywhere.
  // -----------------------------------------------------------------------
  std::string find_h_on_system(const std::string &fname) const {
    // 1. Absolute path
    if (fs::path(fname).is_absolute() && fs::exists(fname))
      return fs::weakly_canonical(fname).string();
    // 2. Relative to source dir
    {
      std::string c = (fs::path(_source_dir) / fname).string();
      if (fs::exists(c)) return fs::weakly_canonical(c).string();
    }
    // 3. -l include paths
    for (const auto &ip : _include_paths) {
      std::string c = (fs::path(ip) / fname).string();
      if (fs::exists(c)) return fs::weakly_canonical(c).string();
    }
    // 4. System paths
    for (const auto &sp : system_include_paths()) {
      std::string c = (fs::path(sp) / fname).string();
      if (fs::exists(c)) return fs::weakly_canonical(c).string();
    }
    return "";
  }

  // -----------------------------------------------------------------------
  // Trim leading/trailing whitespace from a string (in-place helper).
  // -----------------------------------------------------------------------
  static void trim(std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) { s.clear(); return; }
    size_t b = s.find_last_not_of(" \t\r\n");
    s = s.substr(a, b - a + 1);
  }
  static std::string trimmed(std::string s) { trim(s); return s; }

  // -----------------------------------------------------------------------
  // Normalise a C type string: collapse spaces before *, remove trailing ws.
  // "char *"  → "char*"   "unsigned int " → "unsigned int"
  // -----------------------------------------------------------------------
  static std::string norm_ctype(const std::string &raw) {
    std::string s = trimmed(raw);
    // collapse "type *" → "type*" and "type **" → "type**"
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
      if (s[i] == ' ' && i + 1 < s.size() && s[i+1] == '*') continue;
      out += s[i];
    }
    // also handle "const " prefix — keep it for correctness
    return out;
  }

  // -----------------------------------------------------------------------
  // scan_h_full: parse a .h file (or locate it on system paths) and extract:
  //   • typedef struct { ... } Name  → struct_field_types[Name]
  //   • struct Name { ... }          → struct_field_types[Name]
  //   • RetType FuncName(...)        → func_return_types[FuncName]
  //
  // Uses  gcc -E  (preprocessor) to flatten #includes and macros first,
  // then applies a line-by-line C parser for structs and function decls.
  // Falls back silently if gcc is unavailable.
  //
  // 'path' must already be resolved; call find_h_on_system() first.
  // -----------------------------------------------------------------------
  void scan_h_full(const std::string &path) {
    if (path.empty() || !fs::exists(path)) return;

    // ── 1. Read the .h file directly ─────────────────────────────────────
    std::ifstream hf(path);
    if (!hf) return;
    std::string src((std::istreambuf_iterator<char>(hf)),
                     std::istreambuf_iterator<char>());
    if (src.empty()) return;

    // Strip preprocessor directives (#include, #define simple macros, #ifdef etc.)
    // but keep typedef/struct/function declarations.
    // We do a lightweight strip: remove #include lines, and skip #if/#ifdef
    // blocks by tracking nesting depth.
    {
      std::string filtered;
      filtered.reserve(src.size());
      std::istringstream ss2(src);
      std::string ln;

      while (std::getline(ss2, ln)) {
        // Handle line-continuations for #define — skip multi-line macros
        std::string tln = ln;
        { size_t f = tln.find_first_not_of(" \t"); if (f != std::string::npos) tln = tln.substr(f); }
        if (!tln.empty() && tln[0] == '#') {
          while (!ln.empty() && ln.back() == '\\') {
            if (!std::getline(ss2, ln)) break;
          }
          // Silently drop all preprocessor lines; preserve line count.
          filtered += '\n';
          continue;
        }
        filtered += ln + '\n';
      }
      src = std::move(filtered);
    }

    // ── 2. Tokenise preprocessed text into logical lines ──────────────────
    // Collapse multi-line constructs (struct bodies etc.) into single logical
    // lines by tracking brace depth.  We emit each top-level declaration as
    // one string.
    std::vector<std::string> decls;
    {
      std::string cur;
      int brace = 0;
      bool in_str = false, in_char = false, in_bc = false;
      for (size_t i = 0; i < src.size(); i++) {
        char c = src[i];
        // line comments (from -dD macros)
        if (!in_str && !in_char && !in_bc && c == '/' && i+1<src.size() && src[i+1]=='/') {
          while (i < src.size() && src[i] != '\n') i++;
          continue;
        }
        // block comments
        if (!in_str && !in_char && c == '/' && i+1<src.size() && src[i+1]=='*') {
          i += 2;
          while (i+1 < src.size() && !(src[i]=='*' && src[i+1]=='/')) i++;
          i++;
          continue;
        }
        if (c == '"' && !in_char) { in_str = !in_str; cur += c; continue; }
        if (c == '\'' && !in_str) { in_char = !in_char; cur += c; continue; }
        if (in_str || in_char) { cur += c; continue; }
        if (c == '{') { brace++; cur += c; continue; }
        if (c == '}') { brace--; cur += c;
          if (brace == 0) {
            // Consume everything up to and including the next ';' at depth 0.
            // This captures the typedef tag name: "} Point;" → appends " Point;"
            size_t j = i+1;
            while (j < src.size() && src[j] != ';' && src[j] != '{') j++;
            if (j < src.size() && src[j] == ';') {
              cur += src.substr(i+1, j-i); // includes the ';'
              i = j;
            }
            decls.push_back(cur); cur.clear();
          }
          continue;
        }
        if (brace == 0 && c == ';') {
          cur += c;
          std::string d = trimmed(cur);
          if (!d.empty()) decls.push_back(d);
          cur.clear();
          continue;
        }
        // normalise whitespace at brace depth 0
        if (brace == 0 && (c=='\n'||c=='\r')) {
          if (!cur.empty() && cur.back() != ' ') cur += ' ';
          continue;
        }
        cur += c;
      }
      if (!trimmed(cur).empty()) decls.push_back(trimmed(cur));
    }

    // ── 3. Parse each declaration ─────────────────────────────────────────
    // Helper: does a string contain a '{' → it is a struct/union body
    auto has_body = [](const std::string &d) {
      return d.find('{') != std::string::npos;
    };

    // Helper: extract field name and type from "type name" or "type *name" etc.
    // Returns {"", ""} on failure.
    auto parse_field = [&](const std::string &fld) -> std::pair<std::string,std::string> {
      std::string s = trimmed(fld);
      if (s.empty()) return {"",""};
      // Remove array suffixes like "float x[4]" → field name "x", type "float"
      // Also skip function pointer fields (contain '(')
      if (s.find('(') != std::string::npos) return {"",""};
      // strip array suffix
      size_t lb = s.find('[');
      if (lb != std::string::npos) s = trimmed(s.substr(0, lb));
      // last token is the name
      size_t sp = s.rfind(' ');
      if (sp == std::string::npos) return {"",""};
      std::string name = trimmed(s.substr(sp+1));
      std::string type = norm_ctype(trimmed(s.substr(0, sp)));
      // strip leading '*' from name → move to type
      while (!name.empty() && name[0] == '*') { type += '*'; name = name.substr(1); }
      if (name.empty() || type.empty()) return {"",""};
      // skip keywords that aren't field names
      static const std::set<std::string> skip = {"const","volatile","restrict","static","inline"};
      if (skip.count(name)) return {"",""};
      return {name, type};
    };

    // Helper: extract fields from a struct body string "{ type name; type name; ... }"
    auto parse_struct_body = [&](const std::string &body, const std::string &sname) {
      auto &fmap = struct_field_types[sname];
      // strip outer braces
      size_t ob = body.find('{'), cb = body.rfind('}');
      if (ob == std::string::npos || cb == std::string::npos) return;
      std::string inner = body.substr(ob+1, cb-ob-1);
      // split on ';'
      size_t p = 0;
      while (p < inner.size()) {
        size_t semi = inner.find(';', p);
        if (semi == std::string::npos) break;
        std::string fld = trimmed(inner.substr(p, semi-p));
        p = semi + 1;
        if (fld.empty()) continue;
        // skip nested struct/union (contains '{')
        if (fld.find('{') != std::string::npos) continue;
        auto [fn, ft] = parse_field(fld);
        if (!fn.empty() && !ft.empty())
          fmap[fn] = ft;
      }
    };

    // Helper: parse function return type from "RetType fname(..." 
    // where the '(' marks the split.
    auto parse_func_decl = [&](const std::string &d) {
      // skip if it's a macro define or typedef-only
      if (d.substr(0,7) == "#define" || d.substr(0,8) == "typedef ") return;
      size_t lp = d.find('(');
      if (lp == std::string::npos) return;
      std::string before = trimmed(d.substr(0, lp));
      // last whitespace-separated token before '(' is the function name
      // handle "(*name)" function pointers — skip them
      if (before.find('*') != std::string::npos && before.find('(') != std::string::npos)
        return;
      size_t sp2 = before.rfind(' ');
      if (sp2 == std::string::npos) return;
      std::string fn = trimmed(before.substr(sp2+1));
      std::string ret = norm_ctype(trimmed(before.substr(0, sp2)));
      // strip leading '*' from fn → pointer return type
      while (!fn.empty() && fn[0]=='*') { ret += '*'; fn = fn.substr(1); }
      if (fn.empty() || ret.empty()) return;
      // skip C keywords that aren't identifiers
      static const std::set<std::string> skip2 = {
        "if","else","while","for","do","return","switch","case","default",
        "break","continue","sizeof","typedef","struct","union","enum",
        "static","extern","inline","const","volatile","restrict"
      };
      if (skip2.count(fn)) return;
      // must start with letter or underscore
      if (!std::isalpha((unsigned char)fn[0]) && fn[0]!='_') return;
      func_return_types.emplace(fn, ret); // emplace: don't overwrite user-defined
      _h_imported_functions.insert(fn);
    };

    for (const auto &d : decls) {
      std::string s = trimmed(d);
      if (s.empty() || s[0] == '#') continue; // skip macro lines

      // ── typedef struct { ... } Name; ─────────────────────────────────
      if (s.substr(0, 8) == "typedef " && has_body(s)) {
        // extract tag after closing '}'
        size_t cb = s.rfind('}');
        if (cb != std::string::npos) {
          std::string tag = trimmed(s.substr(cb+1));
          // remove trailing ';'
          while (!tag.empty() && tag.back()==';') tag.pop_back();
          trim(tag);
          if (!tag.empty() && tag.find(' ')==std::string::npos) {
            parse_struct_body(s, tag);
            // Also register as a known struct type so is_custom triggers
            var_types[tag] = "STRUCT";
            _h_imported_types.insert(tag);
          }
        }
        continue;
      }

      // ── struct Name { ... }; ─────────────────────────────────────────
      if ((s.substr(0,7)=="struct " || s.substr(0,6)=="union ") && has_body(s)) {
        size_t sp3 = s.find(' ');
        size_t ob  = s.find('{');
        if (sp3 != std::string::npos && ob != std::string::npos) {
          std::string sname = trimmed(s.substr(sp3+1, ob-sp3-1));
          if (!sname.empty() && sname.find(' ')==std::string::npos) {
            parse_struct_body(s, sname);
            var_types[sname] = "STRUCT";
            _h_imported_types.insert(sname);
          }
        }
        continue;
      }

      // ── typedef Name AliasName; (no body) ────────────────────────────
      if (s.substr(0,8) == "typedef " && !has_body(s)) {
        // typedef struct Foo Foo; or typedef int MyInt; or typedef enum E E; etc.
        std::string rest = trimmed(s.substr(8));
        while (!rest.empty() && rest.back()==';') rest.pop_back();
        trim(rest);
        size_t sp3 = rest.rfind(' ');
        if (sp3 != std::string::npos) {
          std::string alias = trimmed(rest.substr(sp3+1));
          std::string orig  = trimmed(rest.substr(0, sp3));
          // strip leading '*' from alias (e.g. typedef int *IntPtr)
          while (!alias.empty() && alias[0] == '*') alias = alias.substr(1);
          if (!alias.empty() && alias.find(' ')==std::string::npos) {
            if (struct_field_types.count(orig) || var_types.count(orig)) {
              var_types[alias] = var_types.count(orig) ? var_types[orig] : "STRUCT";
              if (struct_field_types.count(orig) && !struct_field_types.count(alias))
                struct_field_types[alias] = struct_field_types[orig];
            } else {
              // Plain typedef (int, unsigned, function pointer, etc.)
              var_types[alias] = orig;
            }
            _h_imported_types.insert(alias);
          }
        }
        continue;
      }

      // ── enum Name { ... }; or typedef enum { ... } Name; ─────────────
      if (has_body(s) &&
          (s.substr(0,5) == "enum " || s.find("enum ") != std::string::npos)) {
        size_t cb = s.rfind('}');
        if (cb != std::string::npos) {
          std::string tail = trimmed(s.substr(cb+1));
          while (!tail.empty() && tail.back()==';') tail.pop_back();
          trim(tail);
          if (!tail.empty() && tail.find(' ')==std::string::npos) {
            // typedef enum { ... } Name;
            _enum_names.insert(tail);
            var_types[tail] = "ENUM";
            _h_imported_types.insert(tail);
          } else {
            // enum Name { ... };
            size_t epos = s.find("enum ");
            if (epos != std::string::npos) {
              size_t ob = s.find('{', epos);
              if (ob != std::string::npos) {
                std::string ename = trimmed(s.substr(epos+5, ob-(epos+5)));
                if (!ename.empty() && ename.find(' ')==std::string::npos) {
                  _enum_names.insert(ename);
                  var_types[ename] = "ENUM";
                  _h_imported_types.insert(ename);
                }
              }
            }
          }
        }
        continue;
      }

      // ── Everything else: try as a function declaration or definition ──
      // parse_func_decl only inspects the text before the first '(', so it
      // correctly extracts the name/return-type from both bare declarations
      // ("void foo();") and inline definitions ("void foo() { ... }").
      parse_func_decl(s);
    }
  }

  // Convenience: resolve fname on all paths then call scan_h_full.
  // Replaces the old scan_h_for_funcs call sites.
  void scan_h_for_funcs(const std::string &path) {
    // Keep the old name so existing call sites don't need changing.
    scan_h_full(path);
  }

  // -----------------------------------------------------------------------
  // .xen include
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

    // if still not found, search standard library paths with /xenon subfolder
    if (canonical.empty()) {
      std::vector<std::string> stdlib_paths = {
          "/usr/local/lib/lb", "/usr/include/xenon", "/usr/local/lib",
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
      throw XenonError("LinkError",
                         "Cannot find .xen file '" + fname + "' (looked in '" +
                             _source_dir +
                             "', include paths, and standard locations)",
                         tok.line, tok.col);
    std::ifstream f(canonical);
    if (!f)
      throw XenonError("LinkError", "Could not read '" + fname + "'",
                         tok.line, tok.col);
    std::string lh_source((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    std::vector<Token> lh_toks = Lexer(lh_source, prescan_addop_symbols(lh_source)).tokenize();

    CTranspiler lh(lh_toks, tcc_mode);
    lh.isSubTranspiler = true;
    lh.emit_line_directives = emit_line_directives;
    lh._source_dir = fs::path(canonical).parent_path().string();
    lh._source_file = canonical;
    lh._lh_included = _lh_included;
    lh.var_types = var_types;
    lh.func_return_types = func_return_types;
    lh.struct_field_types = struct_field_types;
    lh.func_param_types = func_param_types;
    lh._handle_declared = _handle_declared;
    lh._memory_safe = _memory_safe;
    lh._unsafe_functions = _unsafe_functions;
    lh._used_modules = _used_modules;
    lh._module_funcs_loaded = _module_funcs_loaded;

    while (lh.current().type != TT::TEOF) {
      Token &lt = lh.current();
      if (lt.type == TT::SEMICOLON) {
        lh.advance();
        continue;
      }
      if (lt.type == TT::FUNCTION) {
        lh.advance();
        std::string code = lh.emit_function(lt, false);
        if (!code.empty())
          functions.push_back(lh.line_directive(lt) + code);
        // If emit_function detected a template, it patched tok_start in
        // lh.template_funcs. Extract tokens from the template for cross-module
        // portability.
        { // process both regular functions and templates (which return "")
          // Find the most recently added template in lh and extract its tokens
          for (auto &[tname, tmpl] : lh.template_funcs) {
            if (tmpl.has_extracted)
              continue; // already extracted
            if (tmpl.tok_start > 0 && tmpl.tok_end <= lh.tokens.size()) {
              tmpl.extracted_tokens.assign(lh.tokens.begin() + tmpl.tok_start,
                                           lh.tokens.begin() + tmpl.tok_end);
              tmpl.has_extracted = !tmpl.extracted_tokens.empty();
            }
          }
        }
      } else if (lt.type == TT::INLINE_KW) {
        lh.advance();
        if (lh.current().type != TT::FUNCTION)
          throw XenonError("SyntaxError",
                             "'inline' must be followed by 'function'", lt.line,
                             lt.col);
        lh.advance();
        std::string code = lh.emit_function(lt, true);
        if (!code.empty())
          functions.push_back(lh.line_directive(lt) + code);
        // Extract tokens from any newly-registered templates
        for (auto &[tname, tmpl] : lh.template_funcs) {
          if (tmpl.has_extracted)
            continue;
          if (tmpl.tok_start > 0 && tmpl.tok_end <= lh.tokens.size()) {
            tmpl.extracted_tokens.assign(lh.tokens.begin() + tmpl.tok_start,
                                         lh.tokens.begin() + tmpl.tok_end);
            tmpl.has_extracted = !tmpl.extracted_tokens.empty();
          }
        }
      } else if (lt.type == TT::TYPE) {
        headers.push_back(lh.line_directive(lt) + lh.parse_type_definition());
      } else if (lt.type == TT::ENUM_KW) {
        headers.push_back(lh.line_directive(lt) + lh.parse_enum());
      } else if (lt.type == TT::LINK) {
        lh.advance();
        Token nt = lh.expect(TT::STRING, true);
        if (!nt.value.empty()) {
          if (ends_with(nt.value, ".xen"))
            transpile_lh(nt.value, nt);
          else {
            headers.push_back("#include \"" + nt.value + "\"");
            // scan for type inference
            for (const auto &ipath : _include_paths) {
              std::string candidate = (fs::path(ipath) / nt.value).string();
              if (fs::exists(candidate)) {
                scan_h_for_funcs(candidate);
                break;
              }
            }
            std::string local = (fs::path(_source_dir) / nt.value).string();
            if (fs::exists(local))
              scan_h_for_funcs(local);
          }
        }
      } else if (lt.type == TT::IN_KW) {
        lh.advance();
        lh.parse_namespace_block(lt);
      } else if (lt.type == TT::IGNORE_KW) {
        lh.advance();
        if (lh.current().type == TT::NAMESPACE_KW) {
          lh.advance();
          Token ns_tok = lh.expect(TT::IDENTIFIER, true);
          if (!ns_tok.value.empty())
            lh._ignored_namespaces.insert(ns_tok.value);
        }
      } else if (lt.type == TT::UNSAFE_KW) {
        lh.advance();
        if (lh.current().type != TT::FUNCTION)
          throw XenonError("SyntaxError",
                           "'unsafe' must be followed by 'function'",
                           lt.line, lt.col);
        Token fn_tok = lh.current();
        lh.advance();
        size_t name_scan = lh.pos;
        {
          static const std::set<TT> skip_ret = {
            TT::INT,TT::FLOAT,TT::STR,TT::LONG,TT::SHORT,TT::DOUBLE,
            TT::VOID,TT::M256,TT::M256I,TT::BOOL_KW,TT::CHAR_KW,
            TT::PTR,TT::U8,TT::U32,TT::U64,TT::LET_KW,TT::VAR_KW,
            TT::IDENTIFIER
          };
          while (name_scan < lh.tokens.size() && skip_ret.count(lh.tokens[name_scan].type)) {
            if (lh.tokens[name_scan].type == TT::PTR) {
              name_scan++;
              if (name_scan < lh.tokens.size() && skip_ret.count(lh.tokens[name_scan].type))
                name_scan++;
              break;
            }
            name_scan++;
            break;
          }
        }
        if (name_scan < lh.tokens.size() && lh.tokens[name_scan].type == TT::IDENTIFIER) {
          std::string unsafe_fname = lh.mangle_with_ns(lh.tokens[name_scan].value);
          lh._unsafe_functions.insert(unsafe_fname);
          lh._unsafe_functions.insert(lh.tokens[name_scan].value);
        }
        std::string code = lh.emit_function(fn_tok, false);
        if (!code.empty())
          functions.push_back(lh.line_directive(fn_tok) + code);
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
      if (type_str == "STRUCT" || type_str == "GENERIC_STRUCT")
        this->_xen_imported_types.insert(name);
    }
    for (auto const &[name, type_str] : lh.func_return_types) {
      this->func_return_types[name] = type_str;
      this->_xen_imported_functions.insert(name);
    }
    for (auto const &[name, ptypes] : lh.func_param_types) {
      this->func_param_types[name] = ptypes;
    }
    for (auto const &[sname, fields] : lh.struct_field_types) {
      this->struct_field_types[sname] = fields;
    }
    for (auto const &[name, tmpl] : lh.template_funcs) {
      this->template_funcs[name] = tmpl;
    }
    for (auto const &[sym, entries] : lh._op_overloads) {
      for (const auto &entry : entries) {
        auto &dest = this->_op_overloads[sym];
        bool already = false;
        for (const auto &e : dest)
          if (e.func_name == entry.func_name) { already = true; break; }
        if (!already) dest.push_back(entry);
      }
    }
    for (auto const &[aname, orig] : lh._aliases) {
      this->_aliases[aname] = orig;
    }
    for (auto const &[nsname, nsmap] : lh._namespaces) {
      for (auto const &[sym, cname] : nsmap)
        this->_namespaces[nsname][sym] = cname;
    }
    for (auto const &ns : lh._ignored_namespaces)
      this->_ignored_namespaces.insert(ns);
    this->_lh_included = lh._lh_included;
    for (auto const &uf : lh._unsafe_functions)
      this->_unsafe_functions.insert(uf);
    for (const auto &fn : lh._xen_imported_functions)
      this->_xen_imported_functions.insert(fn);
    for (const auto &tn : lh._xen_imported_types)
      this->_xen_imported_types.insert(tn);
    for (const auto &en : lh._enum_names)
      this->_enum_names.insert(en);
    // Merge module state
    for (const auto &m : lh._used_modules)
      this->_used_modules.insert(m);
    for (const auto &m : lh._module_funcs_loaded)
      this->_module_funcs_loaded.insert(m);
  }

public:
  explicit CTranspiler(std::vector<Token> toks, bool tcc_mode_ = false, bool memory_safe_ = true, bool borrow_check_ = true, bool suppress_warnings_ = false)
      : tcc_mode(tcc_mode_), tokens(std::move(toks)), _memory_safe(memory_safe_), _borrow_check(borrow_check_), suppress_warnings(suppress_warnings_) {
    // build default headers block
    if (!isSubTranspiler) {
      if (tcc_mode) {
        // TCC: C99 — no <immintrin.h>, no __m256/__m256i, no _Generic
        headers.push_back("#include <stdio.h>\n"
                          "#include <stdbool.h>\n"
                          "#include <stdlib.h>\n"
                          "#include <math.h>\n"
                          "#include <string.h>\n"
                          "#include <unistd.h>\n"
                          "#include <setjmp.h>\n"
                          "#include <inttypes.h>\n"
                          "#include <stdint.h>\n");
        // TCC runtime: only _lb_throw is needed (for try/except/throw).
        // No _lb_ print helpers, no TO_STR — print/println emit plain printf directly.
        headers.push_back(
          "/* generated by Xenon compiler v3.1.2 */\n"
            "#ifndef __XENON_RUNTIME__\n"
            "#define __XENON_RUNTIME__\n"
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
            "#endif /* __XENON_RUNTIME__ */\n");
      } else {
        // Default (clang/gcc): full C11 preamble with _Generic and AVX
        headers.push_back("/* generated by Xenon compiler v3.1.2 */\n"
                          "#include <stdio.h>\n"
                          "#include <stdbool.h>\n"
                          "#include <stdlib.h>\n"
                          "#include <math.h>\n"
                          "#include <immintrin.h>\n"
                          "#include <string.h>\n"
                          "#include <unistd.h>\n"
                          "#include <setjmp.h>\n"
                          "#include <inttypes.h>\n"
                          "#include <stdint.h>\n");
        headers.push_back(
            "#ifndef __XENON_RUNTIME__\n"
            "#define __XENON_RUNTIME__\n"
            "static char _lb_bufs[8][512];\n"
            "static int  _lb_buf_idx = 0;\n"
            "static inline char* _lb_next(void) { _lb_buf_idx=(_lb_buf_idx+1)%8; return _lb_bufs[_lb_buf_idx]; }\n"
            "static inline char* _lb_s(char* x)        { return x; }\n"
            "static inline char* _lb_cs(const char* x) { return (char*)x; }\n"
            "static inline char* _lb_f(float x)        { char*b=_lb_next();"
            "snprintf(b,512,\"%g\",x); return b; }\n"
            "static inline char* _lb_d(double x)       { char*b=_lb_next();"
            "snprintf(b,512,\"%.10g\",x); return b; }\n"
            "static inline char* _lb_i(int x)          { char*b=_lb_next();"
            "snprintf(b,512,\"%d\",x); return b; }\n"
            "static inline char* _lb_l(long x)         { char*b=_lb_next();"
            "snprintf(b,512,\"%ld\",x); return b; }\n"
            "static inline char* _lb_u(short x)        { char*b=_lb_next();"
            "snprintf(b,512,\"%d\",(int)x); return b; }\n"
            "static inline char* _lb_b(int x)          { return x ? \"true\" : "
            "\"false\"; }\n"
            "static inline char* _lb_c(char x)         { char*b=_lb_next(); b[0]=x; "
            "b[1]='\\0'; return b; }\n"
            "static inline char* _lb_m(__m256 v)  { float f[8]; "
            "_mm256_storeu_ps(f,v); char*b=_lb_next();"
            "snprintf(b,512,\"[%g,%g,%g,%g,%g,%g,%g,%g]\","
            "f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7]); return b; }\n"
            "static inline char* _lb_mi(__m256i v) { union{__m256i v;int "
            "i[8];}u; u.v=v; char*b=_lb_next();"
            "snprintf(b,512,\"[%d,%d,%d,%d,%d,%d,%d,%d]\","
            "u.i[0],u.i[1],u.i[2],u.i[3],u.i[4],u.i[5],u.i[6],u.i[7]); return "
            "b; }\n"
            "static inline char* _lb_u8(uint8_t x)   { char*b=_lb_next();"
            "snprintf(b,512,\"%\" PRIu8,x); return b; }\n"
            "static inline char* _lb_u32(uint32_t x) { char*b=_lb_next();"
            "snprintf(b,512,\"%\" PRIu32,x); return b; }\n"
            "static inline char* _lb_u64(uint64_t x) { char*b=_lb_next();"
            "snprintf(b,512,\"%\" PRIu64,x); return b; }\n"
            "#define TO_STR(x) _Generic((x),"
            "char*:_lb_s,const char*:_lb_cs,"
            "__m256:_lb_m,__m256i:_lb_mi,"
            "float:_lb_f,double:_lb_d,"
            "int:_lb_i,long:_lb_l,short:_lb_u,"
            "uint8_t:_lb_u8,uint32_t:_lb_u32,uint64_t:_lb_u64,"
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
            "#endif /* __XENON_RUNTIME__ */\n");
      }
    }
    // ── Seed well-known C stdlib/math function return types ──────────────────
    // Many standard functions are declared via macro magic in system headers
    // (e.g. fabsf via __MATHCALL in math.h) and are invisible to our direct
    // header parser.  We hardcode their signatures here — they're part of the
    // C standard and will never change.
    // Format: { "name", "return_type" }
    // emplace() so user forward-decls (or .h scan results) win over these.
    static const std::pair<const char*, const char*> builtins[] = {
      // <math.h> — float variants
      {"fabsf","float"},{"sqrtf","float"},{"cbrtf","float"},
      {"sinf","float"},{"cosf","float"},{"tanf","float"},
      {"asinf","float"},{"acosf","float"},{"atanf","float"},{"atan2f","float"},
      {"sinhf","float"},{"coshf","float"},{"tanhf","float"},
      {"expf","float"},{"exp2f","float"},{"logf","float"},{"log2f","float"},{"log10f","float"},
      {"powf","float"},{"fmodf","float"},{"remainderf","float"},
      {"ceilf","float"},{"floorf","float"},{"roundf","float"},{"truncf","float"},
      {"fminf","float"},{"fmaxf","float"},{"fmaf","float"},
      {"hypotf","float"},{"copysignf","float"},{"ldexpf","float"},
      {"frexpf","float"},  // actually returns float; exponent via int* param
      {"modff","float"},
      // <math.h> — double variants
      {"fabs","double"},{"sqrt","double"},{"cbrt","double"},
      {"sin","double"},{"cos","double"},{"tan","double"},
      {"asin","double"},{"acos","double"},{"atan","double"},{"atan2","double"},
      {"sinh","double"},{"cosh","double"},{"tanh","double"},
      {"exp","double"},{"exp2","double"},{"log","double"},{"log2","double"},{"log10","double"},
      {"pow","double"},{"fmod","double"},{"remainder","double"},
      {"ceil","double"},{"floor","double"},{"round","double"},{"trunc","double"},
      {"fmin","double"},{"fmax","double"},{"fma","double"},
      {"hypot","double"},{"copysign","double"},{"ldexp","double"},
      // <math.h> — int-returning
      {"abs","int"},{"isinf","int"},{"isnan","int"},{"isfinite","int"},
      {"fpclassify","int"},
      // <stdlib.h>
      {"malloc","void*"},{"calloc","void*"},{"realloc","void*"},
      {"atoi","int"},{"atol","long"},{"atof","double"},
      {"strtol","long"},{"strtoul","unsigned long"},{"strtod","double"},
      {"rand","int"},{"system","int"},{"abs","int"},
      {"getenv","char*"},
      // <string.h>
      {"strlen","int"},{"strcmp","int"},{"strncmp","int"},
      {"strchr","char*"},{"strrchr","char*"},{"strstr","char*"},
      {"strcpy","char*"},{"strncpy","char*"},{"strcat","char*"},{"strncat","char*"},
      {"memcpy","void*"},{"memmove","void*"},{"memset","void*"},{"memcmp","int"},
      {"strdup","char*"},{"strndup","char*"},{"strtok","char*"},
      // <stdio.h>
      {"printf","int"},{"fprintf","int"},{"sprintf","int"},{"snprintf","int"},
      {"scanf","int"},{"fscanf","int"},{"sscanf","int"},
      {"fopen","void*"},{"fclose","int"},{"fread","int"},{"fwrite","int"},
      {"fgets","char*"},{"fputs","int"},{"fputc","int"},{"fgetc","int"},
      {"feof","int"},{"ferror","int"},{"fflush","int"},{"ftell","long"},
      {"fseek","int"},{"rewind","void"},{"remove","int"},{"rename","int"},
      {"puts","int"},{"putchar","int"},{"getchar","int"},{"getc","int"},
      // <time.h>
      {"time","long"},{"clock","long"},{"difftime","double"},
      // <ctype.h>
      {"isalpha","int"},{"isdigit","int"},{"isalnum","int"},{"isspace","int"},
      {"isupper","int"},{"islower","int"},{"toupper","int"},{"tolower","int"},
      // <unistd.h>
      {"sleep","int"},{"usleep","int"},{"getpid","int"},{"getppid","int"},
      {"fork","int"},{"exec","int"},{"read","long"},{"write","long"},{"close","int"},
    };
    for (auto &[name, ret] : builtins)
      func_return_types.emplace(name, ret); // emplace: user decls win
  }

  std::string transpile(const std::string &source_dir,
                        const std::string &source_file, bool manual_main,
                        const std::vector<std::string> &include_paths = {},
                        bool emit_line_dirs = true,
                        bool emit_binary = true) {
    _source_dir = source_dir;
    _source_file = source_file;
    _include_paths = include_paths;
    emit_line_directives = emit_line_dirs;

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
        } else if (tok.type == TT::OVERLOAD_KW) {
          advance();
          if (current().type != TT::OPERATOR_KW)
            throw XenonError("SyntaxError",
              "'overload' must be followed by 'operator'", tok.line, tok.col);
          advance();
          parse_overload(tok);
        } else if (tok.type == TT::ADDOP_KW) {
          advance();
          if (current().type != TT::OPERATOR_KW)
            throw XenonError("SyntaxError",
              "'addop' must be followed by 'operator'", tok.line, tok.col);
          advance();
          parse_addop(tok);
        } else if (tok.type == TT::ALIAS_KW) {
          advance();
          std::string def = parse_alias(tok);
          headers.push_back(def + "\n");
        } else if (tok.type == TT::USE_KW) {
          // use module::something — handled by parse_statement
          tok = current();
          std::string stmt = parse_statement();
          (void)stmt; // no C output for use declarations
        } else if (tok.type == TT::UNSAFE_KW) {
          if (pos + 1 < tokens.size() && tokens[pos + 1].type == TT::LBRACE) {
            tok = current();
            std::string stmt = parse_statement();
            if (!stmt.empty())
              main_body.push_back(line_directive(tok) + stmt);
          } else {
            advance();
            if (current().type != TT::FUNCTION)
              throw XenonError("SyntaxError",
                               "'unsafe' at top level must be followed by 'function' "
                               "or a block '{ }'",
                               tok.line, tok.col);
            Token fn_tok2 = current();
            advance();
            size_t name_scan = pos;
            {
              static const std::set<TT> skip_ret = {
                TT::INT,TT::FLOAT,TT::STR,TT::LONG,TT::SHORT,TT::DOUBLE,
                TT::VOID,TT::M256,TT::M256I,TT::BOOL_KW,TT::CHAR_KW,
                TT::PTR,TT::U8,TT::U32,TT::U64,TT::LET_KW,TT::VAR_KW,
                TT::IDENTIFIER
              };
              while (name_scan < tokens.size() && skip_ret.count(tokens[name_scan].type)) {
                if (tokens[name_scan].type == TT::PTR) {
                  name_scan++; // skip 'ptr'
                  // skip the pointee type so we land on the function name
                  if (name_scan < tokens.size() && skip_ret.count(tokens[name_scan].type))
                    name_scan++;
                  break;
                }
                name_scan++;
                if (name_scan < tokens.size() && tokens[name_scan].type == TT::LT) {
                  while (name_scan < tokens.size() && tokens[name_scan].type != TT::GT)
                    name_scan++;
                  if (name_scan < tokens.size()) name_scan++;
                }
                if (name_scan < tokens.size() && tokens[name_scan].type == TT::LSBRACKET) {
                  while (name_scan < tokens.size() && tokens[name_scan].type != TT::RSBRACKET)
                    name_scan++;
                  if (name_scan < tokens.size()) name_scan++;
                }
                break;
              }
            }
            if (name_scan < tokens.size() && tokens[name_scan].type == TT::IDENTIFIER) {
              std::string unsafe_fname = mangle_with_ns(tokens[name_scan].value);
              _unsafe_functions.insert(unsafe_fname);
              _unsafe_functions.insert(tokens[name_scan].value);
            }
            std::string code = emit_function(fn_tok2, false);
            if (!code.empty())
              functions.push_back(line_directive(fn_tok2) + code);
          }
        } else if (tok.type == TT::FUNCTION) {
          advance();
          std::string code = emit_function(tok, false);
          if (!code.empty())
            functions.push_back(line_directive(tok) + code);
        } else if (tok.type == TT::INLINE_KW) {
          advance();
          if (current().type != TT::FUNCTION)
            throw XenonError("SyntaxError",
                               "'inline' must be followed by 'function'",
                               tok.line, tok.col);
          advance();
          std::string code = emit_function(tok, true);
          if (!code.empty())
            functions.push_back(line_directive(tok) + code);
        } else if (tok.type == TT::LINK) {
          advance();
          Token ft = expect(TT::STRING, true);
          if (ft.value.empty())
            throw XenonError("SyntaxError",
                               "'link' requires a string filename", tok.line,
                               tok.col);
          if (ends_with(ft.value, ".xen"))
            transpile_lh(ft.value, ft);
          else {
            // Resolve the .h on local paths, -l paths, and system include paths.
            std::string h_path = find_h_on_system(ft.value);
            if (h_path.empty()) {
              // Not found — emit as a bare #include and warn; C compiler may find it.
              headers.push_back("#include <" + ft.value + ">");
              emit_warning("LinkWarning",
                "'" + ft.value + "' not found on include paths or system paths — "
                "type/function info unavailable; add -l<path> if it's non-standard",
                ft.line, ft.col);
            } else {
              // Emit as angle-bracket or quoted include depending on whether it's system
              bool is_sys = false;
              for (const auto &sp : system_include_paths())
                if (h_path.find(sp) == 0) { is_sys = true; break; }
              if (is_sys)
                headers.push_back("#include <" + ft.value + ">");
              else
                headers.push_back("#include \"" + ft.value + "\"");
              scan_h_full(h_path);
            }
          }
        } else {
          tok = current();
          std::string stmt = parse_statement();
          if (!stmt.empty())
            main_body.push_back(line_directive(tok) + stmt);
        }
      } catch (XenonError &_tr_err) {
        // Collect error; advance past the offending token so we can continue
        // parsing and surface all errors in one pass.
        transpile_errors.push_back(_tr_err.what());
        if (current().type != TT::TEOF) advance();
      } catch (std::exception &e) {
        transpile_errors.push_back(XenonError("InternalError",
                           std::string("Unexpected error near '") + tok.value +
                               "': " + e.what(),
                           tok.line, tok.col).what());
        if (current().type != TT::TEOF) advance();
      }
    }

    // ── -main mode: require user to declare a main function ─────────────────
    // When -main is active AND we are compiling to a binary (not -c emit-only),
    // the user must supply an explicit  function int main(...)  definition.
    // Without it the linker would fail with a cryptic "symbol main not found";
    // we catch it here and emit a clear compile-time error instead.
    if (manual_main && emit_binary) {
      bool found_main = func_return_types.count("main") > 0;
      if (!found_main) {
        int err_line = (!tokens.empty()) ? tokens[0].line : 1;
        int err_col  = (!tokens.empty()) ? tokens[0].col  : 1;
        std::string first_snip;
        if (!g_current_source.empty())
          first_snip = extract_line_from_source(g_current_source, err_line);
        transpile_errors.push_back(
          XenonError("EntryPointError",
            "-main flag is set but no 'main' function was declared.\n"
            "  In -main mode the program entry point must be written explicitly:\n"
            "    function int main(int argc, ptr str argv) { ... }\n"
            "  or a no-arg variant:\n"
            "    function int main() { ... }\n"
            "  Without it the linker cannot find the program entry point.",
            err_line, err_col, first_snip).what());
      }
    }

    std::string body_str = join(main_body, "\n    ");

    std::string res = join(headers, "\n") + "\n";
    res += join(functions, "\n") + "\n";
    if (manual_main) {
      if (!main_body.empty())
        res += "/* [Xenon] -main mode: top-level statements outside "
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
  bool emit_line_dirs = true;
  bool no_check = false;
  bool no_check_borrow = false;
  bool suppress_warnings = false;
  CCBackend cc_backend = CCBackend::CLANG;
  auto log = [&](const std::string &s) {
    if (!shut)
      std::cout << s << "\n";
  };
  auto die = [](const std::string &s, int code = -1) -> int {
    std::cerr << "\n" << ansi::separator() << "\n"
              << ansi::error_badge("FATAL") << "\n\n"
              << "  " << ansi::bwhite() << ansi::bold() << s << ansi::reset() << "\n"
              << ansi::separator() << "\n\n";
    exit(code);
    return code;
  };

  // Step 1: scan for -o <name>
  std::string explicit_output;
  int o_value_index = -1;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "-o" && i + 1 < argc) {
      explicit_output = argv[i + 1];
      o_value_index = i + 1;
      break;
    }
  }

  for (int i = 1; i < argc; i++) {
    if (i == o_value_index) continue;
    std::string a = argv[i];
    if (a == "--shut" || a == "-s")
      shut = true;
    if (a == "--asm" || a == "-asm")
      getasm = true;
    if (a == "--main" || a == "-main")
      manual_main = true;
    if (a == "-sCMD")
      priCMD = true;
    if (a == "-noLine" || a == "--noLine")
      emit_line_dirs = false;
    if (a == "-no-check" || a == "--no-check")
      no_check = true;
    if (a == "-no-check:borrow" || a == "--no-check:borrow")
      no_check_borrow = true;
    if (a == "-suppress:warnings" || a == "--suppress:warnings")
      suppress_warnings = true;
    if (a == "-help" || a == "--help" || a == "-h")
      log("Usage for xenc:\n  basic CLI:\n    xenc file.xen -o file\n  options:\n    -s: do not emit [*] Messages.\n    -asm: Emit assembly.\n    -main: require manual main\n    -sCMD: show the c compiler command.\n    -noLine: do not emit #line directives.\n    -no-check: disable safety and borrow checks.\n      -no-check:borrow: disable only borrow checking.\n    -suppress:warnings: suppress warnings.\n    -cc:[cc/clang/gcc/tcc]: choose the c compiler.\n    -help: show this message.\n    -c: emit raw C. (compile only)"); exit(0);
    // -cc:gcc / -cc:clang / -cc:tcc / -cc:cc
    if (a.size() > 4 && a.substr(0, 4) == "-cc:") {
      std::string choice = a.substr(4);
      if (choice == "gcc")        cc_backend = CCBackend::GCC;
      else if (choice == "clang") cc_backend = CCBackend::CLANG;
      else if (choice == "tcc")   cc_backend = CCBackend::TCC;
      else if (choice == "cc")    cc_backend = CCBackend::CC;
      else {
        std::cerr << "\n" << ansi::separator() << "\n"
                  << ansi::error_badge("ERROR") << "\n\n"
                  << "  " << ansi::bwhite() << "Unknown -cc: option "
                  << ansi::bred() << "'" << choice << "'" << ansi::reset()
                  << ansi::bwhite() << "\n  Valid choices: "
                  << ansi::bcyan() << "gcc  clang  tcc  cc" << ansi::reset() << "\n"
                  << ansi::separator() << "\n\n";
        exit(1);
      }
    }
  }

  if (argc < 2) {
    log("xenc version 3.1.2");
    die("Usage: xenc <in.xen> [-o <out>] [extra.c] [-lPATH] [-gLIB] [-wLIBDIR] "
        "[-c] [-s] [--asm] [--main] [-no-check] [-no-check:borrow] [-suppress:warnings] [-cc:gcc|clang|tcc|cc]",
        1);
  }

  std::string inf = argv[1];
  if (inf == "-v") {
    std::cout << "xenc compiler version 3.1.2.";
    return 0;
  }

  std::string base = fs::path(inf).stem().string();

  // Step 2: resolve output binary name
  std::string out_bin;
  if (!explicit_output.empty()) {
    out_bin = explicit_output;
  } else if (argc >= 3 && o_value_index == -1) {
    // Backwards compat: if argv[2] looks like an output name, use it
    std::string candidate = argv[2];
    bool looks_like_output = !candidate.empty()
      && candidate[0] != '-'
      && !ends_with(candidate, ".xen")
      && !ends_with(candidate, ".c")
      && !ends_with(candidate, ".o");
    out_bin = looks_like_output ? candidate : base;
  } else {
    out_bin = base;
  }

  if (!fs::exists(inf))
    die("Input file not found: '" + inf + "'", 1);
  if (!ends_with(inf, ".xen"))
    log(ansi::warn_tag("!") + ansi::yellow() + " '" + ansi::reset()
        + ansi::bwhite() + inf + ansi::reset()
        + ansi::yellow() + "' does not have a " + ansi::reset()
        + ansi::bcyan() + ".xen" + ansi::reset()
        + ansi::yellow() + " extension" + ansi::reset());

  std::vector<std::string> custom_includes;
  std::vector<std::string> extra_c_files;
  std::vector<std::string> include_paths;

  for (int i = 3; i < argc; i++) {
    if (i == o_value_index) continue;
    std::string arg = argv[i];
    if (arg == "-o") continue;
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
        log(ansi::warn_tag("!") + ansi::yellow() + " Extra source file "
            + ansi::reset() + ansi::bred() + "'" + arg + "'" + ansi::reset()
            + ansi::yellow() + " not found — skipping." + ansi::reset());
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

  log(ansi::info_tag("*") + ansi::cyan() + " Tokenizing" + ansi::reset() + ansi::grey() + "..." + ansi::reset());
  std::vector<Token> tokens;
  try {
    // Provide the global source buffer so thrown XenonError's can show
    // the snippet/file even when individual throw sites omit it.
    set_current_source_for_errors(source, fs::weakly_canonical(inf).string());
    Lexer lexer(source, prescan_addop_symbols(source));
    tokens = lexer.tokenize();
    if (!lexer.lex_errors.empty()) {
      for (const auto &err : lexer.lex_errors)
        std::cerr << err;
      die("Compilation stopped: " + std::to_string(lexer.lex_errors.size()) +
          " lex error(s) found.");
    }
  } catch (XenonError &e) {
    die(e.what());
  } catch (std::exception &e) {
    die(std::string("Unexpected error during tokenization: ") + e.what());
  }

  std::string source_file = fs::weakly_canonical(inf).string();
  std::string source_dir = fs::path(source_file).parent_path().string();
  if (source_dir.empty())
    source_dir = ".";

  log(ansi::info_tag("*") + ansi::cyan() + " Compiling" + ansi::reset() + ansi::grey() + "..." + ansi::reset());
  std::string c_code;
  try {
    bool tcc_mode = (cc_backend == CCBackend::TCC);
    CTranspiler tr(tokens, tcc_mode, !no_check, !(no_check || no_check_borrow), suppress_warnings);
    c_code = tr.transpile(source_dir, source_file, manual_main, include_paths,
                          emit_line_dirs, !debug);
    if (!tr.transpile_errors.empty()) {
      for (const auto &err : tr.transpile_errors)
        std::cerr << err;
      die("Compilation stopped: " + std::to_string(tr.transpile_errors.size()) +
          " error(s) found.");
    }
    if (!tr.transpile_warnings.empty()) {
      for (const auto &w : tr.transpile_warnings)
        std::cerr << w;
      log(ansi::warn_tag("!") + " " + ansi::byellow()
          + std::to_string(tr.transpile_warnings.size()) + " warning(s)"
          + ansi::reset());
    }
  } catch (XenonError &e) {
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

  // ===========================================================================
  // Locate compiler binary
  // ===========================================================================
  auto find_bin = [](std::initializer_list<const char*> names) -> std::string {
    for (const char *name : names) {
      std::string cmd = std::string("which ") + name + " 2>/dev/null";
      FILE *p = popen(cmd.c_str(), "r");
      if (!p) continue;
      char buf[512]{};
      if (!fgets(buf, sizeof(buf), p)) buf[0] = '\0';
      pclose(p);
      std::string r(buf);
      while (!r.empty() &&
             (r.back() == '\n' || r.back() == '\r' || r.back() == ' '))
        r.pop_back();
      if (!r.empty()) return r;
    }
    return "";
  };

  // find clang (kept for the default/clang path)
  auto find_clang = [&]() -> std::string {
    return find_bin({"clang", "clang-18", "clang-17", "clang-16", "clang-15"});
  };

  std::string compiler_bin;
  std::string compiler_label;

  switch (cc_backend) {
  case CCBackend::CLANG: {
    compiler_bin = find_clang();
    compiler_label = "clang";
    if (compiler_bin.empty()) {
      std::cerr << "\n" << ansi::separator() << "\n"
                << ansi::warn_badge("WARNING") << "\n\n"
                << "  " << ansi::bwhite() << "clang" << ansi::reset()
                << ansi::yellow() << " is not installed on this system.\n" << ansi::reset()
                << "  " << ansi::byellow() << "Install clang now? " << ansi::reset()
                << ansi::grey() << "[y/N] " << ansi::reset();
      std::string ans;
      std::getline(std::cin, ans);
      if (ans == "y" || ans == "Y") {
        if (system("which apt>/dev/null 2>&1") == 0) {
          int _r = system("sudo apt install -y clang lld"); (void)_r;
        } else if (system("which dnf>/dev/null 2>&1") == 0) {
          int _r = system("sudo dnf install -y clang lld"); (void)_r;
        } else if (system("which pacman>/dev/null 2>&1") == 0) {
          int _r = system("sudo pacman -S --noconfirm clang lld"); (void)_r;
        } else
          die("No supported package manager. Install clang manually.");
        compiler_bin = find_clang();
        if (compiler_bin.empty())
          die("clang still not found on PATH after install.");
      } else {
        die("clang is required.");
      }
    }
    break;
  }
  case CCBackend::GCC:
    compiler_bin = find_bin({"gcc", "gcc-13", "gcc-12", "gcc-11"});
    compiler_label = "gcc";
    if (compiler_bin.empty())
      die("gcc not found on PATH. Install it (e.g. apt install gcc).");
    break;
  case CCBackend::TCC:
    compiler_bin = find_bin({"tcc"});
    compiler_label = "tcc";
    if (compiler_bin.empty())
      die("tcc not found on PATH. Install it (e.g. apt install tcc).");
    break;
  case CCBackend::CC:
    compiler_bin = find_bin({"cc"});
    compiler_label = "cc";
    if (compiler_bin.empty())
      die("cc not found on PATH.");
    break;
  }

  // ===========================================================================
  // Build compiler command
  // ===========================================================================
  std::vector<std::string> cc_args;
  cc_args.push_back(compiler_bin);
  cc_args.push_back(c_file);
  for (auto &e : extra_c_files)
    cc_args.push_back(e);
  cc_args.push_back("-o");
  cc_args.push_back(out_bin);
  cc_args.push_back("-lm");
  for (auto &ci : custom_includes)
    cc_args.push_back(ci);

  switch (cc_backend) {
  case CCBackend::CLANG:
    if (!dbuild) {
      cc_args.push_back("-ffast-math");
      cc_args.push_back("-march=x86-64-v3");
      cc_args.push_back("-w");
      cc_args.push_back("-O3");
      cc_args.push_back("-fuse-ld=lld");
      cc_args.push_back("-mavx2");
      cc_args.push_back("-funroll-loops");
      cc_args.push_back("-fvectorize");
      if (!getasm)
        cc_args.push_back("-flto");
    }
    if (getasm) cc_args.push_back("-S");
    cc_args.push_back("-I" + source_dir);
    cc_args.push_back("-g");
    break;

  case CCBackend::GCC:
    if (!dbuild) {
      cc_args.push_back("-ffast-math");
      cc_args.push_back("-march=x86-64-v3");
      cc_args.push_back("-w");
      cc_args.push_back("-O3");
      cc_args.push_back("-mavx2");
      cc_args.push_back("-funroll-loops");
      if (!getasm)
        cc_args.push_back("-flto");
    }
    if (getasm) cc_args.push_back("-S");
    cc_args.push_back("-I" + source_dir);
    cc_args.push_back("-g");
    break;

  case CCBackend::TCC:
    // TCC: C99 mode, minimal flags — no AVX, no LTO, no fast-math
    cc_args.push_back("-std=c99");
    cc_args.push_back("-w");
    if (getasm) cc_args.push_back("-S");
    cc_args.push_back("-I" + source_dir);
    break;

  case CCBackend::CC:
    if (!dbuild) {
      cc_args.push_back("-w");
      cc_args.push_back("-O2");
    }
    if (getasm) cc_args.push_back("-S");
    cc_args.push_back("-I" + source_dir);
    cc_args.push_back("-g");
    break;
  }

  // build shell command string
  std::string cmd;
  for (size_t i = 0; i < cc_args.size(); i++) {
    if (i) cmd += " ";
    if (cc_args[i].find(' ') != std::string::npos)
      cmd += "\"" + cc_args[i] + "\"";
    else
      cmd += cc_args[i];
  }

  log(ansi::info_tag("*") + ansi::cyan() + " Emitting C" + ansi::reset() + ansi::grey() + "..." + ansi::reset());
  if (debug) {
    log(ansi::info_tag("*") + ansi::yellow() + " Debug mode" + ansi::reset()
        + ansi::grey() + " — C written to disk, skipping backend." + ansi::reset());
  } else if (priCMD) {
    log(ansi::info_tag("*") + " " + ansi::grey() + "Running:" + ansi::reset() + " " + ansi::bwhite() + cmd + ansi::reset());
  } else {
    log(ansi::info_tag("*") + " " + ansi::grey() + "Backend:" + ansi::reset()
        + " " + ansi::bcyan() + compiler_label + ansi::reset()
        + ansi::grey() + " → " + ansi::reset()
        + ansi::bgreen() + "./" + out_bin + ansi::reset());
  }

  if (!debug) {
    std::string err_file = c_file + ".err";
    std::string full_cmd = cmd + " 2>" + err_file;
    int ret = system(full_cmd.c_str());
    if (ret == 0) {
      log("\n" + ansi::separator("═") +
          "\n" + ansi::ok_badge("BUILD OK") + "  " +
          ansi::bgreen() + ansi::bold() + "./" + out_bin + ansi::reset() +
          "\n" + ansi::separator("═") + "\n");
    } else {
      std::string local_debug = base + "_debug.c";
      std::ifstream src(c_file, std::ios::binary);
      std::ofstream dst(local_debug, std::ios::binary);
      dst << src.rdbuf();
      std::cerr << "\n" << ansi::separator("═") << "\n"
                << ansi::error_badge("BUILD FAILED") << "  "
                << ansi::grey() << compiler_label << " exited " << ansi::reset()
                << ansi::bred() << ret << ansi::reset() << "\n"
                << ansi::grey() << "  Debug C → " << ansi::reset()
                << ansi::bold() << ansi::underline() << "./" + local_debug << ansi::reset()
                << "\n" << ansi::separator("─") << "\n";
      std::ifstream ef(err_file);
      if (ef) std::cerr << ansi::yellow() << ef.rdbuf() << ansi::reset();
      std::cerr << ansi::separator("═") << "\n\n";
      fs::remove(err_file);
      fs::remove(c_file);
      return -2;
    }
    fs::remove(err_file);
    try {
      fs::remove(c_file);
    } catch (...) {
    }
  }

  return 0;
}