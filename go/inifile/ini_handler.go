package main

import (
	"fmt"

	"gopkg.in/ini.v1"
)

type INIHandler struct {
	filePath string
	cfg      *ini.File
}

// NewINIHandler 初始化 INIHandler 并加载文件
func NewINIHandler(filePath string) (*INIHandler, error) {
	cfg, err := ini.Load(filePath)
	if err != nil {
		return nil, fmt.Errorf("加载 INI 文件失败: %w", err)
	}
	return &INIHandler{
		filePath: filePath,
		cfg:      cfg,
	}, nil
}

// Get 获取指定节和键的值
func (h *INIHandler) Get(section, key string) (string, error) {
	if !h.cfg.Section(section).HasKey(key) {
		return "", fmt.Errorf("键 '%s' 不存在于节 [%s] 中", key, section)
	}
	return h.cfg.Section(section).Key(key).String(), nil
}

// Set 设置指定节和键的值，如果节不存在则自动创建
func (h *INIHandler) Set(section, key, value string) error {
	h.cfg.Section(section).Key(key).SetValue(value)
	return h.cfg.SaveTo(h.filePath)
}

// Delete 删除指定节中的键
func (h *INIHandler) Delete(section, key string) error {
	if !h.cfg.Section(section).HasKey(key) {
		return fmt.Errorf("键 '%s' 不存在于节 [%s] 中", key, section)
	}
	h.cfg.Section(section).DeleteKey(key)
	return h.cfg.SaveTo(h.filePath)
}

// List 列出指定节中的所有键值对
func (h *INIHandler) List(section string) (map[string]string, error) {
	sec := h.cfg.Section(section)
	if sec == nil {
		return nil, fmt.Errorf("节 [%s] 不存在", section)
	}
	result := make(map[string]string)
	for _, key := range sec.Keys() {
		result[key.Name()] = key.Value()
	}
	return result, nil
}
