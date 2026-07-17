using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using GB28181SimulatorWpf.Models;
using GB28181SimulatorWpf.NativeApi;

namespace GB28181SimulatorWpf.Services
{
    /// <summary>
    /// Manages the lifecycle of multiple simulated GB28181 devices.
    /// Thread-safe: all callbacks marshal to the ConcurrentQueue and are
    /// consumed on the UI thread via the LogQueue / EventQueue.
    /// </summary>
    public class SimulatorService : IDisposable
    {
        // --------------------------------------------------------
        //  Public state
        // --------------------------------------------------------

        public ObservableCollection<DeviceStatus> Devices { get; } = new();

        /// <summary>Incoming log entries to be consumed by UI</summary>
        public ConcurrentQueue<LogEntry> LogQueue { get; } = new();

        public bool IsRunning { get; private set; }

        // --------------------------------------------------------
        //  Private fields
        // --------------------------------------------------------

        private readonly List<IntPtr> _handles = new();
        private readonly List<GCHandle> _gcHandles = new();   // keep callbacks alive

        // One callback delegate per device (GC protection)
        private readonly List<GB28181ClientCallback> _callbacks = new();

        private AppConfig? _config;

        // --------------------------------------------------------
        //  Start / Stop
        // --------------------------------------------------------

        public void Start(AppConfig config)
        {
            if (IsRunning) return;
            _config = config;
            IsRunning = true;

            Devices.Clear();
            _handles.Clear();
            _callbacks.Clear();
            foreach (var gch in _gcHandles) gch.Free();
            _gcHandles.Clear();

            int totalDevices = config.DeviceNum;
            if (totalDevices <= 0) return;

            // Parse base DeviceID prefix (strip last 5 digits)
            string baseID = config.DeviceSipID;
            int len = baseID.Length;
            if (len < 20) len = 20;
            string idPrefix = baseID.Length >= 5 ? baseID[..^5] : baseID;
            int startIndex = 0;
            if (baseID.Length >= 5 && int.TryParse(baseID[^5..], out int parsed))
                startIndex = parsed;

            for (int i = 0; i < totalDevices; i++)
            {
                string deviceID = $"{idPrefix}{startIndex + i:D5}";

                var status = new DeviceStatus
                {
                    DeviceID     = deviceID,
                    ServerID     = config.ServerSipID,
                    State        = DeviceRegState.Idle,
                    ChannelCount = config.ChannelsPerDevice,
                };
                Devices.Add(status);

                // Build access info struct
                // NOTE: deviceWanAddr MUST be set to empty string (not left null/garbage)
                var accessInfo = new GB28181ClientAccessInfo
                {
                    enable            = 1,
                    name              = $"EasyGBD-{i + 1:D5}",
                    serverID          = config.ServerSipID,
                    domainID          = config.ServerDomainID,
                    serverSipAddr     = config.ServerSipIP,
                    serverSipPort     = config.ServerSipPort,
                    deviceID          = deviceID,
                    deviceSipPort     = config.DeviceLocalPort + i,  // 本地监听端口，从 DeviceLocalPort 递增（默认 5064，避免与 WVP 的 5060 冲突）
                    accessPwd         = config.AccessPwd,   // SIP Digest password
                    deviceWanAddr     = "",                 // must not be null
                    deviceWanPort     = 0,
                    registerPeriod    = config.RegisterPeriod,
                    heartbeatPeriod   = config.HeartbeatPeriod,
                    maxHeartbeatCount = config.MaxHeartbeatCount,
                    catalogPacketSize = 0,
                    sipProtocol       = config.SipProtocol,
                    charset           = "GB2312",
                };

                // Log the password (masked) for debugging
                string pwdMasked = config.AccessPwd.Length > 2
                    ? config.AccessPwd[..2] + new string('*', config.AccessPwd.Length - 2)
                    : "***";
                AppendLog("INFO", deviceID, $"SIP 注册密码: {pwdMasked}  | 协议: {(config.SipProtocol == 1 ? "TCP" : "UDP")}  | 本地端口: {config.DeviceLocalPort + i}  | 服务器: {config.ServerSipIP}:{config.ServerSipPort}");

                // Create native handle
                int deviceIndex = i; // capture for closure
                GB28181ClientCallback cb = MakeCallback(deviceIndex);
                _callbacks.Add(cb);

                // Pin the callback so GC won't collect it
                var gch = GCHandle.Alloc(cb);
                _gcHandles.Add(gch);

                int ret = EasyGBDNative.Create(out IntPtr handle, IntPtr.Zero, cb, IntPtr.Zero);
                if (ret != 0 || handle == IntPtr.Zero)
                {
                    AppendLog("ERROR", deviceID, $"libGB28181Client_Create 失败, 返回值={ret}");
                    Devices[i].State = DeviceRegState.Disconnected;
                    _handles.Add(IntPtr.Zero);
                    continue;
                }

                _handles.Add(handle);
                status.NativeHandle = handle.ToInt64();

                // Add access node
                EasyGBDNative.AddAccessNode(handle, ref accessInfo);

                // Add channels
                for (int j = 0; j < config.ChannelsPerDevice; j++)
                {
                    string channelID = BuildChannelID(deviceID, j);

                    var chInfo = new GB28181ChannelInfo
                    {
                        channelID   = channelID,
                        name        = $"CH{j + 1:D2}",
                        manufacturer= "TSINGSEE",
                        model       = "EasyGBD",
                        owner       = "owner",
                        civilCode   = "civilcode",
                        address     = "",
                        parentID    = deviceID,
                        registerWay = 1,
                        status      = 1,
                    };

                    EasyGBDNative.AddChannel(handle, config.ServerSipID, ref chInfo, false);
                }

                // Start registration
                EasyGBDNative.Start(handle);
                Devices[i].State = DeviceRegState.Connecting;
                AppendLog("INFO", deviceID, $"设备已启动, 正在连接 {config.ServerSipIP}:{config.ServerSipPort}");
            }
        }

        public void Stop()
        {
            if (!IsRunning) return;
            IsRunning = false;

            for (int i = 0; i < _handles.Count; i++)
            {
                if (_handles[i] == IntPtr.Zero) continue;
                EasyGBDNative.Stop(_handles[i]);

                string devID = i < Devices.Count ? Devices[i].DeviceID : $"Device[{i}]";
                AppendLog("INFO", devID, "设备已停止");

                if (i < Devices.Count)
                    Devices[i].State = DeviceRegState.Idle;
            }

            // Cleanup handles after a brief delay to let DLL flush
            Task.Delay(500).ContinueWith(_ =>
            {
                for (int i = 0; i < _handles.Count; i++)
                {
                    if (_handles[i] != IntPtr.Zero)
                    {
                        IntPtr h = _handles[i];
                        EasyGBDNative.Release(ref h);
                    }
                }
                _handles.Clear();
                foreach (var gch in _gcHandles) gch.Free();
                _gcHandles.Clear();
                _callbacks.Clear();
            });
        }

        // --------------------------------------------------------
        //  Callback factory
        // --------------------------------------------------------

        private GB28181ClientCallback MakeCallback(int deviceIndex)
        {
            return (userptr, type, serverID, channelID, data, size, startTime, endTime, ext) =>
            {
                if (deviceIndex >= Devices.Count) return 0;
                var status = Devices[deviceIndex];

                switch (type)
                {
                    case GB28181CallbackType.Connecting:
                        status.State = DeviceRegState.Connecting;
                        AppendLog("INFO", status.DeviceID, $"正在连接服务器 {serverID}...");
                        break;

                    case GB28181CallbackType.RegisterIng:
                        status.State = DeviceRegState.Registering;
                        string regMsg = size > 0 ? "注册中 (已收到 401 挑战)..." : "发送注册请求...";
                        AppendLog("INFO", status.DeviceID, regMsg);
                        break;

                    case GB28181CallbackType.RegisterTimeout:
                        status.State = DeviceRegState.Timeout;
                        AppendLog("WARN", status.DeviceID, $"注册超时 [{serverID}]");
                        break;

                    case GB28181CallbackType.RegisterOk:
                        status.State = DeviceRegState.Online;
                        AppendLog("OK", status.DeviceID, $"注册成功 ✓ [{serverID}]");
                        break;

                    case GB28181CallbackType.RegisterAuthFail:
                        status.State = DeviceRegState.AuthFail;
                        AppendLog("ERROR", status.DeviceID, $"认证失败 [{serverID}] — 请检查密码");
                        break;

                    case GB28181CallbackType.Disconnect:
                        status.State = DeviceRegState.Disconnected;
                        AppendLog("WARN", status.DeviceID, $"已断开 [{serverID}]");
                        break;

                    case GB28181CallbackType.StartAudioVideo:
                        status.StreamingChannels++;
                        AppendLog("INFO", status.DeviceID, $"开始推流 ▶ 通道={channelID}");
                        break;

                    case GB28181CallbackType.StopAudioVideo:
                        if (status.StreamingChannels > 0) status.StreamingChannels--;
                        AppendLog("INFO", status.DeviceID, $"停止推流 ■ 通道={channelID}");
                        break;

                    default:
                        AppendLog("INFO", status.DeviceID, $"事件={type}, 服务器={serverID}, 通道={channelID}");
                        break;
                }

                return 0;
            };
        }

        // --------------------------------------------------------
        //  Helpers
        // --------------------------------------------------------

        private static string BuildChannelID(string deviceID, int channelIndex)
        {
            // Position [10-12] = "131" (IPC type), last 4 digits = channel number
            char[] id = deviceID.ToCharArray();
            if (id.Length >= 20)
            {
                id[10] = '1'; id[11] = '3'; id[12] = '1';
                string suffix = $"{channelIndex + 1:D4}";
                for (int k = 0; k < 4 && 16 + k < id.Length; k++)
                    id[16 + k] = suffix[k];
            }
            return new string(id);
        }

        private void AppendLog(string level, string deviceID, string message)
        {
            LogQueue.Enqueue(new LogEntry
            {
                Timestamp = DateTime.Now.ToString("HH:mm:ss.fff"),
                DeviceID  = deviceID,
                Level     = level,
                Message   = message,
            });
        }

        // --------------------------------------------------------
        //  Config persistence
        // --------------------------------------------------------

        private static readonly string ConfigPath =
            Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "gb28181_sim_config.json");

        public static AppConfig LoadConfig()
        {
            try
            {
                if (File.Exists(ConfigPath))
                {
                    var json = File.ReadAllText(ConfigPath);
                    return JsonSerializer.Deserialize<AppConfig>(json) ?? new AppConfig();
                }
            }
            catch { /* ignore, use defaults */ }
            return new AppConfig();
        }

        public static void SaveConfig(AppConfig config)
        {
            try
            {
                var opts = new JsonSerializerOptions { WriteIndented = true };
                File.WriteAllText(ConfigPath, JsonSerializer.Serialize(config, opts));
            }
            catch { /* ignore */ }
        }

        // --------------------------------------------------------
        //  IDisposable
        // --------------------------------------------------------

        public void Dispose()
        {
            if (IsRunning) Stop();
        }
    }
}
