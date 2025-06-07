#pragma once

#include "table.h"
#include <unordered_map>
#include <memory>

class DatabaseEngine {
private:
    std::unordered_map<std::string, std::unique_ptr<Table>> tables;
    
public:
    DatabaseEngine() = default;
    ~DatabaseEngine() = default;
    
    // Table management
    void createTable(const std::string& table_name);
    bool hasTable(const std::string& table_name) const;
    Table* getTable(const std::string& table_name);
    void dropTable(const std::string& table_name);
    std::vector<std::string> listTables() const;
    
    // Database operations
    void addColumn(const std::string& table_name, const std::string& column_name, const std::string& type);
    void insertInto(const std::string& table_name, const std::vector<std::string>& values);
    void select(const std::string& table_name, 
                const std::vector<std::string>& columns = {},
                const std::string& where_clause = "") const;
    
    // Utility functions
    void showTables() const;
    void describeTable(const std::string& table_name) const;
    void executeQuery(const std::string& query);
    
    // Database info
    size_t getTableCount() const { return tables.size(); }
    void printDatabaseInfo() const;
};
