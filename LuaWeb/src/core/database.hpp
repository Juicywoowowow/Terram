#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <memory>

namespace luaweb {

/**
 * Database value - can be null, int, double, or string
 */
using DbValue = std::variant<std::nullptr_t, int64_t, double, std::string>;

/**
 * Database row - column name to value mapping
 */
using DbRow = std::unordered_map<std::string, DbValue>;

/**
 * Database result - multiple rows
 */
using DbResult = std::vector<DbRow>;

/**
 * Exec result - changes and last insert ID
 */
struct ExecResult {
    int64_t changes = 0;
    int64_t last_insert_id = 0;
};

/**
 * Database Bridge Instance
 * 
 * Manages a Node.js database bridge process for a specific server.
 * Each LuaWeb server gets its own bridge instance.
 */
class DatabaseBridge {
public:
    /**
     * Create a bridge for a specific server
     * @param server_id Unique server identifier
     */
    explicit DatabaseBridge(const std::string& server_id);
    ~DatabaseBridge();
    
    // Disable copy
    DatabaseBridge(const DatabaseBridge&) = delete;
    DatabaseBridge& operator=(const DatabaseBridge&) = delete;
    
    /**
     * Start the database bridge if not already running
     */
    bool start();
    
    /**
     * Stop the database bridge
     */
    void stop();
    
    /**
     * Check if bridge is running
     */
    bool is_running() const { return running_; }
    
    /**
     * Get the port this bridge is running on
     */
    int port() const { return port_; }
    
    /**
     * Get the server ID
     */
    const std::string& server_id() const { return server_id_; }
    
    /**
     * Make HTTP request to the bridge
     */
    std::string request(const std::string& endpoint, const std::string& json_body);

private:
    std::string server_id_;
    int port_ = 0;
    pid_t bridge_pid_ = 0;
    bool running_ = false;
    
    bool check_health();
    int find_free_port();
};

/**
 * Database Connection
 * 
 * Represents a connection to a SQLite database.
 */
class Database {
public:
    /**
     * Open a database using a specific bridge
     * @param bridge The database bridge to use
     * @param path Database path (relative to server's __DBWEB__ dir or absolute)
     */
    Database(DatabaseBridge& bridge, const std::string& path = "");
    ~Database();
    
    // Disable copy
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    // Enable move
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;
    
    /**
     * Execute SQL (INSERT, UPDATE, DELETE, CREATE, etc.)
     * @return ExecResult with changes and last insert ID
     */
    ExecResult exec(const std::string& sql, 
                    const std::vector<DbValue>& params = {});
    
    /**
     * Query SQL (SELECT)
     * @return Vector of rows
     */
    DbResult query(const std::string& sql,
                   const std::vector<DbValue>& params = {});
    
    /**
     * Close the database connection
     */
    void close();
    
    /**
     * Check if database is open
     */
    bool is_open() const { return !db_id_.empty(); }
    
    /**
     * Get the database path
     */
    const std::string& path() const { return path_; }
    
    /**
     * Get last error message
     */
    const std::string& last_error() const { return last_error_; }

private:
    DatabaseBridge* bridge_ = nullptr;
    std::string db_id_;
    std::string path_;
    std::string last_error_;
};

} // namespace luaweb
