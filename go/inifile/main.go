package main

import (
	"fmt"
	"os"
)

func printUsage() {
	fmt.Println("使用方法:")
	fmt.Println("  ini_config <文件路径> get <节名> <键名>")
	fmt.Println("  ini_config <文件路径> set <节名> <键名> <值>")
	fmt.Println("  ini_config <文件路径> del <节名> <键名>")
	fmt.Println("  ini_config <文件路径> list <节名>")
	os.Exit(1)
}

func main() {
	if len(os.Args) < 4 {
		printUsage()
	}

	filePath := os.Args[1]
	action := os.Args[2]
	section := os.Args[3]

	// 初始化 INIHandler
	handler, err := NewINIHandler(filePath)
	if err != nil {
		fmt.Printf("错误: %v\n", err)
		os.Exit(1)
	}

	switch action {
	case "get":
		if len(os.Args) < 5 {
			printUsage()
		}
		key := os.Args[4]
		value, err := handler.Get(section, key)
		if err != nil {
			fmt.Printf("错误: %v\n", err)
			os.Exit(1)
		}
		fmt.Println(value)

	case "set":
		if len(os.Args) < 6 {
			printUsage()
		}
		key := os.Args[4]
		value := os.Args[5]
		if err := handler.Set(section, key, value); err != nil {
			fmt.Printf("错误: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("已设置 %s=%s 于节 [%s]\n", key, value, section)

	case "del":
		if len(os.Args) < 5 {
			printUsage()
		}
		key := os.Args[4]
		if err := handler.Delete(section, key); err != nil {
			fmt.Printf("错误: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("已删除键 %s 于节 [%s]\n", key, section)

	case "list":
		data, err := handler.List(section)
		if err != nil {
			fmt.Printf("错误: %v\n", err)
			os.Exit(1)
		}
		for key, value := range data {
			fmt.Printf("%s=%s\n", key, value)
		}

	default:
		fmt.Printf("未知操作: %s\n", action)
		printUsage()
	}
}
