import sqlite3
import json
from typing import Dict, List

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
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            number TEXT,
            enable TEXT,  -- SQLite没有布尔类型，用TEXT存储
            name TEXT,
            mode TEXT,
            network1 TEXT,
            reg_addr1 TEXT,
            data_unit1 TEXT,
            data_bit1 TEXT,
            trigger_method TEXT,
            operators TEXT,
            trigger_conditions TEXT,
            trigger_value TEXT,
            func_name TEXT,
            out_network TEXT,
            out_reg_addr TEXT,
            out_data_unit TEXT,
            out_data_bit TEXT,
            type TEXT
        )
        ''')
        
        # 创建group_data子表
        cursor.execute('''
        CREATE TABLE IF NOT EXISTS rule_group_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            rule_id INTEGER,
            logical_conditions TEXT,
            network TEXT,
            reg_addr TEXT,
            data_unit TEXT,
            data_bit TEXT,
            position INTEGER,
            FOREIGN KEY (rule_id) REFERENCES rules(id)
        )
        ''')
        
        self.conn.commit()
    
    def insert_rule(self, rule_data: Dict):
        """插入一条完整的规则数据"""
        cursor = self.conn.cursor()
        
        # 插入主表数据
        cursor.execute('''
        INSERT INTO rules (
            number, enable, name, mode, network1, reg_addr1, data_unit1, data_bit1,
            trigger_method, operators, trigger_conditions, trigger_value, func_name,
            out_network, out_reg_addr, out_data_unit, out_data_bit, type
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            rule_data.get('number'),
            str(rule_data.get('enable')),  # 转换为字符串存储
            rule_data.get('name'),
            rule_data.get('mode'),
            rule_data.get('network1'),
            rule_data.get('reg_addr1'),
            rule_data.get('data_unit1'),
            rule_data.get('data_bit1'),
            rule_data.get('trigger_method'),
            rule_data.get('operators'),
            rule_data.get('trigger_conditions'),
            rule_data.get('trigger_value'),
            rule_data.get('func_name'),
            rule_data.get('out_network'),
            rule_data.get('out_reg_addr'),
            rule_data.get('out_data_unit'),
            rule_data.get('out_data_bit'),
            rule_data.get('type')
        ))
        
        rule_id = cursor.lastrowid
        
        # 插入group_data数据
        if 'group_data' in rule_data:
            for index, group_item in enumerate(rule_data['group_data']):
                cursor.execute(''' 
                INSERT INTO rule_group_data (
                    rule_id, logical_conditions, network, reg_addr, 
                    data_unit, data_bit, position
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
                ''', (
                    rule_id,
                    group_item.get('logical_conditions'),
                    group_item.get('network'),
                    group_item.get('reg_addr'),
                    group_item.get('data_unit'),
                    group_item.get('data_bit'),
                    index  # 保持原始顺序
                ))
        
        self.conn.commit()
        return rule_id
    
    def get_rule(self, rule_id: int) -> Dict:
        """根据ID获取完整规则数据"""
        cursor = self.conn.cursor()
        
        # 获取主表数据
        cursor.execute('SELECT * FROM rules WHERE id = ?', (rule_id,))
        rule_row = cursor.fetchone()
        
        if not rule_row:
            return None
        
        # 获取group_data数据
        cursor.execute(''' 
        SELECT logical_conditions, network, reg_addr, data_unit, data_bit 
        FROM rule_group_data 
        WHERE rule_id = ? 
        ORDER BY position
        ''', (rule_id,))
        
        group_items = []
        for item in cursor.fetchall():
            group_items.append({
                'logical_conditions': item[0],
                'network': item[1],
                'reg_addr': item[2],
                'data_unit': item[3],
                'data_bit': item[4]
            })
        
        # 构建返回的字典
        rule_data = {
            'id': rule_row[0],
            'number': rule_row[1],
            'enable': rule_row[2] == 'True',  # 转换回布尔值
            'name': rule_row[3],
            'mode': rule_row[4],
            'network1': rule_row[5],
            'reg_addr1': rule_row[6],
            'data_unit1': rule_row[7],
            'data_bit1': rule_row[8],
            'trigger_method': rule_row[9],
            'operators': rule_row[10],
            'trigger_conditions': rule_row[11],
            'trigger_value': rule_row[12],
            'func_name': rule_row[13],
            'out_network': rule_row[14],
            'out_reg_addr': rule_row[15],
            'out_data_unit': rule_row[16],
            'out_data_bit': rule_row[17],
            'type': rule_row[18],
            'group_data': group_items
        }
        
        return rule_data
    
    def search_rules(self, **kwargs) -> List[Dict]:
        """根据条件搜索规则"""
        # 这里可以扩展实现更复杂的搜索逻辑
        cursor = self.conn.cursor()
        
        # 简单示例：根据number搜索
        if 'number' in kwargs:
            cursor.execute('SELECT id FROM rules WHERE number = ?', (kwargs['number'],))
            rule_ids = [row[0] for row in cursor.fetchall()]
            return [self.get_rule(rule_id) for rule_id in rule_ids]
        
        return []
    
    def close(self):
        """关闭数据库连接"""
        self.conn.close()

# 使用示例
if __name__ == '__main__':
    # 示例JSON数据
    example_data1 = {
        "number": "001",
        "enable": True,
        "name": "测试规则",
        "mode": "自动",
        "network1": "192.168.1.1",
        "reg_addr1": "0x1000",
        "data_unit1": "byte",
        "data_bit1": "8",
        "trigger_method": "边缘触发",
        "operators": "AND",
        "trigger_conditions": ">",
        "trigger_value": "10",
        "func_name": "报警功能",
        "out_network": "192.168.2.1",
        "out_reg_addr": "0x2000",
        "out_data_unit": "word",
        "out_data_bit": "16",
        "type": "add",
        "group_data": [
            {
                "logical_conditions": "AND",
                "network": "192.168.1.2",
                "reg_addr": "0x1100",
                "data_unit": "byte",
                "data_bit": "8"
            },
            {
                "logical_conditions": "OR",
                "network": "192.168.1.3",
                "reg_addr": "0x1200",
                "data_unit": "word",
                "data_bit": "16"
            }
        ]
    }
    
    example_data2 = {
        "number": "002",
        "enable": True,
        "name": "测试规则",
        "mode": "自动",
        "network1": "192.168.1.1",
        "reg_addr1": "0x1000",
        "data_unit1": "byte",
        "data_bit1": "8",
        "trigger_method": "边缘触发",
        "operators": "AND",
        "trigger_conditions": ">",
        "trigger_value": "10",
        "func_name": "报警功能",
        "out_network": "192.168.2.1",
        "out_reg_addr": "0x2000",
        "out_data_unit": "word",
        "out_data_bit": "16",
        "type": "add",
        "group_data": [
            {
                "logical_conditions": "AND",
                "network": "192.168.1.2",
                "reg_addr": "0x1100",
                "data_unit": "byte",
                "data_bit": "8"
            },
            {
                "logical_conditions": "OR",
                "network": "192.168.1.3",
                "reg_addr": "0x1200",
                "data_unit": "word",
                "data_bit": "16"
            },
            {
                "logical_conditions": "OR",
                "network": "192.168.1.4",
                "reg_addr": "0x1200",
                "data_unit": "word",
                "data_bit": "16"
            },
            {
                "logical_conditions": "OR",
                "network": "192.168.1.5",
                "reg_addr": "0x1200",
                "data_unit": "word",
                "data_bit": "16"
            }
            
        ]
    }

    # 初始化数据库
    db = RuleDatabase()
    
    # 插入数据
    rule_id1 = db.insert_rule(example_data1)
    print(f"插入成功，规则ID: {rule_id1}")
    
    rule_id2 = db.insert_rule(example_data2)
    print(f"插入成功，规则ID: {rule_id2}")
    
    # 查询数据
    rule = db.get_rule(rule_id1)
    print("查询结果:")
    print(json.dumps(rule, indent=2, ensure_ascii=False))
    
    rule = db.get_rule(rule_id2)
    print("查询结果:")
    print(json.dumps(rule, indent=2, ensure_ascii=False))
    
    # 关闭连接
    db.close()
