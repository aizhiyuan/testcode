// main.go
package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
)

var logger *log.Logger

func main() {
	logger = initLogger()
	logger.Printf("[main] 用户: %s", os.Getenv("USER"))
	if len(os.Args) < 2 {
		fmt.Println(`{"status":false,"error":"参数长度错误"}`)
		os.Exit(1)
	}
	mode := os.Args[1]
	var output []byte
	var err error

	switch mode {
	case "readini":
		output, err = getGeneralConfig(os.Args[2])
	case "writeini":
		b, _ := json.Marshal(os.Args[2])
		output, err = setGeneralConfig(b)
	case "select":
		output, err = selectModbus(os.Args[2], os.Args[3], os.Args[4])
	case "isrun":
		output, err = isProgramRunning(os.Args[2])
	case "start":
		output, err = startProgram(os.Args[2])
	case "stop":
		output, err = stopProgram(os.Args[2])
	case "restart":
		output, err = restartProgram(os.Args[2])
	default:
		output, _ = json.Marshal(map[string]interface{}{"status": false, "error": "无效模式"})
	}
	if err != nil {
		logger.Printf("[main] error: %v", err)
		fmt.Println(string(output))
		os.Exit(1)
	}
	fmt.Println(string(output))
}
