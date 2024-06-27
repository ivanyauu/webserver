// An nginx config file parser.
//
// See:
//   http://wiki.nginx.org/Configuration
//   http://blog.martinfjordvald.com/2010/07/nginx-primer/
//
// How Nginx does it:
//   http://lxr.nginx.org/source/src/core/ngx_conf_file.c

#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "config_parser.h"

std::string NginxConfig::ToString(int depth) {
  std::string serialized_config;
  for (const auto& statement : statements_) {
    serialized_config.append(statement->ToString(depth));
  }
  return serialized_config;
}

std::string NginxConfig::FindPortNumber() {
  for (const auto& statement : statements_) {
    // Check if the statement contains "port"
    for (const auto& token : statement->tokens_) {
      if (token == "port") {
        // Check the next token for the port number
        if (!statement->tokens_.empty() && statement->tokens_.size() > 1) {
          return statement->tokens_[1];
        }
      }
    }
    
    // If the statement has a child block, recursively search within it
    if (statement->child_block_) {
      std::string port = statement->child_block_->FindPortNumber();
      if (!port.empty()) {
        return port; // Return the port number if found
      }
    }
  }
  return "";
}

// Returns a list of (path, handler_name, args, ...) pairs
std::vector<HandlerConfig> NginxConfig::GetRequestHandlers() {
  std::vector<HandlerConfig> requestHandlers;
  for (const auto& statement : statements_) {
    if (!statement->tokens_.empty() && statement->tokens_[0] == "location" && statement->tokens_.size() > 2) {
      HandlerConfig handlerConfig;
      handlerConfig.path = statement->tokens_[1];
      handlerConfig.name = statement->tokens_[2];
      handlerConfig.root = statement->child_block_->GetRoot();
      for (const auto& rh : requestHandlers) {
        if (statement->tokens_[1] == rh.path) {
          std::cerr << "Please ensure serving locations are unique" << std::endl;
          return {};
        }
      }
      requestHandlers.push_back(handlerConfig);
      
    }
  }
  return requestHandlers;
}

std::string NginxConfig::GetRoot() {
  bool foundRoot = false;
  for (const auto& statement: statements_) {
    for (const auto& token: statement->tokens_) {
      if(token == "root" || token == "data_path") {
        foundRoot = true;
      } else if(foundRoot) {
        if (!token.empty() && token[0] == '.') {
          std::filesystem::path basePath = std::filesystem::current_path();
          std::string absolutePath = (basePath / token).lexically_normal().string();
          return absolutePath;
        }
        return token;
      }
    }
  }
  return "";
}

// Find a block with the given name and return a pointer to it
NginxConfig* NginxConfig::GetBlock(const std::string& blockName, const std::string& blockParameter) {
  for (const auto& statement : statements_) {
    if(blockParameter == "") {
      if (!statement->tokens_.empty() && statement->tokens_[0] == blockName && statement->child_block_) {
        return statement->child_block_.get();
      }
    } else {
      if (statement->tokens_.size() >= 2 && statement->tokens_[0] == blockName && statement->tokens_[1] == blockParameter && statement->child_block_) {
        return statement->child_block_.get();
      }
    }
  }
  return nullptr;
}

std::string NginxConfigStatement::ToString(int depth) {
  std::string serialized_statement;
  for (int i = 0; i < depth; ++i) {
    serialized_statement.append("  ");
  }
  for (unsigned int i = 0; i < tokens_.size(); ++i) {
    if (i != 0) {
      serialized_statement.append(" ");
    }
    serialized_statement.append(tokens_[i]);
  }
  if (child_block_.get() != nullptr) {
    serialized_statement.append(" {\n");
    serialized_statement.append(child_block_->ToString(depth + 1));
    for (int i = 0; i < depth; ++i) {
      serialized_statement.append("  ");
    }
    serialized_statement.append("}");
  } else {
    serialized_statement.append(";");
  }
  serialized_statement.append("\n");
  return serialized_statement;
}

bool IsValidSeparator(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\t' ||
            c == ';' || c == '{' || c == '}';
}

const char* NginxConfigParser::TokenTypeAsString(TokenType type) {
  switch (type) {
    case TOKEN_TYPE_START:         return "TOKEN_TYPE_START";
    case TOKEN_TYPE_NORMAL:        return "TOKEN_TYPE_NORMAL";
    case TOKEN_TYPE_START_BLOCK:   return "TOKEN_TYPE_START_BLOCK";
    case TOKEN_TYPE_END_BLOCK:     return "TOKEN_TYPE_END_BLOCK";
    case TOKEN_TYPE_COMMENT:       return "TOKEN_TYPE_COMMENT";
    case TOKEN_TYPE_STATEMENT_END: return "TOKEN_TYPE_STATEMENT_END";
    case TOKEN_TYPE_EOF:           return "TOKEN_TYPE_EOF";
    case TOKEN_TYPE_ERROR:         return "TOKEN_TYPE_ERROR";
    default:                       return "Unknown token type";
  }
}

NginxConfigParser::TokenType NginxConfigParser::ParseToken(std::istream* input,
                                                           std::string* value) {
  TokenParserState state = TOKEN_STATE_INITIAL_WHITESPACE;
  while (input->good()) {
    const char c = input->get();
    if (!input->good()) {
      break;
    }
    switch (state) {
      case TOKEN_STATE_INITIAL_WHITESPACE:
        switch (c) {
          case '{':
            *value = c;
            return TOKEN_TYPE_START_BLOCK;
          case '}':
            *value = c;
            return TOKEN_TYPE_END_BLOCK;
          case '#':
            *value = c;
            state = TOKEN_STATE_TOKEN_TYPE_COMMENT;
            continue;
          case '"':
            *value = c;
            state = TOKEN_STATE_DOUBLE_QUOTE;
            continue;
          case '\'':
            *value = c;
            state = TOKEN_STATE_SINGLE_QUOTE;
            continue;
          case ';':
            *value = c;
            return TOKEN_TYPE_STATEMENT_END;
          case ' ':
          case '\t':
          case '\n':
          case '\r':
            continue;
          default:
            *value += c;
            state = TOKEN_STATE_TOKEN_TYPE_NORMAL;
            continue;
        }
      case TOKEN_STATE_SINGLE_QUOTE:
        *value += c;
        if (c == '\\') {
          state = TOKEN_STATE_ESCAPE_IN_SINGLE_QUOTE;
          continue;
        }
        if (c == '\'') {
          const char next_char = input->get();
          if (!IsValidSeparator(next_char)) {
            return TOKEN_TYPE_ERROR;
          }
          input->unget();
          return TOKEN_TYPE_NORMAL;
        }
        continue;
      case TOKEN_STATE_DOUBLE_QUOTE:
        *value += c;
        if (c == '\\') {
          state = TOKEN_STATE_ESCAPE_IN_DOUBLE_QUOTE;
          continue;
        }
        if (c == '"') {
          const char next_char = input->get();
          if (!IsValidSeparator(next_char)) {
            return TOKEN_TYPE_ERROR;
          }
          input->unget();
          return TOKEN_TYPE_NORMAL;
        }
        continue;
      case TOKEN_STATE_ESCAPE_IN_SINGLE_QUOTE:
        *value += c;
        state = TOKEN_STATE_SINGLE_QUOTE;
        continue;
      case TOKEN_STATE_ESCAPE_IN_DOUBLE_QUOTE:
        *value += c;
        state = TOKEN_STATE_DOUBLE_QUOTE;
        continue;
      case TOKEN_STATE_TOKEN_TYPE_COMMENT:
        if (c == '\n' || c == '\r') {
          return TOKEN_TYPE_COMMENT;
        }
        *value += c;
        continue;
      case TOKEN_STATE_TOKEN_TYPE_NORMAL:
        if (IsValidSeparator(c)) {
          input->unget();
          return TOKEN_TYPE_NORMAL;
        }
        *value += c;
        continue;
    }
  }
  // If we get here, we reached the end of the file.
  if (state == TOKEN_STATE_SINGLE_QUOTE ||
      state == TOKEN_STATE_DOUBLE_QUOTE ||
      state == TOKEN_STATE_ESCAPE_IN_SINGLE_QUOTE ||
      state == TOKEN_STATE_ESCAPE_IN_DOUBLE_QUOTE
  ) {
    return TOKEN_TYPE_ERROR;
  }
  return TOKEN_TYPE_EOF;
}
bool NginxConfigParser::Parse(std::istream* config_file, NginxConfig* config) {
  std::stack<NginxConfig*> config_stack;
  config_stack.push(config);
  TokenType last_token_type = TOKEN_TYPE_START;
  TokenType token_type;
  while (true) {
    std::string token;
    token_type = ParseToken(config_file, &token);
    printf ("%s: %s\n", TokenTypeAsString(token_type), token.c_str());
    if (token_type == TOKEN_TYPE_ERROR) {
      break;
    }
    if (token_type == TOKEN_TYPE_COMMENT) {
      // Skip comments.
      continue;
    }
    if (token_type == TOKEN_TYPE_START) {
      // Error.
      break;
    } else if (token_type == TOKEN_TYPE_NORMAL) {
      if (last_token_type == TOKEN_TYPE_START ||
          last_token_type == TOKEN_TYPE_STATEMENT_END ||
          last_token_type == TOKEN_TYPE_START_BLOCK ||
          last_token_type == TOKEN_TYPE_END_BLOCK ||
          last_token_type == TOKEN_TYPE_NORMAL) {
        if (last_token_type != TOKEN_TYPE_NORMAL) {
          config_stack.top()->statements_.emplace_back(
              new NginxConfigStatement);
        }
        config_stack.top()->statements_.back().get()->tokens_.push_back(
            token);
      } else {
        // Error.
        break;
      }
    } else if (token_type == TOKEN_TYPE_STATEMENT_END) {
      if (last_token_type != TOKEN_TYPE_NORMAL) {
        // Error.
        break;
      }
    } else if (token_type == TOKEN_TYPE_START_BLOCK) {
      if (last_token_type != TOKEN_TYPE_NORMAL) {
        // Error.
        break;
      }
      NginxConfig* const new_config = new NginxConfig;
      config_stack.top()->statements_.back().get()->child_block_.reset(
          new_config);
      config_stack.push(new_config);
    } else if (token_type == TOKEN_TYPE_END_BLOCK) {
      if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
          last_token_type != TOKEN_TYPE_START_BLOCK &&
          last_token_type != TOKEN_TYPE_END_BLOCK) {
        // Error.
        break;
      }
      config_stack.pop();
      if (config_stack.empty()) {
        // Error. Main config context was popped off of the stack.
        printf("Unmatched closing brace");
        return false;
      }
    } else if (token_type == TOKEN_TYPE_EOF) {
      if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
          last_token_type != TOKEN_TYPE_END_BLOCK) {
        // Error.
        break;
      }
      if (config_stack.size() > 1) {
        // Error. Only main config context should be on the stack.
        printf("Unmatched open brace");
        return false;
      }
      return true;
    } else {
      // Error. Unknown token.
      break;
    }
    last_token_type = token_type;
  }
  printf ("Bad transition from %s to %s\n",
          TokenTypeAsString(last_token_type),
          TokenTypeAsString(token_type));
  return false;
}
bool NginxConfigParser::Parse(const char* file_name, NginxConfig* config) {
  std::ifstream config_file;
  config_file.open(file_name);
  if (!config_file.good()) {
    printf ("Failed to open config file: %s\n", file_name);
    return false;
  }
  const bool return_value =
      Parse(dynamic_cast<std::istream*>(&config_file), config);
  config_file.close();
  return return_value;
}
