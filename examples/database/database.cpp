#include "database_engine.h"
#include "query_parser.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

class SimpleDatabase {
private:
    DatabaseEngine engine;
    bool running;
    
    void printWelcome() {
        std::cout << "========================================\n";
        std::cout << "      Welcome to SimpleDB v1.0         \n";
        std::cout << "    A lightweight C++ database         \n";
        std::cout << "========================================\n\n";
        std::cout << "Type 'help' for available commands\n";
        std::cout << "Type 'exit' or 'quit' to exit\n\n";
    }
    
    void printHelp() {
        std::cout << "\nAvailable Commands:\n";
        std::cout << "==================\n";
        std::cout << "CREATE TABLE <name> (<col1> <type1>, <col2> <type2>, ...)\n";
        std::cout << "  - Creates a new table with specified columns\n";
        std::cout << "  - Supported types: int, double, string, bool\n";
        std::cout << "  - Example: CREATE TABLE users (id int, name string, age int)\n\n";
        
        std::cout << "INSERT INTO <table> VALUES (<val1>, <val2>, ...)\n";
        std::cout << "  - Inserts a new row into the table\n";
        std::cout << "  - Example: INSERT INTO users VALUES (1, John, 25)\n\n";
        
        std::cout << "SELECT * FROM <table> [WHERE <condition>]\n";
        std::cout << "SELECT <col1>, <col2> FROM <table> [WHERE <condition>]\n";
        std::cout << "  - Selects data from a table\n";
        std::cout << "  - Example: SELECT * FROM users WHERE age > 20\n\n";
        
        std::cout << "DROP TABLE <table>\n";
        std::cout << "  - Removes a table and all its data\n\n";
        
        std::cout << "SHOW TABLES\n";
        std::cout << "  - Lists all tables in the database\n\n";
        
        std::cout << "DESCRIBE <table> or DESC <table>\n";
        std::cout << "  - Shows the structure of a table\n\n";
        
        std::cout << "Other Commands:\n";
        std::cout << "  help     - Show this help message\n";
        std::cout << "  info     - Show database information\n";
        std::cout << "  clear    - Clear the screen\n";
        std::cout << "  exit     - Exit the database\n\n";
    }
    
    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }
    
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
public:
    SimpleDatabase() : running(true) {}
    
    void run() {
        printWelcome();
        
        std::string input;
        while (running) {
            std::cout << "simpledb> ";
            std::getline(std::cin, input);
            
            input = trim(input);
            if (input.empty()) {
                continue;
            }
            
            // Convert to lowercase for command checking
            std::string lower_input = input;
            std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
            
            if (lower_input == "exit" || lower_input == "quit") {
                running = false;
                std::cout << "Goodbye!\n";
                break;
            } else if (lower_input == "help") {
                printHelp();
            } else if (lower_input == "info") {
                engine.printDatabaseInfo();
            } else if (lower_input == "clear") {
                clearScreen();
                printWelcome();
            } else {
                // Execute SQL-like query
                engine.executeQuery(input);
            }
            
            std::cout << std::endl;
        }
    }
    
    void loadSampleData() {
        std::cout << "Loading sample data...\n";
        
        try {
            // Create a users table
            engine.executeQuery("CREATE TABLE users (id int, name string, age int, active bool)");
            engine.executeQuery("INSERT INTO users VALUES (1, Alice, 25, true)");
            engine.executeQuery("INSERT INTO users VALUES (2, Bob, 30, true)");
            engine.executeQuery("INSERT INTO users VALUES (3, Charlie, 22, false)");
            engine.executeQuery("INSERT INTO users VALUES (4, Diana, 28, true)");
            
            // Create a products table
            engine.executeQuery("CREATE TABLE products (id int, name string, price double, in_stock bool)");
            engine.executeQuery("INSERT INTO products VALUES (1, Laptop, 999.99, true)");
            engine.executeQuery("INSERT INTO products VALUES (2, Mouse, 25.50, true)");
            engine.executeQuery("INSERT INTO products VALUES (3, Keyboard, 75.00, false)");
            engine.executeQuery("INSERT INTO products VALUES (4, Monitor, 299.99, true)");
            
            std::cout << "Sample data loaded successfully!\n";
            std::cout << "Try: SHOW TABLES\n";
            std::cout << "Try: SELECT * FROM users\n";
            std::cout << "Try: SELECT * FROM products WHERE price > 50\n\n";
            
        } catch (const std::exception& e) {
            std::cerr << "Error loading sample data: " << e.what() << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    SimpleDatabase db;
    
    // Check for command line arguments
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--sample" || arg == "-s") {
            db.loadSampleData();
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --sample, -s  Load sample data\n";
            std::cout << "  --help, -h    Show this help\n";
            return 0;
        }
    }
    
    db.run();
    return 0;
}