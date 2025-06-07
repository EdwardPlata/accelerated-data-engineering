#include "query_parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> QueryParser::tokenize(const std::string& query) const {
    std::vector<std::string> tokens;
    std::istringstream iss(query);
    std::string token;
    
    while (iss >> token) {
        // Handle parentheses and commas
        std::string current = token;
        
        // Remove trailing punctuation but keep track of it
        while (!current.empty() && (current.back() == ',' || current.back() == ';' || current.back() == ')')) {
            char punct = current.back();
            current = current.substr(0, current.length() - 1);
            
            if (!current.empty()) {
                tokens.push_back(current);
            }
            
            if (punct == ',' || punct == ')') {
                tokens.push_back(std::string(1, punct));
            }
            
            current = "";
            break;
        }
        
        // Handle leading parentheses
        if (!current.empty() && current.front() == '(') {
            tokens.push_back("(");
            current = current.substr(1);
        }
        
        if (!current.empty()) {
            tokens.push_back(current);
        }
    }
    
    return tokens;
}

std::string QueryParser::toLower(const std::string& str) const {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

ParsedQuery QueryParser::parse(const std::string& query) const {
    ParsedQuery parsed_query;
    auto tokens = tokenize(query);
    
    if (tokens.empty()) {
        return parsed_query;
    }
    
    std::string first_token = toLower(tokens[0]);
    
    if (first_token == "create") {
        if (tokens.size() > 1 && toLower(tokens[1]) == "table") {
            parsed_query.type = QueryType::CREATE_TABLE;
            parseCreateTable(tokens, parsed_query);
        }
    } else if (first_token == "insert") {
        parsed_query.type = QueryType::INSERT;
        parseInsert(tokens, parsed_query);
    } else if (first_token == "select") {
        parsed_query.type = QueryType::SELECT;
        parseSelect(tokens, parsed_query);
    } else if (first_token == "drop") {
        if (tokens.size() > 1 && toLower(tokens[1]) == "table") {
            parsed_query.type = QueryType::DROP_TABLE;
            parseDropTable(tokens, parsed_query);
        }
    } else if (first_token == "show") {
        if (tokens.size() > 1 && toLower(tokens[1]) == "tables") {
            parsed_query.type = QueryType::SHOW_TABLES;
        }
    } else if (first_token == "describe" || first_token == "desc") {
        parsed_query.type = QueryType::DESCRIBE;
        parseDescribe(tokens, parsed_query);
    }
    
    return parsed_query;
}

void QueryParser::parseCreateTable(const std::vector<std::string>& tokens, ParsedQuery& query) const {
    // CREATE TABLE table_name (col1 type1, col2 type2, ...)
    if (tokens.size() < 4) {
        throw std::runtime_error("Invalid CREATE TABLE syntax");
    }
    
    query.table_name = tokens[2];
    
    // Find the opening parenthesis
    size_t start = 3;
    while (start < tokens.size() && tokens[start] != "(") {
        start++;
    }
    
    if (start >= tokens.size()) {
        throw std::runtime_error("Missing column definitions in CREATE TABLE");
    }
    
    // Parse column definitions
    start++; // Skip opening parenthesis
    for (size_t i = start; i < tokens.size(); ++i) {
        if (tokens[i] == ")") {
            break;
        }
        if (tokens[i] == ",") {
            continue;
        }
        
        if (i + 1 < tokens.size() && tokens[i + 1] != "," && tokens[i + 1] != ")") {
            std::string col_name = tokens[i];
            std::string col_type = toLower(tokens[i + 1]);
            query.columns.emplace_back(col_name, col_type);
            i++; // Skip the type token
        }
    }
}

void QueryParser::parseInsert(const std::vector<std::string>& tokens, ParsedQuery& query) const {
    // INSERT INTO table_name VALUES (val1, val2, ...)
    if (tokens.size() < 5) {
        throw std::runtime_error("Invalid INSERT syntax");
    }
    
    if (toLower(tokens[1]) != "into") {
        throw std::runtime_error("Expected 'INTO' after 'INSERT'");
    }
    
    query.table_name = tokens[2];
    
    if (toLower(tokens[3]) != "values") {
        throw std::runtime_error("Expected 'VALUES' in INSERT statement");
    }
    
    // Find values in parentheses
    size_t start = 4;
    while (start < tokens.size() && tokens[start] != "(") {
        start++;
    }
    
    if (start >= tokens.size()) {
        throw std::runtime_error("Missing values in INSERT statement");
    }
    
    start++; // Skip opening parenthesis
    for (size_t i = start; i < tokens.size(); ++i) {
        if (tokens[i] == ")") {
            break;
        }
        if (tokens[i] == ",") {
            continue;
        }
        
        // Remove quotes if present
        std::string value = tokens[i];
        if (value.length() >= 2 && 
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }
        
        query.values.push_back(value);
    }
}

void QueryParser::parseSelect(const std::vector<std::string>& tokens, ParsedQuery& query) const {
    // SELECT col1, col2, ... FROM table_name [WHERE condition]
    // SELECT * FROM table_name [WHERE condition]
    
    if (tokens.size() < 4) {
        throw std::runtime_error("Invalid SELECT syntax");
    }
    
    size_t from_pos = 0;
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (toLower(tokens[i]) == "from") {
            from_pos = i;
            break;
        }
    }
    
    if (from_pos == 0) {
        throw std::runtime_error("Missing 'FROM' in SELECT statement");
    }
    
    // Parse selected columns
    for (size_t i = 1; i < from_pos; ++i) {
        if (tokens[i] == ",") {
            continue;
        }
        if (tokens[i] == "*") {
            query.selected_columns.clear(); // Empty means select all
            break;
        } else {
            query.selected_columns.push_back(tokens[i]);
        }
    }
    
    // Parse table name
    if (from_pos + 1 >= tokens.size()) {
        throw std::runtime_error("Missing table name after 'FROM'");
    }
    query.table_name = tokens[from_pos + 1];
    
    // Parse WHERE clause if present
    size_t where_pos = 0;
    for (size_t i = from_pos + 2; i < tokens.size(); ++i) {
        if (toLower(tokens[i]) == "where") {
            where_pos = i;
            break;
        }
    }
    
    if (where_pos > 0 && where_pos + 3 < tokens.size()) {
        // Simple WHERE clause: column operator value
        query.where_clause = tokens[where_pos + 1] + " " + 
                            tokens[where_pos + 2] + " " + 
                            tokens[where_pos + 3];
    }
}

void QueryParser::parseDropTable(const std::vector<std::string>& tokens, ParsedQuery& query) const {
    // DROP TABLE table_name
    if (tokens.size() < 3) {
        throw std::runtime_error("Invalid DROP TABLE syntax");
    }
    
    query.table_name = tokens[2];
}

void QueryParser::parseDescribe(const std::vector<std::string>& tokens, ParsedQuery& query) const {
    // DESCRIBE table_name or DESC table_name
    if (tokens.size() < 2) {
        throw std::runtime_error("Invalid DESCRIBE syntax");
    }
    
    query.table_name = tokens[1];
}
