// config.go
package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"

	"github.com/go-ini/ini"
)

// Get path to settings.ini in same folder
func getIniPath() (string, error) {
	exePath, err := os.Executable()
	if err != nil {
		return "", err
	}
	dir := filepath.Dir(exePath)
	return filepath.Join(dir, "settings.ini"), nil
}

// Load database path
func loadDbPath() (string, error) {
	iniPath, err := getIniPath()
	if err != nil {
		return "", err
	}
	cfg, err := ini.Load(iniPath)
	if err != nil {
		return "", err
	}
	return cfg.Section("database").Key("path").String(), nil
}

// Get general config: time, mode, or all
func getGeneralConfig(typ string) ([]byte, error) {
	iniPath, err := getIniPath()
	if err != nil {
		return nil, err
	}
	cfg, err := ini.Load(iniPath)
	if err != nil {
		return nil, err
	}

	sec := cfg.Section("general")
	result := make(map[string]interface{})
	switch typ {
	case "time":
		result["pollval"], _ = sec.Key("pollval").Int()
		result["waittimeout"], _ = sec.Key("waittimeout").Int()
		result["readtimeout"], _ = sec.Key("readtimeout").Int()
	case "mode":
		result["selectmode"] = sec.Key("selectmode").MustString("1")
	case "all":
		result["pollval"], _ = sec.Key("pollval").Int()
		result["waittimeout"], _ = sec.Key("waittimeout").Int()
		result["readtimeout"], _ = sec.Key("readtimeout").Int()
		result["selectmode"] = sec.Key("selectmode").MustString("2")
	default:
		return json.Marshal(map[string]interface{}{"status": false, "error": "Title does not exist"})
	}
	return json.Marshal(map[string]interface{}{"status": true, "data": result})
}

// Set general config
func setGeneralConfig(data []byte) ([]byte, error) {
	iniPath, err := getIniPath()
	if err != nil {
		return nil, err
	}
	cfg, err := ini.Load(iniPath)
	if err != nil {
		return nil, err
	}
	sec := cfg.Section("general")
	var m map[string]interface{}
	if err := json.Unmarshal(data, &m); err != nil {
		return nil, err
	}
	for k, v := range m {
		sec.Key(k).SetValue(fmt.Sprint(v))
	}
	if err := cfg.SaveTo(iniPath); err != nil {
		return nil, err
	}
	return json.Marshal(map[string]bool{"status": true})
}
