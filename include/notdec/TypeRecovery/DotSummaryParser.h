#include <algorithm>
#include <cassert>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace notdec::retypd {

struct DotNode {
  std::string id;
  std::map<std::string, std::string> attrs;
  std::string nodeLabel;
  std::string identifierLabel;
  std::string nodeDescription;
};

struct DotEdge {
  std::string from;
  std::string to;
  std::map<std::string, std::string> attrs;
};

struct DotGraph {
  std::string name;
  std::vector<DotNode> nodes;
  std::vector<DotEdge> edges;
};

class Lexer {
public:
  enum TokenType {
    TOK_DIGRAPH,
    TOK_ID,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_ARROW,
    TOK_COMMENT,
    TOK_EOF,
    TOK_ERROR
  };

  struct Token {
    TokenType type;
    std::string lexeme;
    Token(TokenType t, const std::string &l) : type(t), lexeme(l) {}
  };

  Lexer(const std::string &input) : input(input), pos(0) {}

  Token getNextToken();

private:
  std::string input;
  size_t pos;

  void skipWhitespace();

  Token parseID();

  Token parseQuotedID();
};

class DotParser {
  Lexer lexer;
  Lexer::Token currentToken;
  std::vector<std::pair<std::string, DotGraph>>& summaryGraphs;

  void eat(Lexer::TokenType expected);

  std::string parseNodeId();

  void parseAttrList(std::map<std::string, std::string> &attrs);

  void parseAList(std::map<std::string, std::string> &attrs);

  DotNode parseNodeStmt();

  DotEdge parseEdgeStmt();

public:
  DotParser(std::vector<std::pair<std::string, DotGraph>>& summary, std::string str)
      : lexer(str), currentToken(lexer.getNextToken()), summaryGraphs(summary) {}

  DotGraph parseGraph();

  std::string parseComment();

  void parse();

};

std::vector<std::pair<std::string, DotGraph>> parseDotSummary(const std::string &input);

} // namespace notdec::retypd
