# 下一步任务

## 当前任务

项目刚刚启动，仓库已经加入记忆系统，Qt 环境可用，第一个 Qt Widgets 程序已经可以构建和运行，并已推送到 GitHub。当前学习状态是：已经理解第一个 `src\main.cpp` 的意思，包括 `QApplication`、`QLabel`、`show()` 和 `app.exec()`。目前已经把 `QLabel` 小窗口改成登录窗口雏形，并让登录按钮点击后通过信号槽读取用户名和密码、检查是否为空、更新窗口内的 `statusLabel`。用户已经理解：`&loginButton` 是取地址，lambda 捕获列表里的 `&` 是按引用捕获；`setEchoMode(QLineEdit::Password)` 用于密码隐藏；长期显示的提示控件应该提前创建，而不是在点击事件里临时创建。已经进一步把登录窗口拆成 `LoginWindow` 类，分别放在 `src\LoginWindow.h` 和 `src\LoginWindow.cpp`，`src\main.cpp` 只负责创建 `QApplication`、创建窗口、显示窗口和进入事件循环。

## 下一步

建议先不要急着写聊天功能，而是按下面顺序来：

1. 逐行讲解拆分后的 `src\main.cpp`、`src\LoginWindow.h` 和 `src\LoginWindow.cpp`
2. 巩固 `.h` / `.cpp` 分工、类声明与类实现
3. 重点理解成员变量、指针、`->`、`this`、Qt 父子对象自动释放
4. 再进入聊天窗口 UI
5. 最后开始网络通信

## 需要确认的问题

- IDE 已确定使用 VS Code。
- Qt 已安装：`C:\Qt\6.11.1\mingw_64`
- 当前 Qt 套件为 MinGW 版，不是 MSVC 版。
- GitHub 远程仓库：`https://github.com/xiaodu4346/chattest.git`
- 以后每完成一个清晰阶段，先询问用户是否提交 Git。
- 你的系统看起来是 Windows，我们后续会默认按 Windows 教程来写。

## 长期路线草案

### 阶段 0：项目启动

- 建立记忆系统
- 明确技术路线
- 检查环境

### 阶段 1：Qt 基础

- 创建窗口
- 学习控件
- 学习布局
- 学习信号槽
- 完成登录界面

### 阶段 2：聊天 UI

- 主窗口布局
- 好友列表
- 消息气泡
- 输入框和发送按钮
- 图片显示区域

### 阶段 3：本地假数据版

- 不联网，先模拟登录和聊天
- 理解界面和业务逻辑如何分开

### 阶段 4：网络通信

- 写一个简单服务器
- 客户端连接服务器
- 发送和接收 JSON 消息

### 阶段 5：图片发送

- 选择图片
- 读取文件
- 传输图片数据
- 接收后显示

### 阶段 6：数据保存

- 保存用户信息
- 保存聊天记录
- 学习 SQLite
