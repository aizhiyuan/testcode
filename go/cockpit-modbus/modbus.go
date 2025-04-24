// modbus.go
package main

import (
	"database/sql"
	"encoding/json"
	"fmt"

	_ "github.com/mattn/go-sqlite3"
)

// openDB opens a SQLite connection using the path from settings.ini
func openDB() (*sql.DB, error) {
	path, err := loadDbPath()
	if err != nil {
		return nil, err
	}
	return sql.Open("sqlite3", path)
}

// selectModbus runs SELECT queries for devices or commands
func selectModbus(table, typ, cond string) ([]byte, error) {
	db, err := openDB()
	if err != nil {
		return nil, err
	}
	defer db.Close()

	var sqlStmt string
	switch {
	case table == "device" && (typ == "mbRtu" || typ == "mbTcp"):
		sqlStmt = fmt.Sprintf("SELECT rowid, * FROM tb_set_device WHERE Type='%s'", cond)
	case table == "table" && (typ == "mbRead" || typ == "mbWrite"):
		sub := map[string]string{"mbRead": "tb_set_cmd_read", "mbWrite": "tb_set_cmd_write"}[typ]
		sqlStmt = fmt.Sprintf(
			`SELECT rowid, * FROM %s WHERE PortID IN (
                SELECT PortID FROM tb_set_device WHERE Type IN ('mb-rtu-m','mb-tcp-m')
            )`, sub)
	default:
		return json.Marshal(map[string]interface{}{"status": false, "error": "Invalid table or type"})
	}

	rows, err := db.Query(sqlStmt)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	cols, _ := rows.Columns()
	results := []map[string]interface{}{}
	for rows.Next() {
		vals := make([]interface{}, len(cols))
		ptrs := make([]interface{}, len(cols))
		for i := range vals {
			ptrs[i] = &vals[i]
		}
		if err := rows.Scan(ptrs...); err != nil {
			return nil, err
		}
		row := make(map[string]interface{})
		for i, col := range cols {
			row[col] = vals[i]
		}
		results = append(results, row)
	}
	return json.Marshal(map[string]interface{}{"status": true, "data": results})
}

// insertModbus inserts a device or command record
func insertModbus(table, typ string, raw []byte) ([]byte, error) {
	var m map[string]interface{}
	if err := json.Unmarshal(raw, &m); err != nil {
		return nil, err
	}
	db, err := openDB()
	if err != nil {
		return nil, err
	}
	defer db.Close()

	var sqlStmt string
	args := []interface{}{}
	switch {
	case table == "device" && typ == "mbRtu":
		sqlStmt = `INSERT INTO tb_set_device (RTUID, PortID, Enable, Type, CommPara1, CommPara2, CommPara3, CommPara4, CommPara5) VALUES ('200333001', ?, ?, ?, ?, ?, ?, ?, ?)`
		args = []interface{}{m["PortID"], m["Enable"], m["Type"], m["CommPara1"], m["CommPara2"], m["CommPara3"], m["CommPara4"], m["CommPara5"]}
	case table == "device" && typ == "mbTcp":
		sqlStmt = `INSERT INTO tb_set_device (RTUID, PortID, Enable, Type, CommPara1, CommPara2) VALUES ('200333001', ?, ?, ?, ?, ?)`
		args = []interface{}{m["PortID"], m["Enable"], m["Type"], m["CommPara1"], m["CommPara2"]}
	case table == "table" && typ == "mbRead":
		sqlStmt = `INSERT INTO tb_set_cmd_read (RTUID, PortID, DataLabel, DeviceID, FunCode, DataReadStart, DataReadLen, DataStorPiece, DataStorStart, EndianSwap, FaultValue, PollingInterval, CmdWaitTimeOut, CmdReadTimeOut) VALUES ('200333001', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`
		args = []interface{}{m["PortID"], m["DataLabel"], m["DeviceID"], m["FunCode"], m["DataReadStart"], m["DataReadLen"], m["DataStorPiece"], m["DataStorStart"], m["EndianSwap"], m["FaultValue"], m["PollingInterval"], m["CmdWaitTimeOut"], m["CmdReadTimeOut"]}
	case table == "table" && typ == "mbWrite":
		sqlStmt = `INSERT INTO tb_set_cmd_write (RTUID, PortID, DataLabel, DeviceID, Trigger, WatchPiece, WatchDataID, FunCode, DataWriteStart, DataWriteLen, DataStorPiece, DataStorStart, EndianSwap, PollingInterval, CmdWaitTimeOut, CmdReadTimeOut) VALUES ('200333001', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`
		args = []interface{}{m["PortID"], m["DataLabel"], m["DeviceID"], m["Trigger"], m["WatchPiece"], m["WatchDataID"], m["FunCode"], m["DataWriteStart"], m["DataWriteLen"], m["DataStorPiece"], m["DataStorStart"], m["EndianSwap"], m["PollingInterval"], m["CmdWaitTimeOut"], m["CmdReadTimeOut"]}
	default:
		return json.Marshal(map[string]interface{}{"status": false, "error": "Invalid table or type"})
	}

	res, err := db.Exec(sqlStmt, args...)
	if err != nil {
		return nil, err
	}
	id, _ := res.LastInsertId()
	return json.Marshal(map[string]interface{}{"status": true, "rowid": id})
}

// updateModbus updates a device or command record by rowid
func updateModbus(table, typ string, raw []byte) ([]byte, error) {
	var m map[string]interface{}
	if err := json.Unmarshal(raw, &m); err != nil {
		return nil, err
	}
	id := m["rowid"]
	db, err := openDB()
	if err != nil {
		return nil, err
	}
	defer db.Close()

	var sqlStmt string
	args := []interface{}{}
	switch {
	case table == "device" && typ == "mbRtu":
		sqlStmt = `UPDATE tb_set_device SET PortID=?, Enable=?, CommPara1=?, CommPara2=?, CommPara3=?, CommPara4=?, CommPara5=? WHERE rowid=?`
		args = []interface{}{m["PortID"], m["Enable"], m["CommPara1"], m["CommPara2"], m["CommPara3"], m["CommPara4"], m["CommPara5"], id}
	case table == "device" && typ == "mbTcp":
		sqlStmt = `UPDATE tb_set_device SET PortID=?, Enable=?, CommPara1=?, CommPara2=? WHERE rowid=?`
		args = []interface{}{m["PortID"], m["Enable"], m["CommPara1"], m["CommPara2"], id}
	case table == "table" && typ == "mbRead":
		sqlStmt = `UPDATE tb_set_cmd_read SET PortID=?, DataLabel=?, DeviceID=?, FunCode=?, DataReadStart=?, DataReadLen=?, DataStorPiece=?, DataStorStart=?, EndianSwap=?, FaultValue=?, PollingInterval=?, CmdWaitTimeOut=?, CmdReadTimeOut=? WHERE rowid=?`
		args = []interface{}{m["PortID"], m["DataLabel"], m["DeviceID"], m["FunCode"], m["DataReadStart"], m["DataReadLen"], m["DataStorPiece"], m["DataStorStart"], m["EndianSwap"], m["FaultValue"], m["PollingInterval"], m["CmdWaitTimeOut"], m["CmdReadTimeOut"], id}
	case table == "table" && typ == "mbWrite":
		sqlStmt = `UPDATE tb_set_cmd_write SET PortID=?, DataLabel=?, DeviceID=?, Trigger=?, WatchPiece=?, WatchDataID=?, FunCode=?, DataWriteStart=?, DataWriteLen=?, DataStorPiece=?, DataStorStart=?, EndianSwap=?, PollingInterval=?, CmdWaitTimeOut=?, CmdReadTimeOut=? WHERE rowid=?`
		args = []interface{}{m["PortID"], m["DataLabel"], m["DeviceID"], m["Trigger"], m["WatchPiece"], m["WatchDataID"], m["FunCode"], m["DataWriteStart"], m["DataWriteLen"], m["DataStorPiece"], m["DataStorStart"], m["EndianSwap"], m["PollingInterval"], m["CmdWaitTimeOut"], m["CmdReadTimeOut"], id}
	default:
		return json.Marshal(map[string]interface{}{"status": false, "error": "Invalid table or type"})
	}
	if _, err := db.Exec(sqlStmt, args...); err != nil {
		return nil, err
	}
	return json.Marshal(map[string]bool{"status": true})
}

// deleteModbus deletes a record by rowid
func deleteModbus(table, typ, id string) ([]byte, error) {
	db, err := openDB()
	if err != nil {
		return nil, err
	}
	defer db.Close()

	var sqlStmt string
	switch {
	case table == "device" && (typ == "mbRtu" || typ == "mbTcp"):
		sqlStmt = fmt.Sprintf("DELETE FROM tb_set_device WHERE rowid=%s", id)
	case table == "table" && (typ == "mbRead" || typ == "mbWrite"):
		sub := map[string]string{"mbRead": "tb_set_cmd_read", "mbWrite": "tb_set_cmd_write"}[typ]
		sqlStmt = fmt.Sprintf("DELETE FROM %s WHERE rowid=%s", sub, id)
	default:
		return json.Marshal(map[string]interface{}{"status": false, "error": "Invalid table or type"})
	}
	if _, err := db.Exec(sqlStmt); err != nil {
		return nil, err
	}
	return json.Marshal(map[string]bool{"status": true})
}

// clearModbus clears all command records
func clearModbus(table, typ string) ([]byte, error) {
	if table != "table" || (typ != "mbRead" && typ != "mbWrite") {
		return json.Marshal(map[string]interface{}{"status": false, "error": "Invalid table or type"})
	}
	db, err := openDB()
	if err != nil {
		return nil, err
	}
	defer db.Close()
	sub := map[string]string{"mbRead": "tb_set_cmd_read", "mbWrite": "tb_set_cmd_write"}[typ]
	sqlStmt := fmt.Sprintf("DELETE FROM %s", sub)
	if _, err := db.Exec(sqlStmt); err != nil {
		return nil, err
	}
	return json.Marshal(map[string]bool{"status": true})
}

// updateTimeModbus updates polling and timeout for all commands
func updateTimeModbus(typ string, raw []byte) ([]byte, error) {
	if typ != "mbRead" && typ != "mbWrite" {
		return json.Marshal(map[string]interface{}{"status": false, "error": "Wrong type selection, only mbRead and mbWrite allowed"})
	}
	var m map[string]int
	if err := json.Unmarshal(raw, &m); err != nil {
		return nil, err
	}
	db, err := openDB()
	if err != nil {
		return nil, err
	}
	defer db.Close()

	readSQL := fmt.Sprintf("UPDATE tb_set_cmd_read SET PollingInterval=%d, CmdWaitTimeOut=%d, CmdReadTimeOut=%d", m["pollval"], m["waittimeout"], m["readtimeout"])
	writeSQL := fmt.Sprintf("UPDATE tb_set_cmd_write SET PollingInterval=%d, CmdWaitTimeOut=%d, CmdReadTimeOut=%d", m["pollval"], m["waittimeout"], m["readtimeout"])
	if _, err := db.Exec(readSQL); err != nil {
		return nil, err
	}
	if _, err := db.Exec(writeSQL); err != nil {
		return nil, err
	}
	return json.Marshal(map[string]bool{"status": true})
}
