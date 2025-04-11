package main

import (
	"crypto/rand"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

// 自定义 token 长度和字符集
const tokenLength = 48                                                                      // 定义 token 的长度
var tokenCharset = []byte("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") // 定义 token 可用字符集

// Session 结构体，用于 token 存储
type Session struct {
	UserID    string    // 用户 ID
	ExpiresAt time.Time // token 过期时间
}

var (
	sessionStore = make(map[string]Session) // 存储 token 和对应 Session 的 map
	sessionMutex = sync.RWMutex{}           // 读写锁，保证 session 的线程安全
)

// 消息结构体，WebSocket 中的消息类型和数据
type Message struct {
	Type string      `json:"type"` // 消息类型
	Data interface{} `json:"data"` // 消息内容
}

// 客户端结构体，代表每一个连接的客户端
type Client struct {
	conn     *websocket.Conn // WebSocket 连接
	send     chan Message    // 发送消息的通道
	userID   string          // 客户端的用户 ID
	lastPing time.Time       // 最后一次 ping 的时间，用于检测客户端是否在线
}

var (
	clients   = make(map[*Client]bool) // 存储所有连接的客户端
	broadcast = make(chan Message)     // 广播消息的通道
	mutex     = sync.Mutex{}           // 互斥锁，保证并发安全
	upgrader  = websocket.Upgrader{    // WebSocket 升级器，用于将 HTTP 请求升级为 WebSocket 连接
		CheckOrigin: func(r *http.Request) bool { return true }, // 允许任何来源的请求
	}
)

// 生成随机 token
func generateToken(length int) (string, error) {
	b := make([]byte, length)
	if _, err := rand.Read(b); err != nil { // 使用加密安全的随机数生成器生成 token
		return "", err
	}
	token := make([]byte, length)
	for i := range b {
		token[i] = tokenCharset[int(b[i])%len(tokenCharset)] // 将生成的随机字节映射到字符集
	}
	return string(token), nil
}

// 存储 session 到 sessionStore
func storeSession(token, userID string) {
	sessionMutex.Lock() // 锁定 sessionStore
	defer sessionMutex.Unlock()
	sessionStore[token] = Session{ // 将 token 和对应的 session 存储到 map 中
		UserID:    userID,
		ExpiresAt: time.Now().Add(2 * time.Hour), // 设置 token 过期时间为 2 小时后
	}
}

// 验证 token 是否有效
func validateToken(r *http.Request) (string, error) {
	header := r.Header.Get("Authorization")    // 获取请求头中的 Authorization 字段
	if !strings.HasPrefix(header, "Bearer ") { // 检查是否是 Bearer token 格式
		return "", fmt.Errorf("Missing or malformed token")
	}
	token := strings.TrimPrefix(header, "Bearer ") // 去掉 "Bearer " 前缀

	sessionMutex.RLock() // 只读锁定 sessionStore
	defer sessionMutex.RUnlock()
	s, ok := sessionStore[token]              // 查找 session
	if !ok || time.Now().After(s.ExpiresAt) { // 检查 session 是否存在或是否过期
		return "", fmt.Errorf("Invalid or expired token")
	}
	return s.UserID, nil // 返回用户 ID
}

// 登录接口，使用固定账号密码进行测试
func loginHandler(w http.ResponseWriter, r *http.Request) {
	username := r.FormValue("username")              // 获取用户名
	password := r.FormValue("password")              // 获取密码
	if username != "admin" || password != "123456" { // 检查用户名和密码
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}
	token, err := generateToken(tokenLength) // 生成 token
	if err != nil {
		http.Error(w, "Token generation failed", http.StatusInternalServerError)
		return
	}
	storeSession(token, username)                                // 存储 session
	json.NewEncoder(w).Encode(map[string]string{"token": token}) // 返回 token 给客户端
}

// WebSocket 主处理函数
func wsHandler(w http.ResponseWriter, r *http.Request) {
	userID, err := validateToken(r) // 验证 token
	if err != nil {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	conn, err := upgrader.Upgrade(w, r, nil) // 将 HTTP 请求升级为 WebSocket 连接
	if err != nil {
		log.Println("WebSocket upgrade error:", err)
		return
	}

	// 创建新的客户端
	client := &Client{
		conn:     conn,
		send:     make(chan Message), // 创建发送消息的通道
		userID:   userID,
		lastPing: time.Now(),
	}

	mutex.Lock() // 锁定 clients，避免并发问题
	clients[client] = true
	mutex.Unlock()

	log.Printf("User %s connected. Total clients: %d", userID, len(clients))

	// 启动读写 goroutine
	go client.writePump()
	client.readPump()
}

// 读取消息的函数
func (c *Client) readPump() {
	defer func() {
		mutex.Lock()
		delete(clients, c) // 客户端断开时从 clients 中删除
		mutex.Unlock()
		c.conn.Close() // 关闭 WebSocket 连接
		log.Printf("User %s disconnected.", c.userID)
	}()

	c.conn.SetReadLimit(512)                                 // 设置读取消息的最大字节数
	c.conn.SetReadDeadline(time.Now().Add(60 * time.Second)) // 设置读取超时为 60 秒
	c.conn.SetPongHandler(func(appData string) error {       // 设置 pong 处理函数，用于维持连接
		c.lastPing = time.Now()
		c.conn.SetReadDeadline(time.Now().Add(60 * time.Second)) // 重设超时
		return nil
	})

	for {
		_, msgData, err := c.conn.ReadMessage() // 读取消息
		if err != nil {
			log.Println("Read error:", err)
			break
		}
		var msg Message
		if err := json.Unmarshal(msgData, &msg); err != nil { // 解析 JSON 消息
			log.Println("Invalid JSON:", err)
			continue
		}
		switch msg.Type {
		case "ping": // 如果消息类型是 ping，回应 pong
			c.send <- Message{Type: "pong", Data: "pong"}
		case "broadcast": // 如果消息类型是 broadcast，发送广播
			broadcast <- Message{Type: "broadcast", Data: fmt.Sprintf("%s: %v", c.userID, msg.Data)}
		}
	}
}

// 发送消息的函数
func (c *Client) writePump() {
	for msg := range c.send { // 从 send 通道中获取消息并发送
		c.conn.SetWriteDeadline(time.Now().Add(10 * time.Second)) // 设置写入超时
		if err := c.conn.WriteJSON(msg); err != nil {             // 发送消息
			log.Println("Write error:", err)
			break
		}
	}
}

// 处理广播消息
func handleBroadcasts() {
	for msg := range broadcast { // 从广播通道获取消息并发送给所有客户端
		mutex.Lock()
		for client := range clients {
			select {
			case client.send <- msg: // 发送消息
			default:
				close(client.send)      // 如果客户端无法接收消息，关闭其通道
				delete(clients, client) // 从 clients 中删除该客户端
			}
		}
		mutex.Unlock()
	}
}

// 启动 WebSocket 服务器
func main() {
	http.HandleFunc("/login", loginHandler) // 登录接口
	http.HandleFunc("/ws", wsHandler)       // WebSocket 接口
	go handleBroadcasts()                   // 启动广播处理 goroutine

	log.Println("Starting secure WebSocket server on https://localhost:8443")
	err := http.ListenAndServeTLS(":8443", "cert.pem", "key.pem", nil) // 启动带 HTTPS 的 WebSocket 服务器
	if err != nil {
		log.Fatal("ListenAndServeTLS Error:", err)
	}
}
