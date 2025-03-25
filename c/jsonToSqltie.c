#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

#define DB_PATH "rules.db"

// 规则表结构定义
typedef struct {
    int id;
    char number[50];
    char enable[10];
    char name[100];
    char mode[50];
    char network1[50];
    char reg_addr1[50];
    char data_unit1[50];
    char data_bit1[50];
    char trigger_method[50];
    char operators[50];
    char trigger_conditions[50];
    char trigger_value[50];
    char func_name[50];
    char out_network[50];
    char out_reg_addr[50];
    char out_data_unit[50];
    char out_data_bit[50];
    char type[50];
} Rule;

// group_data子表结构定义
typedef struct {
    int id;
    int rule_id;
    char logical_conditions[50];
    char network[50];
    char reg_addr[50];
    char data_unit[50];
    char data_bit[50];
    int position;
} RuleGroupData;

typedef struct {
    sqlite3 *conn;
} RuleDatabase;

// 初始化数据库连接
int open_db(RuleDatabase *db) {
    int rc = sqlite3_open(DB_PATH, &db->conn);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db->conn));
        return rc;
    }
    return 0;
}

// 创建表结构
int create_tables(RuleDatabase *db) {
    const char *create_rules_table_sql = "CREATE TABLE IF NOT EXISTS rules ("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "number TEXT, "
                                        "enable TEXT, "
                                        "name TEXT, "
                                        "mode TEXT, "
                                        "network1 TEXT, "
                                        "reg_addr1 TEXT, "
                                        "data_unit1 TEXT, "
                                        "data_bit1 TEXT, "
                                        "trigger_method TEXT, "
                                        "operators TEXT, "
                                        "trigger_conditions TEXT, "
                                        "trigger_value TEXT, "
                                        "func_name TEXT, "
                                        "out_network TEXT, "
                                        "out_reg_addr TEXT, "
                                        "out_data_unit TEXT, "
                                        "out_data_bit TEXT, "
                                        "type TEXT);";
    
    const char *create_group_data_table_sql = "CREATE TABLE IF NOT EXISTS rule_group_data ("
                                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                              "rule_id INTEGER, "
                                              "logical_conditions TEXT, "
                                              "network TEXT, "
                                              "reg_addr TEXT, "
                                              "data_unit TEXT, "
                                              "data_bit TEXT, "
                                              "position INTEGER, "
                                              "FOREIGN KEY (rule_id) REFERENCES rules(id));";
    
    char *err_msg = 0;
    int rc = sqlite3_exec(db->conn, create_rules_table_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    rc = sqlite3_exec(db->conn, create_group_data_table_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return 0;
}

// 插入规则
int insert_rule(RuleDatabase *db, Rule *rule, RuleGroupData *group_data, int group_data_count) {
    sqlite3_stmt *stmt;
    const char *insert_rule_sql = "INSERT INTO rules (number, enable, name, mode, network1, reg_addr1, "
                                  "data_unit1, data_bit1, trigger_method, operators, trigger_conditions, "
                                  "trigger_value, func_name, out_network, out_reg_addr, out_data_unit, "
                                  "out_data_bit, type) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    // 插入规则数据
    int rc = sqlite3_prepare_v2(db->conn, insert_rule_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert rule: %s\n", sqlite3_errmsg(db->conn));
        return rc;
    }

    sqlite3_bind_text(stmt, 1, rule->number, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, rule->enable, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, rule->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, rule->mode, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, rule->network1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, rule->reg_addr1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, rule->data_unit1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, rule->data_bit1, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, rule->trigger_method, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, rule->operators, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, rule->trigger_conditions, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, rule->trigger_value, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, rule->func_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 14, rule->out_network, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 15, rule->out_reg_addr, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 16, rule->out_data_unit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 17, rule->out_data_bit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 18, rule->type, -1, SQLITE_STATIC);  // Added missing bind for the 'type' field

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert rule: %s\n", sqlite3_errmsg(db->conn));
        sqlite3_finalize(stmt);
        return rc;
    }

    int rule_id = sqlite3_last_insert_rowid(db->conn);
    sqlite3_finalize(stmt);

    // 插入group_data数据
    const char *insert_group_data_sql = "INSERT INTO rule_group_data (rule_id, logical_conditions, network, "
                                        "reg_addr, data_unit, data_bit, position) VALUES (?, ?, ?, ?, ?, ?, ?)";
    for (int i = 0; i < group_data_count; i++) {
        rc = sqlite3_prepare_v2(db->conn, insert_group_data_sql, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare insert group data: %s\n", sqlite3_errmsg(db->conn));
            return rc;
        }
        
        sqlite3_bind_int(stmt, 1, rule_id);
        sqlite3_bind_text(stmt, 2, group_data[i].logical_conditions, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, group_data[i].network, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, group_data[i].reg_addr, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, group_data[i].data_unit, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, group_data[i].data_bit, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 7, group_data[i].position);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed to insert group data: %s\n", sqlite3_errmsg(db->conn));
            sqlite3_finalize(stmt);
            return rc;
        }
    }
    
    sqlite3_finalize(stmt);
    return rule_id;
}

// 查询规则
int get_rule(RuleDatabase *db, int rule_id, Rule *rule, RuleGroupData **group_data, int *group_data_count) {
    sqlite3_stmt *stmt;
    const char *select_rule_sql = "SELECT * FROM rules WHERE id = ?";
    
    int rc = sqlite3_prepare_v2(db->conn, select_rule_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select rule: %s\n", sqlite3_errmsg(db->conn));
        return rc;
    }

    sqlite3_bind_int(stmt, 1, rule_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        fprintf(stderr, "Failed to get rule data: %s\n", sqlite3_errmsg(db->conn));
        sqlite3_finalize(stmt);
        return rc;
    }
    
    // 填充rule结构体
    rule->id = sqlite3_column_int(stmt, 0);
    strcpy(rule->number, (const char *)sqlite3_column_text(stmt, 1));
    strcpy(rule->enable, (const char *)sqlite3_column_text(stmt, 2));
    strcpy(rule->name, (const char *)sqlite3_column_text(stmt, 3));
    strcpy(rule->mode, (const char *)sqlite3_column_text(stmt, 4));
    strcpy(rule->network1, (const char *)sqlite3_column_text(stmt, 5));
    strcpy(rule->reg_addr1, (const char *)sqlite3_column_text(stmt, 6));
    strcpy(rule->data_unit1, (const char *)sqlite3_column_text(stmt, 7));
    strcpy(rule->data_bit1, (const char *)sqlite3_column_text(stmt, 8));
    strcpy(rule->trigger_method, (const char *)sqlite3_column_text(stmt, 9));
    strcpy(rule->operators, (const char *)sqlite3_column_text(stmt, 10));
    strcpy(rule->trigger_conditions, (const char *)sqlite3_column_text(stmt, 11));
    strcpy(rule->trigger_value, (const char *)sqlite3_column_text(stmt, 12));
    strcpy(rule->func_name, (const char *)sqlite3_column_text(stmt, 13));
    strcpy(rule->out_network, (const char *)sqlite3_column_text(stmt, 14));
    strcpy(rule->out_reg_addr, (const char *)sqlite3_column_text(stmt, 15));
    strcpy(rule->out_data_unit, (const char *)sqlite3_column_text(stmt, 16));
    strcpy(rule->out_data_bit, (const char *)sqlite3_column_text(stmt, 17));
    strcpy(rule->type, (const char *)sqlite3_column_text(stmt, 18));
    
    sqlite3_finalize(stmt);
    
    // 查询group_data数据
    const char *select_group_data_sql = "SELECT logical_conditions, network, reg_addr, data_unit, data_bit "
                                        "FROM rule_group_data WHERE rule_id = ? ORDER BY position";
    
    rc = sqlite3_prepare_v2(db->conn, select_group_data_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare select group data: %s\n", sqlite3_errmsg(db->conn));
        return rc;
    }

    sqlite3_bind_int(stmt, 1, rule_id);
    *group_data_count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*group_data_count)++;
    }
    
    *group_data = malloc(sizeof(RuleGroupData) * (*group_data_count));
    
    sqlite3_reset(stmt);
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        strcpy((*group_data)[i].logical_conditions, (const char *)sqlite3_column_text(stmt, 0));
        strcpy((*group_data)[i].network, (const char *)sqlite3_column_text(stmt, 1));
        strcpy((*group_data)[i].reg_addr, (const char *)sqlite3_column_text(stmt, 2));
        strcpy((*group_data)[i].data_unit, (const char *)sqlite3_column_text(stmt, 3));
        strcpy((*group_data)[i].data_bit, (const char *)sqlite3_column_text(stmt, 4));
        i++;
    }
    
    sqlite3_finalize(stmt);
    
    return 0;
}

// 关闭数据库连接
void close_db(RuleDatabase *db) {
    sqlite3_close(db->conn);
}

int main() {
    RuleDatabase db;
    int rc = open_db(&db);
    if (rc != 0) {
        return rc;
    }

    rc = create_tables(&db);
    if (rc != 0) {
        close_db(&db);
        return rc;
    }

    // 测试插入和查询规则
    Rule rule = {
        .number = "001",
        .enable = "True",
        .name = "测试规则",
        .mode = "自动",
        .network1 = "192.168.1.1",
        .reg_addr1 = "0x1000",
        .data_unit1 = "byte",
        .data_bit1 = "8",
        .trigger_method = "边缘触发",
        .operators = "AND",
        .trigger_conditions = ">",
        .trigger_value = "10",
        .func_name = "报警功能",
        .out_network = "192.168.2.1",
        .out_reg_addr = "0x2000",
        .out_data_unit = "word",
        .out_data_bit = "16",
        .type = "add"
    };
    
    RuleGroupData group_data[] = {
        { .logical_conditions = "AND", .network = "192.168.1.2", .reg_addr = "0x1100", .data_unit = "byte", .data_bit = "8", .position = 0 },
        { .logical_conditions = "OR", .network = "192.168.1.3", .reg_addr = "0x1200", .data_unit = "word", .data_bit = "16", .position = 1 }
    };

    int group_data_count = 2;
    
    int rule_id = insert_rule(&db, &rule, group_data, group_data_count);
    printf("Inserted rule with ID: %d\n", rule_id);
    
    Rule fetched_rule;
    RuleGroupData *fetched_group_data = NULL;
    int fetched_group_data_count;
    
    rc = get_rule(&db, rule_id, &fetched_rule, &fetched_group_data, &fetched_group_data_count);
    if (rc == 0) {
        printf("Fetched rule ID: %d\n", fetched_rule.id);
        printf("Group data count: %d\n", fetched_group_data_count);
    }

    free(fetched_group_data);
    close_db(&db);
    return 0;
}
