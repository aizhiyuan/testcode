#include <stdio.h>
#include <string.h>
#include "rule_database.h"

int main()
{
    RuleDatabase *db = init_db("rules.db");
    if (!db)
    {
        return -1;
    }

    // 示例规则1
    Rule rule1 = {
        .id = "1001",
        .enable = "on",
        .name = "测试规则1",
        .mode = "自动",
        .trg_mtd = "边缘触发",
        .ops = "AND",
        .trg_cnds = ">",
        .trg_val = "50",
        .func_name = "温度报警",
        .out_net = "192.168.1.100",
        .out_data_addr = "0x3000",
        .out_data_unit = "word",
        .out_data_bit = "16",
        .net = "192.168.1.1",
        .data_addr = "0x4000",
        .data_unit = "byte",
        .data_bit = "8",
        .grp_data_size = 2,
        .grp_data = {
            {"1", "AND", "192.168.1.2", "0x5000", "word", "16"},
            {"2", "OR", "192.168.1.3", "0x5001", "byte", "8"}}};

    Rule rule2 = {
        .id = "1002",
        .enable = "on",
        .name = "测试规则2",
        .mode = "自动",
        .trg_mtd = "边缘触发",
        .ops = "AND",
        .trg_cnds = ">",
        .trg_val = "50",
        .func_name = "温度报警",
        .out_net = "192.168.1.100",
        .out_data_addr = "0x3000",
        .out_data_unit = "word",
        .out_data_bit = "16",
        .net = "192.168.1.1",
        .data_addr = "0x4000",
        .data_unit = "byte",
        .data_bit = "8",
        .grp_data_size = 7,
        .grp_data = {
            {"1", "AND", "192.168.1.2", "0x5000", "word", "16"},
            {"2", "OR", "192.168.1.3", "0x5001", "byte", "8"},
            {"3", "OR", "192.168.1.3", "0x5001", "byte", "8"},
            {"4", "OR", "192.168.1.3", "0x5001", "byte", "8"},
            {"5", "OR", "192.168.1.3", "0x5001", "byte", "8"},
            {"6", "OR", "192.168.1.3", "0x5001", "byte", "8"},
            {"7", "OR", "192.168.1.3", "0x5001", "byte", "8"}}};

    // 插入规则1
    if (insert_rule(db, &rule1))
    {
        printf("规则1插入成功\n");
    }
    else
    {
        printf("规则1插入失败\n");
    }

    // 插入规则2
    if (insert_rule(db, &rule2))
    {
        printf("规则2插入成功\n");
    }
    else
    {
        printf("规则2插入失败\n");
    }

    // 查询规则1
    Rule fetched_rule;
    if (get_rule(db, "1001", &fetched_rule))
    {
        printf("规则1查询成功:\n");
        print_rule_json(&fetched_rule);
    }

    // 查询规则2
    memset(&fetched_rule, 0, sizeof(fetched_rule));
    if (get_rule(db, "1002", &fetched_rule))
    {
        printf("规则2查询成功:\n");
        print_rule_json(&fetched_rule);
    }

    Rule updataRule1 = {
        .id = "1001",
        .enable = "on",
        .name = "测试规则1",
        .mode = "自动",
        .trg_mtd = "边缘触发",
        .ops = "AND",
        .trg_cnds = ">",
        .trg_val = "50",
        .func_name = "温度报警",
        .out_net = "192.168.1.100",
        .out_data_addr = "0x3000",
        .out_data_unit = "word",
        .out_data_bit = "16",
        .net = "192.168.1.1",
        .data_addr = "0x4000",
        .data_unit = "byte",
        .data_bit = "8",
        .grp_data_size = 0,
        .grp_data = {}
    };

    // 更新规则1
    if (update_rule(db, "1001", &updataRule1))
    {
        printf("规则1更新成功:\n");
    }
    else
    {
        printf("规则1更新失败:\n");
    }

    memset(&fetched_rule, 0, sizeof(fetched_rule));
    if (get_rule(db, "1001", &fetched_rule))
    {
        printf("规则1查询成功:\n");
        print_rule_json(&fetched_rule);
    }

    // // 删除规则1
    // if (delete_rule(db, "1001"))
    // {
    //     printf("规则1删除成功\n");
    // }

    // // 删除规则2
    // if (delete_rule(db, "1002"))
    // {
    //     printf("规则2删除成功\n");
    // }

    close_db(db);
    return 0;
}
