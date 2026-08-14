# DeepFurryPet — 可交互式 Furry 桌宠

跨平台 Furry 桌宠（Qt6 C++），运行于 **Windows / Ubuntu / 安卓平板**。
内置 DeepSeek v4-flash 对话，会记得你的一切。

## 功能

| 操作 | 效果 |
|------|------|
| 空闲 | 显示 `1.gif` |
| 单击 | 播放 `2.gif` 1.2s → 回 `1.gif` |
| 双击 / 安卓长按 | 播放 `3.gif` 7.84s → 回 `1.gif` |
| 左键按住下滑 / 安卓下划 | 呼出 DeepFurry 对话 |
| 对话内上滑 / 安卓下划 | 关闭对话 |
| 夜间 19:00-06:00 | 固定 `4.gif`（交互仍可用） |
| 右键双击 | 退出程序 |

## 对话

- 调用 DeepSeek API（`deepseek-v4-flash`）
- 首次运行输入 API key（本地保存）
- 隐藏提示词：`你是一只可爱的DeepFurry`
- 对话记忆永久保存（本地 `chat_history.json`）

## 构建

### Linux / Ubuntu

```bash
sudo apt install qt6-base-dev qt6-multimedia-dev cmake g++
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/DeepFurryPet
```

### Windows（Linux 交叉编译）

```bash
# 1. 安装 mingw + aqt
sudo apt install mingw-w64
pip install aqtinstall

# 2. 下载 Qt Windows 版
aqt install-qt windows desktop 6.4.2 win64_mingw -m qtmultimedia

# 3. 手动编译 (moc 用 Linux 工具, 参考 build 脚本)
```

### 安卓平板

需要 Qt for Android 工具链 + 触摸事件适配（代码已预留手势逻辑）。

## 素材

`resources/assets/`：
- `1.gif` 空闲 / `2.gif` 单击 / `3.gif` 双击 / `4.gif` 夜间
- `6.png` 应用图标

## 下载

- Windows exe: 见 Release 或联系作者

## License

GPL-3.0 © Cloud LTE Studio
