#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class RuleDatabase {
public:
    explicit RuleDatabase(const std::string& db_path = "rules.db")
        : db_path_(db_path), conn_(nullptr) {
        InitializeDatabase();
    }

    ~RuleDatabase() {
        Close();
    }

    bool InsertRule(const std::unordered_map<std::string, std::string>& rule_data) {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_stmt* stmt_group_data = nullptr;
        try {
            // 1. Insert into main table
            const std::string insert_query = R"(
                INSERT INTO rules VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
            )";
            sqlite3_prepare_v2(conn_, insert_query.c_str(), -1, &stmt, nullptr);
            BindRuleData(stmt, rule_data);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::runtime_error("Error inserting rule into main table.");
            }

            // 2. Insert group data if exists
            if (rule_data.find("grp_data") != rule_data.end()) {
                for (size_t index = 0; index < rule_data.size(); ++index) {
                    const std::string group_data_query = R"(
                        INSERT INTO rule_group_data VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?);
                    )";
                    sqlite3_prepare_v2(conn_, group_data_query.c_str(), -1, &stmt_group_data, nullptr);
                    BindGroupData(stmt_group_data, rule_data, index);

                    if (sqlite3_step(stmt_group_data) != SQLITE_DONE) {
                        throw std::runtime_error("Error inserting group data.");
                    }
                }
            }

            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_group_data);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "InsertRule error: " << e.what() << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_group_data);
            return false;
        }
    }

    bool UpdateRule(const std::string& rule_id, const std::unordered_map<std::string, std::string>& rule_data) {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_stmt* stmt_group_data = nullptr;
        try {
            // 1. Delete old group data
            const std::string delete_group_data_query = R"(
                DELETE FROM rule_group_data WHERE rule_id = ?;
            )";
            sqlite3_prepare_v2(conn_, delete_group_data_query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, rule_id.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::runtime_error("Error deleting old group data.");
            }

            // 2. Update main table data
            const std::string update_query = R"(
                UPDATE rules SET enable = ?, name = ?, mode = ?, trg_mtd = ?, ops = ?, trg_cnds = ?, 
                trg_val = ?, func_name = ?, out_net = ?, out_reg_addr = ?, out_data_unit = ?, out_data_bit = ?, 
                net = ?, data_addr = ?, data_unit = ?, data_bit = ? WHERE id = ?;
            )";
            sqlite3_prepare_v2(conn_, update_query.c_str(), -1, &stmt, nullptr);
            BindRuleData(stmt, rule_data);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::runtime_error("Error updating rule in main table.");
            }

            // 3. Insert new group data if exists
            if (rule_data.find("grp_data") != rule_data.end()) {
                for (size_t index = 0; index < rule_data.size(); ++index) {
                    const std::string group_data_query = R"(
                        INSERT INTO rule_group_data VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?);
                    )";
                    sqlite3_prepare_v2(conn_, group_data_query.c_str(), -1, &stmt_group_data, nullptr);
                    BindGroupData(stmt_group_data, rule_data, index);

                    if (sqlite3_step(stmt_group_data) != SQLITE_DONE) {
                        throw std::runtime_error("Error inserting new group data.");
                    }
                }
            }

            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_group_data);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "UpdateRule error: " << e.what() << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_group_data);
            return false;
        }
    }

    bool DeleteRule(const std::string& rule_id) {
        try {
            // Delete rule from main table
            const std::string delete_query = R"(
                DELETE FROM rules WHERE id = ?;
            )";
            sqlite3_stmt* stmt = nullptr;
            sqlite3_prepare_v2(conn_, delete_query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, rule_id.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::runtime_error("Error deleting rule.");
            }

            sqlite3_finalize(stmt);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "DeleteRule error: " << e.what() << std::endl;
            return false;
        }
    }

    bool GetRule(const std::string& rule_id, std::unordered_map<std::string, std::string>& rule_data) {
        try {
            // Fetch rule data from main table
            const std::string select_query = R"(
                SELECT * FROM rules WHERE id = ?;
            )";
            sqlite3_stmt* stmt = nullptr;
            sqlite3_prepare_v2(conn_, select_query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, rule_id.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                // Populate rule_data from result set
                rule_data = GetRowData(stmt);
            }

            sqlite3_finalize(stmt);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "GetRule error: " << e.what() << std::endl;
            return false;
        }
    }

private:
    void InitializeDatabase() {
        if (sqlite3_open(db_path_.c_str(), &conn_) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database.");
        }
        CreateTables();
    }

    void CreateTables() {
        const std::string create_query = R"(
            CREATE TABLE IF NOT EXISTS rules (
                id TEXT PRIMARY KEY, enable TEXT, name TEXT, mode TEXT, trg_mtd TEXT,
                ops TEXT, trg_cnds TEXT, trg_val TEXT, func_name TEXT, out_net TEXT,
                out_reg_addr TEXT, out_data_unit TEXT, out_data_bit TEXT, net TEXT,
                data_addr TEXT, data_unit TEXT, data_bit TEXT
            );
            CREATE TABLE IF NOT EXISTS rule_group_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT, rule_id TEXT, item_index TEXT,
                lgcl_cnds TEXT, net TEXT, data_addr TEXT, data_unit TEXT, data_bit TEXT,
                position INTEGER, FOREIGN KEY(rule_id) REFERENCES rules(id) ON DELETE CASCADE
            );
        )";

        char* err_msg = nullptr;
        if (sqlite3_exec(conn_, create_query.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::string err(err_msg);
            sqlite3_free(err_msg);
            throw std::runtime_error("Error creating tables: " + err);
        }
    }

    void BindRuleData(sqlite3_stmt* stmt, const std::unordered_map<std::string, std::string>& rule_data) {
        // Bind parameters to the query statement from rule_data
        // This function will use the rule_data to bind each value to the prepared statement
    }

    void BindGroupData(sqlite3_stmt* stmt, const std::unordered_map<std::string, std::string>& rule_data, size_t index) {
        // Bind group data to the statement
    }

    std::unordered_map<std::string, std::string> GetRowData(sqlite3_stmt* stmt) {
        // Extract data from the sqlite3_stmt and return as a map
        std::unordered_map<std::string, std::string> row_data;
        return row_data;
    }

    void Close() {
        if (conn_) {
            sqlite3_close(conn_);
        }
    }

private:
    std::string db_path_;
    sqlite3* conn_;
};
