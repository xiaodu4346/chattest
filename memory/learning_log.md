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
- 新增 `scripts/build.ps1`，用于通过 CMake 固定构建 Qt 项目，避免误用单文件 gcc 编译

## 2026-05-20

### 已完成

- 解释 VS Code 不是编译器，真正编译 C++ 的是 MSVC、MinGW 或 Clang
- 解释 Qt、VS Code、CMake、Ninja、MinGW 的分工
- 解释 `main.cpp` 不能直接运行，必须先编译生成 `ChatTest.exe`
- 处理 VS Code 中 `QApplication` include 报错：使用 `compile_commands.json` 给 IntelliSense 提供真实编译参数
- 完成第一次 Git 提交：`Initialize Qt chat project`
- 配置 GitHub 远程仓库：`https://github.com/xiaodu4346/chattest.git`
- 合并 GitHub 上已有的远程初始提交，保留本地完整 README
- 成功推送 `main` 分支到 GitHub
- 先逐行讲解了第一个 `QLabel` 示例窗口
- 理解了 `QApplication`、`QLabel`、`show()` 和 `app.exec()` 的基本作用
- 把 `QLabel` 示例窗口改成登录窗口雏形
- 初步学习 `QWidget`、`QLineEdit`、`QPushButton` 和 `QVBoxLayout`
- 初步学习 Qt 信号槽：使用 `QObject::connect` 让登录按钮点击后更新窗口标题
- 让登录按钮同时读取用户名和密码，并检查两个输入框是否为空
- 把错误提示从窗口标题改为窗口内的 `QLabel statusLabel`
- 理解 `&loginButton` 是取地址，而 lambda 捕获列表里的 `[&window]` / `[&statusLabel]` 是按引用捕获
- 理解 `passwordEdit.setEchoMode(QLineEdit::Password)` 用于让密码输入框隐藏明文输入，用户名框不需要这一行
- 理解控件生命周期：不要在点击事件的 `if` 代码块里临时创建错误提示 `QLabel`，应该提前创建长期存在的 `statusLabel`，事件发生时只调用 `setText()` 修改文字

### 当前阶段

阶段 1：Qt 基础入门。

### 下一次建议学习内容

1. 从当前登录窗口版本继续
2. 逐行复习当前 `src\main.cpp`
3. 进一步理解 Qt 信号槽、lambda 捕获和控件生命周期
4. 考虑把界面代码从 `main.cpp` 拆到单独的登录窗口类
