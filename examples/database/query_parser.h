#pragma once

#include "table.h"
#include <string>
#include <vector>

enum class QueryType {
    CREATE_TABLE,
    INSERT,
    SELECT,
    DROP_TABLE,
    SHOW_TABLES,
    DESCRIBE,
    UNKNOWN
};

struct ParsedQuery {
    QueryType type;
    std::string table_name;
    std::vector<Column> columns;
    std::vector<std::string> selected_columns;
    std::vector<std::string> values;
    std::string where_clause;
    
    ParsedQuery() : type(QueryType::UNKNOWN) {}
};

class QueryParser {
private:
    std::vector<std::string> tokenize(const std::string& query) const;
    std::string toLower(const std::string& str) const;
    void parseCreateTable(const std::vector<std::string>& tokens, ParsedQuery& query) const;
    void parseInsert(const std::vector<std::string>& tokens, ParsedQuery& query) const;
    void parseSelect(const std::vector<std::string>& tokens, ParsedQuery& query) const;
    void parseDropTable(const std::vector<std::string>& tokens, ParsedQuery& query) const;
    void parseDescribe(const std::vector<std::string>& tokens, ParsedQuery& query) const;
    
public:
    ParsedQuery parse(const std::string& query) const;
};
