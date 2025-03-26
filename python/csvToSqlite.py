import csv
import sqlite3
import json
from typing import Dict, List, Optional

class RuleDatabase:
    def __init__(self, db_path: str = 'rules.db'):
        """初始化数据库连接并创建表结构"""
        self.conn = sqlite3.connect(db_path)
        self.create_tables()
    
    def create_tables(self):
        """创建数据库表结构"""
        cursor = self.conn.cursor()
        
        # 创建主表
        cursor.execute('''
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
        )
        ''')
        
        # 创建group_data子表
        cursor.execute('''
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
            FOREIGN KEY (rule_id) REFERENCES rules(id) ON DELETE CASCADE
        )
        ''')
        
        self.conn.commit()
    
    def insert_rule(self, rule_data: Dict) -> bool:
        """添加新规则"""
        cursor = self.conn.cursor()
        try:
            # 插入主表数据
            cursor.execute('''
            INSERT INTO rules VALUES (
                ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
            )
            ''', (
                rule_data.get('id'),
                str(rule_data.get('enable')),
                rule_data.get('name'),
                rule_data.get('mode'),
                rule_data.get('trg_mtd'),
                rule_data.get('ops'),
                rule_data.get('trg_cnds'),
                rule_data.get('trg_val'),
                rule_data.get('func_name'),
                rule_data.get('out_net'),
                rule_data.get('out_reg_addr'),
                rule_data.get('out_data_unit'),
                rule_data.get('out_data_bit'),
                rule_data.get('net'),
                rule_data.get('data_addr'),
                rule_data.get('data_unit'),
                rule_data.get('data_bit')
            ))
            
            # 插入group_data数据
            if 'grp_data' in rule_data:
                for index, item in enumerate(rule_data['grp_data']):
                    cursor.execute('''
                    INSERT INTO rule_group_data VALUES (
                        NULL, ?, ?, ?, ?, ?, ?, ?, ?
                    )
                    ''', (
                        rule_data['id'],
                        item.get('index'),
                        item.get('lgcl_cnds'),
                        item.get('net'),
                        item.get('data_addr'),
                        item.get('data_unit'),
                        item.get('data_bit'),
                        index
                    ))
            
            self.conn.commit()
            return True
        except sqlite3.Error as e:
            print(f"添加规则失败: {e}")
            self.conn.rollback()
            return False
    
    def update_rule(self, rule_id: str, rule_data: Dict) -> bool:
        """更新现有规则"""
        cursor = self.conn.cursor()
        try:
            # 1. 先删除旧的group_data
            cursor.execute('DELETE FROM rule_group_data WHERE rule_id = ?', (rule_id,))
            
            # 2. 更新主表数据
            cursor.execute('''
            UPDATE rules SET
                enable = ?,
                name = ?,
                mode = ?,
                trg_mtd = ?,
                ops = ?,
                trg_cnds = ?,
                trg_val = ?,
                func_name = ?,
                out_net = ?,
                out_reg_addr = ?,
                out_data_unit = ?,
                out_data_bit = ?,
                net = ?,
                data_addr = ?,
                data_unit = ?,
                data_bit = ?
            WHERE id = ?
            ''', (
                str(rule_data.get('enable')),
                rule_data.get('name'),
                rule_data.get('mode'),
                rule_data.get('trg_mtd'),
                rule_data.get('ops'),
                rule_data.get('trg_cnds'),
                rule_data.get('trg_val'),
                rule_data.get('func_name'),
                rule_data.get('out_net'),
                rule_data.get('out_reg_addr'),
                rule_data.get('out_data_unit'),
                rule_data.get('out_data_bit'),
                rule_data.get('net'),
                rule_data.get('data_addr'),
                rule_data.get('data_unit'),
                rule_data.get('data_bit'),
                rule_id
            ))
            
            # 3. 插入新的group_data
            if 'grp_data' in rule_data:
                for index, item in enumerate(rule_data['grp_data']):
                    cursor.execute('''
                    INSERT INTO rule_group_data VALUES (
                        NULL, ?, ?, ?, ?, ?, ?, ?, ?
                    )
                    ''', (
                        rule_id,
                        item.get('index'),
                        item.get('lgcl_cnds'),
                        item.get('net'),
                        item.get('data_addr'),
                        item.get('data_unit'),
                        item.get('data_bit'),
                        index
                    ))
            
            self.conn.commit()
            return True
        except sqlite3.Error as e:
            print(f"更新规则失败: {e}")
            self.conn.rollback()
            return False
    
    def get_rule(self, rule_id: str) -> Optional[Dict]:
        """获取单个规则"""
        cursor = self.conn.cursor()
        
        # 获取主表数据
        cursor.execute('SELECT * FROM rules WHERE id = ?', (rule_id,))
        rule_row = cursor.fetchone()
        if not rule_row:
            return None
        
        # 获取group_data数据
        cursor.execute('''
        SELECT item_index, lgcl_cnds, net, data_addr, data_unit, data_bit
        FROM rule_group_data 
        WHERE rule_id = ?
        ORDER BY position
        ''', (rule_id,))
        
        grp_data = [{
            'index': row[0],
            'lgcl_cnds': row[1],
            'net': row[2],
            'data_addr': row[3],
            'data_unit': row[4],
            'data_bit': row[5]
        } for row in cursor.fetchall()]
        
        return {
            'id': rule_row[0],
            'enable': rule_row[1] == 'True',
            'name': rule_row[2],
            'mode': rule_row[3],
            'trg_mtd': rule_row[4],
            'ops': rule_row[5],
            'trg_cnds': rule_row[6],
            'trg_val': rule_row[7],
            'func_name': rule_row[8],
            'out_net': rule_row[9],
            'out_reg_addr': rule_row[10],
            'out_data_unit': rule_row[11],
            'out_data_bit': rule_row[12],
            'net': rule_row[13],
            'data_addr': rule_row[14],
            'data_unit': rule_row[15],
            'data_bit': rule_row[16],
            'grp_data': grp_data
        }
    
    def get_all_rules(self) -> List[Dict]:
        """获取所有规则"""
        cursor = self.conn.cursor()
        cursor.execute('SELECT id FROM rules')
        return [self.get_rule(row[0]) for row in cursor.fetchall()]
    
    def delete_rule(self, rule_id: str) -> bool:
        """删除规则（级联删除group_data）"""
        try:
            self.conn.execute('DELETE FROM rules WHERE id = ?', (rule_id,))
            self.conn.commit()
            return True
        except sqlite3.Error as e:
            print(f"删除规则失败: {e}")
            self.conn.rollback()
            return False
    
    def export_to_csv(self, csv_path: str = 'rules_export.csv'):
        """
        导出所有规则到单个CSV文件
        格式：每条主规则占一行，group_data数据合并为JSON字符串
        """
        rules = self.get_all_rules()
        if not rules:
            print("没有规则可导出")
            return
        
        # 准备CSV字段
        fieldnames = [
            'id', 'enable', 'name', 'mode', 
            'trg_mtd', 'ops', 'trg_cnds', 'trg_val',
            'func_name', 'out_net', 'out_reg_addr', 
            'out_data_unit', 'out_data_bit',
            'net', 'data_addr', 'data_unit', 'data_bit',
            'grp_data'  # group_data将以JSON字符串存储
        ]
        
        with open(csv_path, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            
            for rule in rules:
                # 复制规则数据并转换group_data为JSON字符串
                row_data = rule.copy()
                row_data['grp_data'] = json.dumps(rule['grp_data'], ensure_ascii=False)
                row_data['enable'] = str(row_data['enable'])  # 确保布尔值转为字符串
                writer.writerow(row_data)
        
        print(f"成功导出 {len(rules)} 条规则到 {csv_path}")
    
    def import_from_csv(self, csv_path: str = 'rules_export.csv'):
        """
        从CSV文件导入规则
        格式：每条主规则一行，group_data为JSON字符串
        """
        try:
            with open(csv_path, 'r', newline='', encoding='utf-8') as csvfile:
                reader = csv.DictReader(csvfile)
                for row in reader:
                    # 转换JSON字符串回Python对象
                    if 'grp_data' in row and row['grp_data']:
                        row['grp_data'] = json.loads(row['grp_data'])
                    else:
                        row['grp_data'] = []
                    
                    # 转换enable为布尔值
                    row['enable'] = row['enable'].lower() == 'true'
                    
                    # 检查规则是否已存在
                    existing_rule = self.get_rule(row['id'])
                    if existing_rule:
                        self.update_rule(row['id'], row)
                    else:
                        self.insert_rule(row)
            
            print(f"成功从 {csv_path} 导入规则")
            return True
        except Exception as e:
            print(f"导入规则失败: {e}")
            return False
    
    def close(self):
        """关闭数据库连接"""
        self.conn.close()

# 使用示例
if __name__ == '__main__':
    db = RuleDatabase()
    
    # 测试数据
    test_rule1 = {
        "id": "1001",
        "enable": True,
        "name": "测试规则1",
        "mode": "自动",
        "trg_mtd": "边缘触发",
        "ops": "AND",
        "trg_cnds": ">",
        "trg_val": "50",
        "func_name": "温度报警",
        "out_net": "192.168.1.100",
        "out_reg_addr": "0x3000",
        "out_data_unit": "word",
        "out_data_bit": "16",
        "net": "192.168.1.1",
        "data_addr": "0x4000",
        "data_unit": "byte",
        "data_bit": "8",
        "grp_data": [
            {
                "index": "1",
                "lgcl_cnds": "AND",
                "net": "192.168.1.2",
                "data_addr": "0x5000",
                "data_unit": "word",
                "data_bit": "16"
            }
        ]
    }
    
    test_rule2 = {
        "id": "1002",
        "enable": False,
        "name": "测试规则2",
        "mode": "手动",
        "trg_mtd": "电平触发",
        "ops": "OR",
        "trg_cnds": "<",
        "trg_val": "20",
        "func_name": "控制功能",
        "out_net": "192.168.1.101",
        "out_reg_addr": "0x3001",
        "out_data_unit": "byte",
        "out_data_bit": "8",
        "net": "192.168.1.10",
        "data_addr": "0x4001",
        "data_unit": "word",
        "data_bit": "16",
        "grp_data": [
            {
                "index": "1",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },
            {
                "index": "2",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },
            {
                "index": "3",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },
            {
                "index": "4",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },
            {
                "index": "5",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },{
                "index": "6",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            }
            ,{
                "index": "7",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },{
                "index": "8",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },{
                "index": "9",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            },{
                "index": "10",
                "lgcl_cnds": "OR",
                "net": "192.168.1.11",
                "data_addr": "0x5001",
                "data_unit": "word",
                "data_bit": "16"
            }
        ]
    }

    # 测试增删改查
    print("1. 添加规则:", db.insert_rule(test_rule1))
    print("2. 添加规则:", db.insert_rule(test_rule2))
    
    # 导出到CSV
    print("\n导出测试:")
    db.export_to_csv('rules_export.csv')
    
    # 清空数据库
    print("\n清空数据库测试:")
    for rule in db.get_all_rules():
        db.delete_rule(rule['id'])
    print("当前规则数量:", len(db.get_all_rules()))
    
    # 从CSV导入
    print("\n导入测试:")
    db.import_from_csv('rules_export.csv')
    print("导入后规则数量:", len(db.get_all_rules()))
    
    # 打印导入后的规则
    print("\n导入后的规则:")
    for rule in db.get_all_rules():
        print(json.dumps(rule, indent=2, ensure_ascii=False))
    
    db.close()