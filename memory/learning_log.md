# 学习日志

## 2026-05-19

### 已完成

- 创建项目记忆系统
- 明确项目是一个 C++ + Qt 的简化聊天软件
- 初步确定先从 Qt Widgets、CMake 和基础 TCP 通信开始
- 检查本机开发环境：已安装 Qt 6.11.1，路径为 `C:\Qt\6.11.1\mingw_64`
- 已发现 Qt 自带 MinGW 13.1.0、CMake 3.30.5、Ninja 1.12.1 和 Qt Creator
- 确认开发 IDE 使用 VS Code
- 创建第一个 Qt Widgets + CMake 最小程序
- 成功使用 Qt 自带 CMake、Ninja 和 MinGW 编译生成 `build\ChatTest.exe`
- 初次直接运行失败，原因很可能是 Qt 运行时 DLL 未在 PATH 中
- 使用 `windeployqt` 后程序能进入 Qt 事件循环
- 新增 `scripts/run.ps1`，用于临时配置 Qt 运行环境并启动程序
- 配置 VS Code C/C++ 扩展读取 `build\compile_commands.json`，用于识别 Qt 头文件和编译参数

### 当前阶段

阶段 1：Qt 基础入门。

### 下一次建议学习内容

1. 运行第一个 Qt 窗口程序
2. 理解 `QApplication`、`QLabel`、`show()` 和 `app.exec()`
3. 把简单标签窗口改成登录窗口
4. 理解 Qt 的控件、布局和信号槽
