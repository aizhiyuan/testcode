#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

// 定义数据库连接
sqlite3 *db;

// 处理查询结果的回调函数
int callback(void *data, int argc, char **argv, char **col_names) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", col_names[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

// 打开数据库
int open_db(const char *db_path) {
    int rc = sqlite3_open(db_path, &db);
    if (rc) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    return SQLITE_OK;
}

// 关闭数据库
void close_db() {
    sqlite3_close(db);
}

// 查询规则
int get_rule(const char *rule_id) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT * FROM rules WHERE id = ?;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL错误: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    sqlite3_bind_text(stmt, 1, rule_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // 打印查询结果
        for (int i = 0; i < sqlite3_column_count(stmt); i++) {
            printf("%s = %s\n", sqlite3_column_name(stmt, i), sqlite3_column_text(stmt, i));
        }
    } else if (rc == SQLITE_DONE) {
        printf("没有找到该规则: %s\n", rule_id);
    } else {
        fprintf(stderr, "查询失败: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return rc;
}

// 查询所有规则
int get_all_rules() {
    const char *sql = "SELECT id FROM rules;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL错误: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char *rule_id = (const char *)sqlite3_column_text(stmt, 0);
        printf("规则ID: %s\n", rule_id);
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "查询失败: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return rc;
}

// 删除规则
int delete_rule(const char *rule_id) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM rules WHERE id = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL错误: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    sqlite3_bind_text(stmt, 1, rule_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "删除失败: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return rc;
    }

    printf("删除规则成功: %s\n", rule_id);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

int main() {
    const char *db_path = "rules.db";
    
    // 打开数据库
    if (open_db(db_path) != SQLITE_OK) {
        return 1;
    }

    // 查询单个规则
    const char *rule_id = "1001";
    printf("查询规则: %s\n", rule_id);
    get_rule(rule_id);

    // 查询所有规则
    printf("\n查询所有规则:\n");
    get_all_rules();

    // 删除规则
    printf("\n删除规则: %s\n", rule_id);
    delete_rule(rule_id);

    // 关闭数据库连接
    close_db();
    
    return 0;
}
