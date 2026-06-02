# Nebula Runner

Nebula Runner 是一个使用 C++20 和 FTXUI 开发的终端实时小游戏。玩家驾驶侦察机在星云矿区中收集能量晶体，同时躲避巡航无人机。游戏包含分数、连击、等级、生命值、动态难度和终端彩色界面。

## 项目特点

- 使用 FTXUI 构建字符终端图形界面
- 实时刷新和键盘交互
- 随等级提升逐渐增加敌人移动速度和数量
- 连击系统：连续收集晶体可获得更高分数
- 完整 CMake 项目结构，适合提交到个人 GitHub 仓库

## 运行方式

确保本机已安装 CMake 和支持 C++20 的编译器。

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\nebula_runner.exe
```

Linux 或 macOS：

```bash
cmake -S . -B build
cmake --build build
./build/nebula_runner
```

首次配置时，CMake 会通过 FetchContent 自动下载 FTXUI。

## 操作说明

- `WASD` 或方向键：移动飞船
- `P`：暂停或继续
- `R`：重新开始
- `Q` 或 `Esc`：退出游戏

## 游戏规则

收集蓝色晶体获得分数。短时间内连续收集会提高连击倍率。碰到红色无人机会损失生命并打断连击。分数越高等级越高，无人机数量和移动频率也会提高。

## 项目结构

```text
ftxui_nebula_runner/
├── CMakeLists.txt
├── README.md
└── src/
    ├── game.cpp
    ├── game.h
    └── main.cpp
```

## 贡献指南

欢迎继续扩展以下内容：

- 添加不同类型的敌人
- 添加本地最高分保存
- 添加道具和护盾机制
- 优化地图生成策略

