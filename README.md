# GB28181 设备模拟器

> 基于 EasyGBD SDK 的 GB28181 设备批量模拟工具，支持 WPF 图形界面与 C++ 命令行两种运行方式。

---

## 📋 项目简介

本项目提供两个子项目，用于模拟大量 GB28181 标准摄像头设备向视频管理平台（如 WVP-Pro、EasyCVR 等）注册并推送视频流：

| 子项目 | 说明 |
|--------|------|
| **GB28181SimulatorWpf** | WPF 图形界面版，支持实时日志、设备状态监控（.NET 8 / C#） |
| **GB28181DeviceSimulator** | C++ 命令行版，通过 XML 文件配置，适合批量/自动化场景 |

---

## 🏗️ 架构概览

```
GB28181DeviceSimulator/
├── GB28181SimulatorWpf/          # WPF GUI 项目 (.NET 8, C#)
│   ├── Models/
│   │   └── DeviceModels.cs       # AppConfig、DeviceStatus、LogEntry 数据模型
│   ├── Services/
│   │   ├── SimulatorService.cs   # 设备生命周期管理（注册、心跳、推流调度）
│   │   └── StreamPusher.cs       # StreamHub：单连接多通道共享拉流
│   ├── NativeApi/                # P/Invoke 封装（libEasyGBD、libEasyStreamClient）
│   ├── Converters/               # WPF 值转换器
│   ├── MainWindow.xaml           # 主界面
│   └── EasyDarwin.mp4            # 默认测试媒体文件
│
├── GB28181DeviceSimulator/       # C++ 命令行项目
│   ├── main.cpp                  # 单设备入口
│   ├── main_multiDevice.cpp      # 多设备并发示例
│   ├── GB28181Device.xml         # 设备配置文件
│   ├── xmlConfig*.cpp/h          # XML 配置解析（tinyxml）
│   ├── libEasyGBD.dll            # EasyGBD GB28181 核心库
│   ├── libEasyStreamClient.dll   # 拉流客户端库
│   └── ffmpeg DLLs (avcodec-58 等)  # 音视频编解码
│
└── GB28181DeviceSimulator.sln    # Visual Studio 解决方案
```

---

## ⚙️ 环境要求

### WPF 版本（GB28181SimulatorWpf）

- **操作系统**：Windows 10 / 11 x64
- **运行时**：[.NET 8.0 Desktop Runtime](https://dotnet.microsoft.com/download/dotnet/8.0)
- **平台**：仅支持 **x64**

### C++ 版本（GB28181DeviceSimulator）

- **操作系统**：Windows x64
- **运行时**：Visual C++ 2015-2022 Redistributable (x64)
- **编译环境**：Visual Studio 2019 或更高版本（C++17）

---

## 🚀 快速开始

### WPF 图形界面版

1. **打开解决方案**

   用 Visual Studio 2022 打开 `GB28181DeviceSimulator.sln`

2. **设置启动项目**

   右键 `GB28181SimulatorWpf` → 设为启动项目

3. **编译运行**

   选择 `x64 | Debug / Release` 配置，按 <kbd>F5</kbd> 启动

4. **配置并启动模拟**

   在界面中填写以下参数，然后点击「启动」：

   | 参数 | 说明 | 默认值 |
   |------|------|--------|
   | 服务器 SIP ID | 平台的 SIP ID | `34020000002000000001` |
   | 服务器域 ID | SIP 域 | `3402000000` |
   | 服务器 IP | 平台 SIP 地址 | `192.168.0.88` |
   | 服务器端口 | 平台 SIP 端口 | `15060` |
   | 接入密码 | SIP Digest 认证密码 | `87654321` |
   | 设备 SIP ID | 模拟设备基础 ID（末 5 位自动递增） | `11010000001320000001` |
   | 设备数量 | 同时模拟的设备数 | `5` |
   | 每设备通道数 | 每台设备的通道数 | `2` |
   | 本地 SIP 端口 | 设备监听起始端口（每台设备自动 +1） | `5064` |
   | 注册周期 | 注册有效期（秒） | `3600` |
   | 心跳周期 | 心跳间隔（秒） | `60` |
   | 最大心跳次数 | 心跳超时次数上限 | `3` |
   | SIP 协议 | `TCP` 或 `UDP` | `UDP` |
   | 媒体源 | RTSP URL 或本地 MP4/H264 文件路径 | `EasyDarwin.mp4` |

### C++ 命令行版

1. 编辑 `GB28181DeviceSimulator/GB28181Device.xml`：

   ```xml
   <?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
   <XMLConfig>
       <ServerSIPID>34020000002000000001</ServerSIPID>  <!--服务器SIP ID-->
       <ServerDomainID>3402000000</ServerDomainID>      <!--服务器域-->
       <ServerIP>192.168.0.88</ServerIP>                <!--服务器IP地址-->
       <ServerPort>15060</ServerPort>                   <!--服务器SIP端口-->
       <AccessPwd>87654321</AccessPwd>                  <!--接入密码-->
       <DeviceSIPID>11010000001320000001</DeviceSIPID>  <!--基础设备ID-->
       <RegisterPeriod>3600</RegisterPeriod>            <!--注册周期(秒)-->
       <HeartbeatPeriod>60</HeartbeatPeriod>            <!--心跳周期(秒)-->
       <MaxHeartbeatCount>3</MaxHeartbeatCount>         <!--最大心跳次数-->
       <SipProtocol>UDP</SipProtocol>                  <!--TCP 或 UDP-->
       <DeviceNum>20</DeviceNum>                       <!--模拟设备数量-->
       <FilePath>EasyDarwin.mp4</FilePath>             <!--媒体文件路径-->
   </XMLConfig>
   ```

2. 用 Visual Studio 编译 `GB28181DeviceSimulator` 项目，运行生成的 `.exe`

---

## 🔑 核心功能说明

### 设备 ID 自动递增

设备 SIP ID 基于 `DeviceSipID` 末 5 位数字自动递增：

```
基础 ID: 11010000001320000001
设备 0:  11010000001320000001
设备 1:  11010000001320000002
设备 2:  11010000001320000003
...
```

### 通道 ID 生成规则

通道 ID 在设备 ID 基础上修改第 10-12 位为 `131`（IPC 类型），末 4 位为通道序号（1 起始）：

```
设备 ID:   11010000001320000001
通道 1:    11010000001310000001  ← 位[10-12]=131, 末4位=0001
通道 2:    11010000001310000002
```

### StreamHub 共享拉流

多通道共享同一 RTSP 连接，避免 RTSP 源拒绝重复连接或资源浪费：

```
RTSP URL (单连接)
    └── StreamHub
            ├── 推流器 A（设备1 / 通道1）
            ├── 推流器 B（设备1 / 通道2）
            └── 推流器 C（设备2 / 通道1）
```

每路推流独立管理媒体信息设置（`SetMediaInfo`）及帧推送（`PushFrame`）。

---

## 📦 依赖 DLL 说明

| DLL | 用途 |
|-----|------|
| `libEasyGBD.dll` | EasyGBD GB28181 设备端核心（注册 / 心跳 / 推流） |
| `libEasyStreamClient.dll` | EasyStreamClient 拉流客户端 |
| `libEasyAACEncoder.dll` | AAC 音频编码 |
| `libeasyrtmp.dll` | RTMP 推流支持 |
| `Logger.dll` | 日志组件 |
| `avcodec-58.dll` 等 | FFmpeg 4.x 音视频编解码 |
| `concrt140.dll` | Visual C++ 并发运行时 |

> ⚠️ 所有 DLL 均为 **x64** 版本，不支持 32 位环境。

---

## 🛠️ 配置持久化（WPF 版）

WPF 版本自动将界面配置保存至程序目录下的 `gb28181_sim_config.json`，下次启动时自动加载，无需重复填写。

---

## ❓ 常见问题

**Q: 注册超时怎么办？**  
A: 检查服务器 IP / 端口是否正确，防火墙是否放行对应 SIP 端口（UDP/TCP），以及 SIP 协议选择是否与服务器一致。

**Q: 认证失败 (Auth Fail) 怎么办？**  
A: 确认 `AccessPwd`（接入密码）与平台侧配置完全一致，注意区分大小写。

**Q: 只有一个通道能推流？**  
A: 本项目已通过 `StreamHub` 机制解决此问题，多通道共享同一拉流连接并分发帧数据。若仍有问题，请检查媒体源 RTSP URL 是否可正常访问。

**Q: 启动报「本地端口冲突」？**  
A: 默认本地 SIP 端口从 `5064` 开始递增，刻意避开 WVP 常用的 `5060`。如与其他程序冲突，在界面中修改「本地 SIP 端口」起始值。

**Q: 如何模拟更多设备？**  
A: 增大「设备数量」参数即可，每台设备会自动分配递增的设备 ID 和本地端口。注意系统资源（线程、文件句柄、端口）的限制。

---

## 📄 许可证

本项目使用 [EasyGBD SDK](http://www.tsingsee.com/) 进行开发，SDK 相关版权归青犀 TSINGSEE 所有。请遵循仓库中的许可证文件使用本项目代码。
