package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"

	"github.com/gorilla/websocket"
)

// 消息结构体，与服务器相同
type Message struct {
	Type string      `json:"type"`
	Data interface{} `json:"data"`
}

var token string // 用于存储从服务器获取的 token

// 登录函数，向服务器请求登录并获取 token
func login(username, password string) (string, error) {
	// 创建请求体
	data := map[string]string{
		"username": username,
		"password": password,
	}
	jsonData, err := json.Marshal(data)
	if err != nil {
		return "", err
	}

	// 向服务器发送登录请求
	resp, err := http.Post("https://localhost:8443/login", "application/json", bytes.NewBuffer(jsonData))
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	// 解析响应中的 token
	var result map[string]string
	if err := json.NewDecoder(resp.Body).Decode(&result); err != nil {
		return "", err
	}

	token, ok := result["token"]
	if !ok {
		return "", fmt.Errorf("Token not found in response")
	}

	return token, nil
}

// 连接 WebSocket 服务器并处理消息
func connectWebSocket(token string) {
	// 创建 http.Header 来包含 Authorization 请求头
	headers := http.Header{}
	headers.Add("Authorization", "Bearer "+token)

	// 连接到 WebSocket 服务器
	conn, _, err := websocket.DefaultDialer.Dial("wss://localhost:8443/ws", headers)
	if err != nil {
		log.Fatal("Error connecting to WebSocket:", err)
	}
	defer conn.Close()

	// 启动接收消息的 goroutine
	go receiveMessages(conn)

	// 发送 ping 消息并等待响应
	pingMessage := Message{
		Type: "ping",
		Data: "ping",
	}
	err = conn.WriteJSON(pingMessage)
	if err != nil {
		log.Println("Error sending ping:", err)
	}

	// 发送广播消息
	broadcastMessage := Message{
		Type: "broadcast",
		Data: "Hello, WebSocket Server!",
	}
	err = conn.WriteJSON(broadcastMessage)
	if err != nil {
		log.Println("Error sending broadcast:", err)
	}

	// 等待一段时间后退出
	time.Sleep(10 * time.Second)
}

// 接收 WebSocket 消息的函数
func receiveMessages(conn *websocket.Conn) {
	for {
		_, msgData, err := conn.ReadMessage()
		if err != nil {
			log.Println("Error reading message:", err)
			return
		}

		var msg Message
		if err := json.Unmarshal(msgData, &msg); err != nil {
			log.Println("Invalid JSON:", err)
			continue
		}

		switch msg.Type {
		case "pong":
			log.Println("Received pong:", msg.Data)
		case "broadcast":
			log.Println("Received broadcast:", msg.Data)
		default:
			log.Println("Received unknown message type:", msg.Type)
		}
	}
}

func main() {
	// 登录并获取 token
	token, err := login("admin", "123456")
	if err != nil {
		log.Fatal("Error during login:", err)
	}

	log.Println("Successfully logged in, received token:", token)

	// 使用 token 连接 WebSocket 服务器
	connectWebSocket(token)
}
