#include "rule_database.h"
#include <stdio.h>
#include <string.h>

// 初始化数据库连接
RuleDatabase *init_db(const char *db_path) {
    RuleDatabase *db = (RuleDatabase *)malloc(sizeof(RuleDatabase));
    db->db_path = db_path;
    if (sqlite3_open(db_path, &db->conn) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db->conn));
        free(db);
        return NULL;
    }
    create_tables(db);
    return db;
}

// 关闭数据库
void close_db(RuleDatabase *db) {
    if (db) {
        sqlite3_close(db->conn);
        free(db);
    }
}

// 创建数据库表
bool create_tables(RuleDatabase *db) {
    const char *create_rules_table = R"(
        CREATE TABLE IF NOT EXISTS rules (
            id TEXT PRIMARY KEY,
            enable TEXT,
            name TEXT,
            mode TEXT,
            trg_mtd TEXT,
            ops TEXT,
            trg_cnds TEXT,
            trg_val TEXT,
            func_name TEXT,
            out_net TEXT,
            out_reg_addr TEXT,
            out_data_unit TEXT,
            out_data_bit TEXT,
            net TEXT,
            data_addr TEXT,
            data_unit TEXT,
            data_bit TEXT
        );
    )";
    
    const char *create_group_data_table = R"(
        CREATE TABLE IF NOT EXISTS rule_group_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            rule_id TEXT,
            item_index TEXT,
            lgcl_cnds TEXT,
            net TEXT,
            data_addr TEXT,
            data_unit TEXT,
            data_bit TEXT,
            position INTEGER,
            FOREIGN KEY(rule_id) REFERENCES rules(id) ON DELETE CASCADE
        );
    )";
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db->conn, create_rules_table, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    rc = sqlite3_exec(db->conn, create_group_data_table, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

// 插入规则
bool insert_rule(RuleDatabase *db, Rule *rule) {
    sqlite3_stmt *stmt;
    const char *insert_rule_sql = R"(
        INSERT INTO rules (
            id, enable, name, mode, trg_mtd, ops, trg_cnds, trg_val,
            func_name, out_net, out_reg_addr, out_data_unit, out_data_bit,
            net, data_addr, data_unit, data_bit
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    
    int rc = sqlite3_prepare_v2(db->conn, insert_rule_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert statement\n");
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, rule->id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, rule->enable ? "True" : "False", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, rule->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, rule->mode, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, rule->trg_mtd, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, rule->ops, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, rule->trg_cnds, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, rule->trg_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, rule->func_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, rule->out_net, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, rule->out_reg_addr, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, rule->out_data_unit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, rule->out_data_bit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 14, rule->net, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 15, rule->data_addr, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 16, rule->data_unit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 17, rule->data_bit, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert rule\n");
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    
    // Insert group data
    const char *insert_group_data_sql = R"(
        INSERT INTO rule_group_data (rule_id, item_index, lgcl_cnds, net, data_addr, data_unit, data_bit, position)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";
    
    for (int i = 0; i < rule->grp_data_size; ++i) {
        sqlite3_prepare_v2(db->conn, insert_group_data_sql, -1, &stmt, 0);
        
        sqlite3_bind_text(stmt, 1, rule->id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, rule->grp_data[i].index, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, rule->grp_data[i].lgcl_cnds, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, rule->grp_data[i].net, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, rule->grp_data[i].data_addr, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, rule->grp_data[i].data_unit, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 7, rule->grp_data[i].data_bit, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 8, i);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to insert group data\n");
            sqlite3_finalize(stmt);
            return false;
        }
        
        sqlite3_finalize(stmt);
    }
    
    return true;
}

// 更新规则
bool update_rule(RuleDatabase *db, const char *rule_id, Rule *rule) {
    sqlite3_stmt *stmt;
    const char *update_rule_sql = R"(
        UPDATE rules SET 
            enable = ?, name = ?, mode = ?, trg_mtd = ?, ops = ?, trg_cnds = ?, trg_val = ?,
            func_name = ?, out_net = ?, out_reg_addr = ?, out_data_unit = ?, out_data_bit = ?,
            net = ?, data_addr = ?, data_unit = ?, data_bit = ?
        WHERE id = ?;
    )";
    
    int rc = sqlite3_prepare_v2(db->conn, update_rule_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare update statement\n");
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, rule->enable ? "True" : "False", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, rule->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, rule->mode, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, rule->trg_mtd, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, rule->ops, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, rule->trg_cnds, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, rule->trg_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, rule->func_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, rule->out_net, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, rule->out_reg_addr, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, rule->out_data_unit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, rule->out_data_bit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, rule->net, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 14, rule->data_addr, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 15, rule->data_unit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 16, rule->data_bit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 17, rule_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to update rule\n");
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    
    // Delete old group data
    const char *delete_group_data_sql = "DELETE FROM rule_group_data WHERE rule_id = ?;";
    sqlite3_prepare_v2(db->conn, delete_group_data_sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, rule_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete old group data\n");
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    
    // Insert new group data
    const char *insert_group_data_sql = R"(
        INSERT INTO rule_group_data (rule_id, item_index, lgcl_cnds, net, data_addr, data_unit, data_bit, position)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";
    
    for (int i = 0; i < rule->grp_data_size; ++i) {
        sqlite3_prepare_v2(db->conn, insert_group_data_sql, -1, &stmt, 0);
        
        sqlite3_bind_text(stmt, 1, rule->id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, rule->grp_data[i].index, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, rule->grp_data[i].lgcl_cnds, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, rule->grp_data[i].net, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, rule->grp_data[i].data_addr, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, rule->grp_data[i].data_unit, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 7, rule->grp_data[i].data_bit, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 8, i);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to insert group data\n");
            sqlite3_finalize(stmt);
            return false;
        }
        
        sqlite3_finalize(stmt);
    }
    
    return true;
}

// 获取规则
bool get_rule(RuleDatabase *db, const char *rule_id, Rule *rule) {
    sqlite3_stmt *stmt;
    const char *select_rule_sql = "SELECT * FROM rules WHERE id = ?;";
    
    int rc = sqlite3_prepare_v2(db->conn, select_rule_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select statement\n");
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, rule_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        fprintf(stderr, "Rule not found\n");
        sqlite3_finalize(stmt);
        return false;
    }
    
    // Populate rule struct
    strncpy(rule->id, (const char *)sqlite3_column_text(stmt, 0), sizeof(rule->id));
    rule->enable = strcmp((const char *)sqlite3_column_text(stmt, 1), "True") == 0;
    strncpy(rule->name, (const char *)sqlite3_column_text(stmt, 2), sizeof(rule->name));
    strncpy(rule->mode, (const char *)sqlite3_column_text(stmt, 3), sizeof(rule->mode));
    strncpy(rule->trg_mtd, (const char *)sqlite3_column_text(stmt, 4), sizeof(rule->trg_mtd));
    strncpy(rule->ops, (const char *)sqlite3_column_text(stmt, 5), sizeof(rule->ops));
    strncpy(rule->trg_cnds, (const char *)sqlite3_column_text(stmt, 6), sizeof(rule->trg_cnds));
    strncpy(rule->trg_val, (const char *)sqlite3_column_text(stmt, 7), sizeof(rule->trg_val));
    strncpy(rule->func_name, (const char *)sqlite3_column_text(stmt, 8), sizeof(rule->func_name));
    strncpy(rule->out_net, (const char *)sqlite3_column_text(stmt, 9), sizeof(rule->out_net));
    strncpy(rule->out_reg_addr, (const char *)sqlite3_column_text(stmt, 10), sizeof(rule->out_reg_addr));
    strncpy(rule->out_data_unit, (const char *)sqlite3_column_text(stmt, 11), sizeof(rule->out_data_unit));
    strncpy(rule->out_data_bit, (const char *)sqlite3_column_text(stmt, 12), sizeof(rule->out_data_bit));
    strncpy(rule->net, (const char *)sqlite3_column_text(stmt, 13), sizeof(rule->net));
    strncpy(rule->data_addr, (const char *)sqlite3_column_text(stmt, 14), sizeof(rule->data_addr));
    strncpy(rule->data_unit, (const char *)sqlite3_column_text(stmt, 15), sizeof(rule->data_unit));
    strncpy(rule->data_bit, (const char *)sqlite3_column_text(stmt, 16), sizeof(rule->data_bit));
    
    // Fetch group data
    const char *select_group_data_sql = "SELECT item_index, lgcl_cnds, net, data_addr, data_unit, data_bit FROM rule_group_data WHERE rule_id = ?;";
    sqlite3_prepare_v2(db->conn, select_group_data_sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, rule_id, -1, SQLITE_STATIC);
    
    int i = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        strncpy(rule->grp_data[i].index, (const char *)sqlite3_column_text(stmt, 0), sizeof(rule->grp_data[i].index));
        strncpy(rule->grp_data[i].lgcl_cnds, (const char *)sqlite3_column_text(stmt, 1), sizeof(rule->grp_data[i].lgcl_cnds));
        strncpy(rule->grp_data[i].net, (const char *)sqlite3_column_text(stmt, 2), sizeof(rule->grp_data[i].net));
        strncpy(rule->grp_data[i].data_addr, (const char *)sqlite3_column_text(stmt, 3), sizeof(rule->grp_data[i].data_addr));
        strncpy(rule->grp_data[i].data_unit, (const char *)sqlite3_column_text(stmt, 4), sizeof(rule->grp_data[i].data_unit));
        strncpy(rule->grp_data[i].data_bit, (const char *)sqlite3_column_text(stmt, 5), sizeof(rule->grp_data[i].data_bit));
        i++;
    }
    rule->grp_data_size = i;
    
    sqlite3_finalize(stmt);
    return true;
}

// 删除规则
bool delete_rule(RuleDatabase *db, const char *rule_id) {
    sqlite3_stmt *stmt;
    
    const char *delete_group_data_sql = "DELETE FROM rule_group_data WHERE rule_id = ?;";
    sqlite3_prepare_v2(db->conn, delete_group_data_sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, rule_id, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete group data\n");
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    
    const char *delete_rule_sql = "DELETE FROM rules WHERE id = ?;";
    sqlite3_prepare_v2(db->conn, delete_rule_sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, rule_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete rule\n");
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    return true;
}

// 获取所有规则
int get_all_rules(RuleDatabase *db, Rule *rules) {
    sqlite3_stmt *stmt;
    const char *select_all_rules_sql = "SELECT id FROM rules;";
    
    int rc = sqlite3_prepare_v2(db->conn, select_all_rules_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select statement\n");
        return 0;
    }
    
    int count = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char *rule_id = (const char *)sqlite3_column_text(stmt, 0);
        get_rule(db, rule_id, &rules[count]);
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}
