#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>

// Data types supported by our database
using Value = std::variant<int, double, std::string, bool>;

// Column definition
struct Column {
    std::string name;
    std::string type;  // "int", "double", "string", "bool"
    
    Column(const std::string& n, const std::string& t) : name(n), type(t) {}
};

// Row is a collection of values
using Row = std::vector<Value>;

// Table class to store data
class Table {
private:
    std::string table_name;
    std::vector<Column> columns;
    std::vector<Row> rows;
    std::unordered_map<std::string, size_t> column_index_map;

public:
    Table(const std::string& name);
    
    // Schema operations
    void addColumn(const std::string& name, const std::string& type);
    bool hasColumn(const std::string& name) const;
    size_t getColumnIndex(const std::string& name) const;
    const std::vector<Column>& getColumns() const { return columns; }
    const std::string& getName() const { return table_name; }
    
    // Data operations
    void insertRow(const Row& row);
    void insertRow(const std::vector<std::string>& values);
    const std::vector<Row>& getRows() const { return rows; }
    
    // Query operations
    std::vector<size_t> selectRows(const std::string& where_clause = "") const;
    void printTable() const;
    void printRows(const std::vector<size_t>& row_indices, 
                   const std::vector<std::string>& selected_columns = {}) const;
    
    // Utility functions
    Value parseValue(const std::string& value_str, const std::string& type) const;
    std::string valueToString(const Value& value) const;
    bool evaluateCondition(const Row& row, const std::string& condition) const;
    
    size_t size() const { return rows.size(); }
    bool empty() const { return rows.empty(); }
};
