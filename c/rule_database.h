#ifndef RULE_DATABASE_H
#define RULE_DATABASE_H

#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_RULES 256
#define MAX_GRPS 256

// 规则结构体
typedef struct {
    char id[64];
    bool enable;
    char name[128];
    char mode[64];
    char trg_mtd[64];
    char ops[64];
    char trg_cnds[64];
    char trg_val[64];
    char func_name[128];
    char out_net[64];
    char out_reg_addr[64];
    char out_data_unit[64];
    char out_data_bit[64];
    char net[64];
    char data_addr[64];
    char data_unit[64];
    char data_bit[64];
    // group_data数组，存储相关的条件和信息
    struct GroupData *grp_data;
    int grp_data_size;
} Rule;

// group_data结构体
typedef struct {
    char index[64];
    char lgcl_cnds[64];
    char net[64];
    char data_addr[64];
    char data_unit[64];
    char data_bit[64];
} GroupData;

typedef struct {
    sqlite3 *conn;
    const char *db_path;
} RuleDatabase;

// 初始化数据库
RuleDatabase *init_db(const char *db_path);

// 关闭数据库
void close_db(RuleDatabase *db);

// 插入规则
bool insert_rule(RuleDatabase *db, Rule *rule);

// 更新规则
bool update_rule(RuleDatabase *db, const char *rule_id, Rule *rule);

// 获取规则
bool get_rule(RuleDatabase *db, const char *rule_id, Rule *rule);

// 删除规则
bool delete_rule(RuleDatabase *db, const char *rule_id);

// 获取所有规则
int get_all_rules(RuleDatabase *db, Rule *rules);

// 创建表
bool create_tables(RuleDatabase *db);

#endif // RULE_DATABASE_H
