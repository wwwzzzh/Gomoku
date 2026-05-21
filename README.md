# Gobang Qt Client/Server

基于 Qt (C++17) 的五子棋（Gomoku）联机对战项目，包含 TCP 服务端和 Qt Widgets 图形客户端，支持**账号注册/登录**、**好友对战**、**人机对战**（本地 AI + HTTP AI）功能。

---

## 项目结构

```
Gomoku/
├── CMakeLists.txt                  # 根 CMake 配置（Qt5/Qt6 自动检测、子模块引入）
├── CMakePresets.json               # CMake 预设（debug 构建）
├── README.md
│
├── shared/                         # 共享库 (INTERFACE)
│   ├── CMakeLists.txt
│   └── include/
│       ├── gobang_types.h          # 游戏类型定义
│       └── protocol.h              # 网络协议序列化/反序列化工具
│
├── server/                         # TCP 服务端
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp                # 服务端入口
│       ├── gobang_server.h         # 服务端核心类声明
│       ├── gobang_server.cpp       # 服务端核心实现（连接管理、房间管理、胜负判定）
│       ├── account_store.h         # 账号存储类声明
│       └── account_store.cpp       # SQLite 数据库账号管理（注册/登录/密码哈希）
│
├── client/                         # Qt Widgets 客户端
│   ├── CMakeLists.txt
│   └── src/
│       ├── main.cpp                # 客户端入口（主流程编排）
│       ├── login_dialog.h          # 登录/注册对话框声明
│       ├── login_dialog.cpp        # 登录/注册对话框实现（UI + 暗色主题样式）
│       ├── network_client.h        # TCP 网络客户端声明
│       ├── network_client.cpp      # TCP 网络客户端实现（连接/消息收发/信号分发）
│       ├── main_window.h           # 主窗口声明
│       ├── main_window.cpp         # 主窗口实现（布局 + 好友/人机切换 + 深色样式）
│       ├── game_controller.h       # 游戏控制器声明
│       ├── game_controller.cpp     # 游戏控制器实现（两种模式逻辑 + 胜负判断）
│       ├── game_board_widget.h     # 棋盘组件声明
│       ├── game_board_widget.cpp   # 棋盘绘制与交互（星位、棋子渐变、最后一步高亮）
│       ├── ai_engine.h             # AI 引擎声明
│       └── ai_engine.cpp           # AI 引擎实现（HTTP AI 接口 + 本地策略搜索）
│
└── build-verify/                   # 构建验证目录（已编译产物）
```

---

## 各模块说明

### 1. shared — 共享库

纯头文件库，提供全局游戏类型和网络协议工具。

| 文件 | 说明 |
|------|------|
| [gobang_types.h](shared/include/gobang_types.h) | 15x15 棋盘、`StoneColor`（空/黑/白）、`AiDifficulty`（简单/普通/困难）等类型定义 |
| [protocol.h](shared/include/protocol.h) | JSON 消息的创建、序列化（`\n` 分隔行协议）、反序列化工具函数 |

### 2. server — TCP 服务端

基于 `QTcpServer` 的并发 TCP 服务端，负责账号认证、在线用户管理、对局房间调度。

| 文件 | 说明 |
|------|------|
| [main.cpp](server/src/main.cpp) | 创建 `QCoreApplication`，启动服务端监听 `7777` 端口 |
| [gobang_server.h](server/src/gobang_server.h) / [.cpp](server/src/gobang_server.cpp) | 核心类：`QTcpServer` 监听新连接，维护 `socket <-> 用户` 映射、在线列表、`Room` 房间对象（包含黑/白玩家、棋盘状态、当前回合），处理注册、登录、邀请、落子、五子连珠判定等消息 |
| [account_store.h](server/src/account_store.h) / [.cpp](server/src/account_store.cpp) | SQLite 账号存取：首次运行自动建库建表（`accounts` 表），密码以 SHA-256 哈希存储，支持用户名唯一性校验、长度校验（用户名 ≤ 32 字符，密码 ≥ 6 位） |

**服务端消息处理流程**（[gobang_server.cpp](server/src/gobang_server.cpp#L118-L248)）：
- `register` → 注册账号 → 返回 `register_reply`
- `login` → 验证登录 → 记录在线状态 → 广播 `online_list` → 返回 `login_reply`
- `invite` → 转发邀请给目标用户 → 对方决定接受/拒绝
- `invite_reply` → 处理邀请回应 → 接受则创建 Room 并发送 `game_start`
- `move` → 校验合法性 → 更新棋盘 → 转发给对手 → 检查五子连珠 → 有胜者则结束房间
- `game_over` → 手动结束对局（认输/退出）

### 3. client — Qt Widgets 客户端

图形界面客户端，使用 Qt Widgets 构建，通过 TCP 连接服务端。

#### 3.1 客户端入口 [main.cpp](client/src/main.cpp)

编排客户端主流程：
1. 创建 `NetworkClient`、`LoginDialog`、`MainWindow`
2. 处理连接/登录/注册的完整状态机：
   - 用户点击登录/注册时触发连接请求
   - 连接成功后自动重发未完成的认证请求
   - 登录成功后接受对话框并显示主窗口

#### 3.2 网络层 — [network_client.h](client/src/network_client.h) / [.cpp](client/src/network_client.cpp)

- 封装 `QTcpSocket`，提供 `connectToServer`、`login`、`registerAccount`、`invitePlayer`、`replyInvite`、`sendMove`、`sendGameOver` 等方法
- 读取 `\n` 分隔的 JSON 行协议，按 `type` 字段分发信号（`loginResult`、`gameStarted`、`opponentMoved` 等 12+ 种信号）

#### 3.3 登录对话框 — [login_dialog.h](client/src/login_dialog.h) / [.cpp](client/src/login_dialog.cpp)

- 深色渐变主题的登录表单
- 输入：服务器地址、端口（默认 127.0.0.1:7777）、用户名、密码
- 支持连接/登录/注册操作，带 `setBusy` 忙状态和 `setStatusText` 状态提示

#### 3.4 主窗口 — [main_window.h](client/src/main_window.h) / [.cpp](client/src/main_window.cpp)

左侧边栏 + 右侧棋盘（`QSplitter` / `QHBoxLayout` 布局）：
- 左面板：用户标识、`QTabWidget` 双标签页
  - **好友对战**标签：在线用户列表、邀请输入框、邀请按钮
  - **人机对战**标签：AI 难度下拉框（简单/普通/困难）、AI HTTP 接口地址输入框、开始按钮
- 右侧：`GameBoardWidget` 棋盘组件
- 深色主题样式（`QSS`）

#### 3.5 游戏控制器 — [game_controller.h](client/src/game_controller.h) / [.cpp](client/src/game_controller.cpp)

两种游戏模式的逻辑中枢：

| 模式 | 说明 |
|------|------|
| **好友对战（Friend）** | 通过服务端转发落子，接收对手移动，本地校验胜负 |
| **人机对战（AI）** | 本地落子后调用 AI 引擎（HTTP 或本地），自动切换回合 |

核心职责：
- 处理棋盘点击事件（`onBoardCellClicked`）
- 落子后调用 `afterMovePlaced` 更新棋盘并通知网络层
- 五子连珠判定（`hasFiveAt`，四方向检查）
- 胜负处理（`handleWin`）：中间消息显示"你赢了"/"你输了"
- 横幅信息刷新（`refreshBoardBanner`）：回合数 + 当前行动方

#### 3.6 棋盘组件 — [game_board_widget.h](client/src/game_board_widget.h) / [.cpp](client/src/game_board_widget.cpp)

基于 `QWidget` 自绘的 15×15 棋盘：

| 特性 | 说明 |
|------|------|
| 棋盘背景 | 木质渐变（`#f6ddb2` → `#d7a96a`），圆角矩形 |
| 全局背景 | 深色渐变（`#2b2f3a` → `#171a23`） |
| 星位标记 | 5 个传统星位（3,3 / 3,11 / 7,7 / 11,3 / 11,11） |
| 棋子绘制 | 径向渐变（黑白均有高光），略小于格子间距 |
| 最后一步高亮 | 橙色圆环 + 淡黄色外环 |
| 消息显示 | 顶部横幅（`m_gameLabel`）+ 中央大号消息（`m_centerMessage`，如"你赢了"） |
| 交互 | `mousePressEvent` 坐标转换 → 格子定位 → 发射 `cellClicked` 信号 |

#### 3.7 AI 引擎 — [ai_engine.h](client/src/ai_engine.h) / [.cpp](client/src/ai_engine.cpp)

支持两种 AI 模式：

| 模式 | 说明 |
|------|------|
| **HTTP AI** | 向指定 HTTP 接口 POST 当前棋盘状态和难度，期望返回 `{"row": N, "col": N}` 格式 JSON |
| **本地策略 AI** | 基于评分算法的本地搜索：遍历所有空位，从进攻（AI 棋子）和防守（人类棋子）两个维度评分，选择最高分位置 |

**本地 AI 评分策略**（[ai_engine.cpp](client/src/ai_engine.cpp#L71-L124)）：
- 对每个空位，沿四个方向（水平、垂直、两条对角线）统计连续同色棋子的数量
- 不同连子数给予不同权重：5 连 ≥ 100000，活四 = 10000，活三 = 2000，活二 = 200
- 困难模式下加入距中心距离奖励
- 简单模式下优先选择靠近棋盘中心的位置

---

## 构建

项目使用 CMake，根 CMakeLists.txt 已配置 Qt `Widgets / Network / Sql` 依赖，并自动检测 Qt5/Qt6。

```bash
# 方式一：使用 CMake预设（需要安装 Ninja）
cmake --preset debug
cmake --build --preset debug

# 方式二：标准 CMake 构建（自动选择系统生成器）
cmake -B build
cmake --build build
```

也可直接用 Qt Creator / CLion 打开工程根目录。

### 依赖

- Qt 5.15+ 或 Qt 6.x（Widgets, Network, Sql 模块）
- CMake 3.28+
- C++17 编译器（MSVC / MinGW / GCC / Clang）
- Ninja（可选，仅 `cmake --preset debug` 时需要）

---

## 运行

### 1. 启动服务端

```bash
./build-verify/server/gobang_server
```

默认监听端口 `7777`，启动日志：
```
Gobang server listening on port 7777
```

### 2. 启动客户端

```bash
./build-verify/client/gobang_client
```

登录框默认连接 `127.0.0.1:7777`，输入用户名/密码即可注册或登录。

---

## 数据库

服务端自动在可执行文件所在目录创建 SQLite 数据库文件 `gobang_accounts.db`。

表结构：

```sql
CREATE TABLE IF NOT EXISTS accounts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
);
```

账号规则：

- 用户名不能为空，最长 32 个字符
- 密码不能为空，至少 6 位
- 密码以 **SHA-256** 哈希后存入数据库

---

## 网络协议

基于 TCP，采用 **JSON 行协议**（每行一个 JSON 对象，`\n` 分隔）。

消息统一格式：

```json
{"type": "消息类型", ...其他字段}
```

### 消息类型一览

| 类型 | 方向 | 说明 |
|------|------|------|
| `login` / `login_reply` | C ↔ S | 登录请求/响应 |
| `register` / `register_reply` | C ↔ S | 注册请求/响应 |
| `online_list` | S → C | 在线用户列表广播 |
| `invite` | S → C | 收到对战邀请 |
| `invite_reply` / `invite_result` | C ↔ S | 邀请回应/结果通知 |
| `game_start` | S → C | 对局开始（含房间 ID、黑白分配） |
| `move` / `move_result` | C ↔ S | 落子请求/响应 |
| `opponent_move` | S → C | 转发对手落子 |
| `game_over` | C ↔ S | 对局结束 |
| `error` | S → C | 错误消息 |

---

## AI 对战

客户端支持两种 AI 模式：

### 本地 AI（默认）

本地策略 AI，基于评分搜索，有三个难度等级：

- **简单（Easy）**：优先中心落子，无防守意识
- **普通（Normal）**：兼顾进攻（AI 棋子）和防守（人类棋子），防守权重为进攻的 2/3
- **困难（Hard）**：在普通基础上加入中心距离奖励，更倾向于占据棋盘中央

### HTTP AI（可选）

如果不填写 AI 接口地址则使用本地 AI。填写后将对指定 URL 发起 POST 请求：

**请求格式**（JSON）：
```json
{
    "board": [[0,0,0,...], ...],
    "difficulty": 0
}
```

**响应格式**（JSON）：
```json
{"row": 7, "col": 7}
```

---

## 类关系图

```
                    ┌──────────────┐
                    │ NetworkClient │──→ 信号驱动 UI
                    └──────┬───────┘
                           │ TCP
                    ┌──────▼───────┐
                    │  GobangServer │←→ AccountStore (SQLite)
                    │  (Room 管理)  │
                    └──────────────┘

Client 端:
┌──────────────┐   ┌──────────────────┐   ┌──────────────────┐
│ LoginDialog  │   │   MainWindow     │   │ GameBoardWidget  │
│ (登录/注册)   │   │ (布局 + 切换模式) │   │ (棋盘自绘 + 交互) │
└──────────────┘   └────────┬─────────┘   └────────┬─────────┘
                            │ 构造                    │ 信号
                            ▼                        ▼
                     ┌───────────────────────────────────┐
                     │         GameController            │
                     │  (好友对战 / 人机对战 逻辑中枢)    │
                     └───────────────┬───────────────────┘
                                     │
                           ┌─────────▼─────────┐
                           │     AIEngine      │
                           │ (HTTP + 本地搜索)  │
                           └───────────────────┘
```
