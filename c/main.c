#include <stdio.h>
#include "rule_database.h"

int main() {
    RuleDatabase *db = init_db("rules.db");
    if (!db) {
        return -1;
    }

    // 示例规则1
    Rule rule1 = {
        .id = "1001",
        .enable = true,
        .name = "测试规则1",
        .mode = "自动",
        .trg_mtd = "边缘触发",
        .ops = "AND",
        .trg_cnds = ">",
        .trg_val = "50",
        .func_name = "温度报警",
        .out_net = "192.168.1.100",
        .out_reg_addr = "0x3000",
        .out_data_unit = "word",
        .out_data_bit = "16",
        .net = "192.168.1.1",
        .data_addr = "0x4000",
        .data_unit = "byte",
        .data_bit = "8",
        .grp_data = {
            {.index = "1", .lgcl_cnds = "AND", .net = "192.168.1.2", .data_addr = "0x5000", .data_unit = "word", .data_bit = "16"}
        },
        .grp_data_size = 1
    };

    // 插入规则1
    if (insert_rule(db, &rule1)) {
        printf("规则1插入成功\n");
    } else {
        printf("规则1插入失败\n");
    }

    // 查询规则1
    Rule fetched_rule;
    if (get_rule(db, "1001", &fetched_rule)) {
        printf("规则1查询成功：%s\n", fetched_rule.name);
    }

    // 删除规则1
    if (delete_rule(db, "1001")) {
        printf("规则1删除成功\n");
    }

    close_db(db);
    return 0;
}
