// logger.go
package main

import (
	"log"

	"gopkg.in/natefinch/lumberjack.v2"
)

func initLogger() *log.Logger {
	rotating := &lumberjack.Logger{
		Filename:   "/tmp/cockpit-modbus.log",
		MaxSize:    1, // megabytes
		MaxBackups: 0,
	}
	logger := log.New(rotating, "", log.LstdFlags)
	return logger
}
