#include "database_engine.h"
#include "query_parser.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

void DatabaseEngine::createTable(const std::string& table_name) {
    if (hasTable(table_name)) {
        throw std::runtime_error("Table '" + table_name + "' already exists");
    }
    tables[table_name] = std::make_unique<Table>(table_name);
}

bool DatabaseEngine::hasTable(const std::string& table_name) const {
    return tables.find(table_name) != tables.end();
}

Table* DatabaseEngine::getTable(const std::string& table_name) {
    auto it = tables.find(table_name);
    if (it == tables.end()) {
        throw std::runtime_error("Table '" + table_name + "' not found");
    }
    return it->second.get();
}

void DatabaseEngine::dropTable(const std::string& table_name) {
    if (!hasTable(table_name)) {
        throw std::runtime_error("Table '" + table_name + "' not found");
    }
    tables.erase(table_name);
}

std::vector<std::string> DatabaseEngine::listTables() const {
    std::vector<std::string> table_names;
    for (const auto& pair : tables) {
        table_names.push_back(pair.first);
    }
    std::sort(table_names.begin(), table_names.end());
    return table_names;
}

void DatabaseEngine::addColumn(const std::string& table_name, const std::string& column_name, const std::string& type) {
    Table* table = getTable(table_name);
    table->addColumn(column_name, type);
}

void DatabaseEngine::insertInto(const std::string& table_name, const std::vector<std::string>& values) {
    Table* table = getTable(table_name);
    table->insertRow(values);
}

void DatabaseEngine::select(const std::string& table_name, 
                            const std::vector<std::string>& columns,
                            const std::string& where_clause) const {
    auto it = tables.find(table_name);
    if (it == tables.end()) {
        throw std::runtime_error("Table '" + table_name + "' not found");
    }
    
    const Table* table = it->second.get();
    std::vector<size_t> row_indices = table->selectRows(where_clause);
    table->printRows(row_indices, columns);
}

void DatabaseEngine::showTables() const {
    std::cout << "Tables in database:\n";
    std::cout << "+----------------+-------+\n";
    std::cout << "| Table Name     | Rows  |\n";
    std::cout << "+----------------+-------+\n";
    
    auto table_names = listTables();
    for (const auto& name : table_names) {
        auto it = tables.find(name);
        if (it != tables.end()) {
            std::cout << "| " << std::left << std::setw(14) << name 
                      << " | " << std::right << std::setw(5) << it->second->size() << " |\n";
        }
    }
    
    std::cout << "+----------------+-------+\n";
    std::cout << "(" << tables.size() << " tables)\n";
}

void DatabaseEngine::describeTable(const std::string& table_name) const {
    auto it = tables.find(table_name);
    if (it == tables.end()) {
        throw std::runtime_error("Table '" + table_name + "' not found");
    }
    
    const Table* table = it->second.get();
    const auto& columns = table->getColumns();
    
    std::cout << "Table: " << table_name << "\n";
    std::cout << "+----------------+----------------+\n";
    std::cout << "| Column Name    | Type           |\n";
    std::cout << "+----------------+----------------+\n";
    
    for (const auto& column : columns) {
        std::cout << "| " << std::left << std::setw(14) << column.name 
                  << " | " << std::left << std::setw(14) << column.type << " |\n";
    }
    
    std::cout << "+----------------+----------------+\n";
    std::cout << "(" << columns.size() << " columns, " << table->size() << " rows)\n";
}

void DatabaseEngine::executeQuery(const std::string& query) {
    try {
        QueryParser parser;
        auto parsed_query = parser.parse(query);
        
        if (parsed_query.type == QueryType::CREATE_TABLE) {
            createTable(parsed_query.table_name);
            for (const auto& col : parsed_query.columns) {
                addColumn(parsed_query.table_name, col.name, col.type);
            }
            std::cout << "Table '" << parsed_query.table_name << "' created successfully.\n";
            
        } else if (parsed_query.type == QueryType::INSERT) {
            insertInto(parsed_query.table_name, parsed_query.values);
            std::cout << "1 row inserted.\n";
            
        } else if (parsed_query.type == QueryType::SELECT) {
            select(parsed_query.table_name, parsed_query.selected_columns, parsed_query.where_clause);
            
        } else if (parsed_query.type == QueryType::DROP_TABLE) {
            dropTable(parsed_query.table_name);
            std::cout << "Table '" << parsed_query.table_name << "' dropped successfully.\n";
            
        } else if (parsed_query.type == QueryType::SHOW_TABLES) {
            showTables();
            
        } else if (parsed_query.type == QueryType::DESCRIBE) {
            describeTable(parsed_query.table_name);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void DatabaseEngine::printDatabaseInfo() const {
    std::cout << "\n=== SimpleDB Database Information ===\n";
    std::cout << "Total tables: " << tables.size() << "\n";
    
    size_t total_rows = 0;
    for (const auto& pair : tables) {
        total_rows += pair.second->size();
    }
    std::cout << "Total rows: " << total_rows << "\n";
    std::cout << "====================================\n\n";
}
