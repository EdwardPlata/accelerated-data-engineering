#include "table.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

Table::Table(const std::string& name) : table_name(name) {}

void Table::addColumn(const std::string& name, const std::string& type) {
    columns.emplace_back(name, type);
    column_index_map[name] = columns.size() - 1;
}

bool Table::hasColumn(const std::string& name) const {
    return column_index_map.find(name) != column_index_map.end();
}

size_t Table::getColumnIndex(const std::string& name) const {
    auto it = column_index_map.find(name);
    if (it == column_index_map.end()) {
        throw std::runtime_error("Column '" + name + "' not found");
    }
    return it->second;
}

void Table::insertRow(const Row& row) {
    if (row.size() != columns.size()) {
        throw std::runtime_error("Row size doesn't match number of columns");
    }
    rows.push_back(row);
}

void Table::insertRow(const std::vector<std::string>& values) {
    if (values.size() != columns.size()) {
        throw std::runtime_error("Number of values doesn't match number of columns");
    }
    
    Row row;
    for (size_t i = 0; i < values.size(); ++i) {
        row.push_back(parseValue(values[i], columns[i].type));
    }
    rows.push_back(row);
}

Value Table::parseValue(const std::string& value_str, const std::string& type) const {
    if (type == "int") {
        return std::stoi(value_str);
    } else if (type == "double") {
        return std::stod(value_str);
    } else if (type == "bool") {
        std::string lower = value_str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return (lower == "true" || lower == "1");
    } else {  // string
        return value_str;
    }
}

std::string Table::valueToString(const Value& value) const {
    return std::visit([](const auto& v) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
            return v ? "true" : "false";
        } else {
            return std::to_string(v);
        }
    }, value);
}

std::vector<size_t> Table::selectRows(const std::string& where_clause) const {
    std::vector<size_t> result;
    
    for (size_t i = 0; i < rows.size(); ++i) {
        if (where_clause.empty() || evaluateCondition(rows[i], where_clause)) {
            result.push_back(i);
        }
    }
    
    return result;
}

bool Table::evaluateCondition(const Row& row, const std::string& condition) const {
    // Simple condition parser: column_name operator value
    // Supports: =, !=, <, >, <=, >=
    std::istringstream iss(condition);
    std::string column_name, op, value_str;
    
    if (!(iss >> column_name >> op >> value_str)) {
        return true; // Invalid condition, return all rows
    }
    
    if (!hasColumn(column_name)) {
        return false;
    }
    
    size_t col_idx = getColumnIndex(column_name);
    const Value& row_value = row[col_idx];
    Value condition_value = parseValue(value_str, columns[col_idx].type);
    
    // Compare values based on operator
    if (op == "=") {
        return row_value == condition_value;
    } else if (op == "!=") {
        return row_value != condition_value;
    } else if (op == "<") {
        return row_value < condition_value;
    } else if (op == ">") {
        return row_value > condition_value;
    } else if (op == "<=") {
        return row_value <= condition_value;
    } else if (op == ">=") {
        return row_value >= condition_value;
    }
    
    return false;
}

void Table::printTable() const {
    printRows(selectRows());
}

void Table::printRows(const std::vector<size_t>& row_indices, 
                      const std::vector<std::string>& selected_columns) const {
    std::vector<size_t> col_indices;
    std::vector<std::string> headers;
    
    if (selected_columns.empty()) {
        // Select all columns
        for (size_t i = 0; i < columns.size(); ++i) {
            col_indices.push_back(i);
            headers.push_back(columns[i].name);
        }
    } else {
        // Select specified columns
        for (const auto& col_name : selected_columns) {
            if (hasColumn(col_name)) {
                col_indices.push_back(getColumnIndex(col_name));
                headers.push_back(col_name);
            }
        }
    }
    
    // Calculate column widths
    std::vector<size_t> widths;
    for (size_t i = 0; i < headers.size(); ++i) {
        size_t max_width = headers[i].length();
        
        for (size_t row_idx : row_indices) {
            if (row_idx < rows.size()) {
                std::string value_str = valueToString(rows[row_idx][col_indices[i]]);
                max_width = std::max(max_width, value_str.length());
            }
        }
        widths.push_back(std::max(max_width, size_t(8))); // Minimum width of 8
    }
    
    // Print header
    std::cout << "+";
    for (size_t width : widths) {
        std::cout << std::string(width + 2, '-') << "+";
    }
    std::cout << "\n|";
    
    for (size_t i = 0; i < headers.size(); ++i) {
        std::cout << " " << std::left << std::setw(widths[i]) << headers[i] << " |";
    }
    std::cout << "\n+";
    
    for (size_t width : widths) {
        std::cout << std::string(width + 2, '-') << "+";
    }
    std::cout << "\n";
    
    // Print rows
    for (size_t row_idx : row_indices) {
        if (row_idx < rows.size()) {
            std::cout << "|";
            for (size_t i = 0; i < col_indices.size(); ++i) {
                std::string value_str = valueToString(rows[row_idx][col_indices[i]]);
                std::cout << " " << std::left << std::setw(widths[i]) << value_str << " |";
            }
            std::cout << "\n";
        }
    }
    
    // Print footer
    std::cout << "+";
    for (size_t width : widths) {
        std::cout << std::string(width + 2, '-') << "+";
    }
    std::cout << "\n";
    
    std::cout << "(" << row_indices.size() << " rows)\n";
}
