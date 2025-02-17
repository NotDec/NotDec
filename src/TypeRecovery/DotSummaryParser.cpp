#include "TypeRecovery/DotSummaryParser.h"

namespace notdec::retypd {

std::vector<std::pair<std::string, DotGraph>>
parseDotSummary(const std::string &input) {
  std::vector<std::pair<std::string, DotGraph>> Ret;
  DotParser parser(Ret, input);
  parser.parse();
  return Ret;
}

using Token = Lexer::Token;

Token Lexer::getNextToken() {
  skipWhitespace();
  if (pos >= input.size())
    return {TOK_EOF, ""};

  char c = input[pos];
  if (c == '"') {
    return parseQuotedID();
  } else if (isalpha(c) || c == '_') {
    return parseID();
  } else if (c == '{') {
    pos++;
    return {TOK_LBRACE, "{"};
  } else if (c == '}') {
    pos++;
    return {TOK_RBRACE, "}"};
  } else if (c == '[') {
    pos++;
    return {TOK_LBRACKET, "["};
  } else if (c == ']') {
    pos++;
    return {TOK_RBRACKET, "]"};
  } else if (c == '=') {
    pos++;
    return {TOK_EQUAL, "="};
  } else if (c == ',') {
    pos++;
    return {TOK_COMMA, ","};
  } else if (c == ';') {
    pos++;
    return {TOK_SEMICOLON, ";"};
  } else if (c == '-') {
    if (pos + 1 < input.size() && input[pos + 1] == '>') {
      pos += 2;
      return {TOK_ARROW, "->"};
    } else {
      pos++;
      return {TOK_ERROR, "-"};
    }
  } else if (c == '/') {
    // TOK_COMMENT
    if (pos + 1 < input.size() && input[pos + 1] == '/') {
      pos += 2;
      auto begin = pos;
      while (pos < input.size() && input[pos] != '\n')
        pos++;
      return {TOK_COMMENT,
              std::string(input.begin() + begin, input.begin() + pos)};
    } else {
      pos++;
      return {TOK_ERROR, std::string(1, c)};
    }
  } else {
    pos++;
    return {TOK_ERROR, std::string(1, c)};
  }
}

void Lexer::skipWhitespace() {
  while (pos < input.size() && isspace(input[pos]))
    pos++;
}

Token Lexer::parseID() {
  size_t start = pos;
  while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_'))
    pos++;
  std::string id = input.substr(start, pos - start);
  if (id == "digraph")
    return {TOK_DIGRAPH, id};
  return {TOK_ID, id};
}

Token Lexer::parseQuotedID() {
  pos++; // skip opening "
  std::string value;
  while (pos < input.size() && input[pos] != '"') {
    if (input[pos] == '\\') {
      pos++;
      if (pos >= input.size())
        break;
      char c = input[pos];
      switch (c) {
      case 'n':
        value += '\n';
        break;
      case 't':
        value += '\t';
        break;
      case '\\':
        value += '\\';
        break;
      case '"':
        value += '"';
        break;
      case '|':
        value += '|';
        break;
      case '{':
        value += '{';
        break;
      case '}':
        value += '}';
        break;
      case 'l':
        value += "\\l";
        break;
      default:
        value += '\\';
        value += c;
        break;
      }
      pos++;
    } else {
      value += input[pos++];
    }
  }
  if (pos < input.size() && input[pos] == '"')
    pos++;
  return {TOK_ID, value};
}

void DotParser::eat(Lexer::TokenType expected) {
  if (currentToken.type == expected) {
    currentToken = lexer.getNextToken();
  } else {
    assert(false && "Syntax error");
  }
}

std::string DotParser::parseNodeId() {
  std::string id = currentToken.lexeme;
  eat(Lexer::TOK_ID);
  return id;
}

void DotParser::parseAttrList(std::map<std::string, std::string> &attrs) {
  while (currentToken.type == Lexer::TOK_LBRACKET) {
    eat(Lexer::TOK_LBRACKET);
    if (currentToken.type != Lexer::TOK_RBRACKET) {
      parseAList(attrs);
    }
    eat(Lexer::TOK_RBRACKET);
  }
}

void DotParser::parseAList(std::map<std::string, std::string> &attrs) {
  while (true) {
    std::string key = currentToken.lexeme;
    eat(Lexer::TOK_ID);
    eat(Lexer::TOK_EQUAL);
    std::string value = currentToken.lexeme;
    eat(Lexer::TOK_ID);
    attrs[key] = value;
    if (currentToken.type != Lexer::TOK_COMMA)
      break;
    eat(Lexer::TOK_COMMA);
  }
}

DotNode DotParser::parseNodeStmt() {
  std::string nodeId = parseNodeId();
  std::map<std::string, std::string> attrs;
  if (currentToken.type == Lexer::TOK_LBRACKET) {
    parseAttrList(attrs);
  }
  DotNode node;
  node.id = nodeId;
  node.attrs = attrs;
  if (attrs.count("shape") && attrs["shape"] == "record" &&
      attrs.count("label")) {
    std::string label = attrs["label"];
    if (!label.empty() && label.front() == '{' && label.back() == '}') {
      label = label.substr(1, label.size() - 2);
      std::istringstream iss(label);
      std::string part;
      std::vector<std::string> parts;
      while (std::getline(iss, part, '|')) {
        parts.push_back(part);
      }
      if (!parts.empty())
        node.nodeLabel = parts[0];
      if (parts.size() > 1)
        node.identifierLabel = parts[1];
      if (parts.size() > 2)
        node.nodeDescription = parts[2];
      assert(parts.size() <= 3);
    }
  }
  return node;
}

DotEdge DotParser::parseEdgeStmt() {
  std::string from = parseNodeId();
  eat(Lexer::TOK_ARROW);
  std::string to = parseNodeId();
  std::map<std::string, std::string> attrs;
  if (currentToken.type == Lexer::TOK_LBRACKET) {
    parseAttrList(attrs);
  }
  DotEdge edge;
  edge.from = from;
  edge.to = to;
  edge.attrs = attrs;
  return edge;
}

DotGraph DotParser::parseGraph() {
  DotGraph graph;
  eat(Lexer::TOK_DIGRAPH);
  if (currentToken.type == Lexer::TOK_ID) {
    graph.name = currentToken.lexeme;
  }
  eat(Lexer::TOK_LBRACE);
  while (currentToken.type != Lexer::TOK_RBRACE) {
    if (currentToken.type == Lexer::TOK_ID) {
      std::string nodeId = currentToken.lexeme;
      Lexer::Token next = lexer.getNextToken();
      if (next.type == Lexer::TOK_ARROW) {
        currentToken = Lexer::Token(next.type, next.lexeme);
        graph.edges.push_back(parseEdgeStmt());
      } else {
        currentToken = Lexer::Token(Lexer::TOK_ID, nodeId);
        graph.nodes.push_back(parseNodeStmt());
      }
    }
    if (currentToken.type == Lexer::TOK_SEMICOLON) {
      eat(Lexer::TOK_SEMICOLON);
    }
  }
  eat(Lexer::TOK_RBRACE);
  return graph;
}

std::string DotParser::parseComment() {
  if (currentToken.type == Lexer::TOK_COMMENT) {
    std::string comment = currentToken.lexeme;
    currentToken = lexer.getNextToken();
    return comment;
  } else {
    assert(false && "Syntax error, expect comment.");
  }
}

void DotParser::parse() {
  while (currentToken.type != Lexer::TOK_EOF) {
    std::string comment = parseComment();
    DotGraph graph = parseGraph();
    summaryGraphs.push_back({comment, graph});
  }
}

} // namespace notdec::retypd
