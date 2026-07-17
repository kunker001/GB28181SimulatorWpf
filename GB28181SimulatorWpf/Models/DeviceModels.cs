using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Text.Json.Serialization;

namespace GB28181SimulatorWpf.Models
{
    // ============================================================
    //  Persistent configuration (saved to JSON)
    // ============================================================

    public class AppConfig
    {
        // --- Server ---
        public string ServerSipID      { get; set; } = "34020000002000000001";
        public string ServerDomainID   { get; set; } = "3402000000";
        public string ServerSipIP      { get; set; } = "192.168.0.88";
        public int    ServerSipPort    { get; set; } = 15060;

        // --- Device template ---
        public string DeviceSipID      { get; set; } = "11010000001320000001";
        public string AccessPwd        { get; set; } = "87654321";
        public int    DeviceNum        { get; set; } = 5;
        public int    ChannelsPerDevice{ get; set; } = 2;
        public int    DeviceLocalPort  { get; set; } = 5064;   // 本地 SIP 监听起始端口，避免与 WVP 的 5060 冲突

        // --- Timing ---
        public int    RegisterPeriod   { get; set; } = 3600;
        public int    HeartbeatPeriod  { get; set; } = 60;
        public int    MaxHeartbeatCount{ get; set; } = 3;

        /// <summary>1=TCP, 2=UDP</summary>
        public int    SipProtocol      { get; set; } = 2;

        // --- Media ---
        /// <summary>RTSP URL or local file path (mp4/h264)</summary>
        public string MediaSource      { get; set; } = "EasyDarwin.mp4";
    }

    // ============================================================
    //  Runtime device status (UI-bound, INotifyPropertyChanged)
    // ============================================================

    public enum DeviceRegState
    {
        Idle,
        Connecting,
        Registering,
        Online,
        AuthFail,
        Timeout,
        Disconnected,
    }

    public class DeviceStatus : INotifyPropertyChanged
    {
        private string          _deviceID  = string.Empty;
        private string          _serverID  = string.Empty;
        private DeviceRegState  _state     = DeviceRegState.Idle;
        private int             _channelCount;
        private int             _streamingChannels;

        public string DeviceID
        {
            get => _deviceID;
            set { _deviceID = value; OnPropertyChanged(); }
        }

        public string ServerID
        {
            get => _serverID;
            set { _serverID = value; OnPropertyChanged(); }
        }

        public DeviceRegState State
        {
            get => _state;
            set { _state = value; OnPropertyChanged(); OnPropertyChanged(nameof(StateLabel)); OnPropertyChanged(nameof(IsOnline)); }
        }

        public string StateLabel => State switch
        {
            DeviceRegState.Idle         => "空闲",
            DeviceRegState.Connecting   => "连接中...",
            DeviceRegState.Registering  => "注册中...",
            DeviceRegState.Online       => "已注册 ✓",
            DeviceRegState.AuthFail     => "认证失败",
            DeviceRegState.Timeout      => "注册超时",
            DeviceRegState.Disconnected => "已断开",
            _                           => "-"
        };

        public bool IsOnline => State == DeviceRegState.Online;

        public int ChannelCount
        {
            get => _channelCount;
            set { _channelCount = value; OnPropertyChanged(); }
        }

        public int StreamingChannels
        {
            get => _streamingChannels;
            set { _streamingChannels = value; OnPropertyChanged(); }
        }

        // Pointer stored as long for display purposes
        public long NativeHandle { get; set; }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }

    // ============================================================
    //  Log entry
    // ============================================================

    public class LogEntry
    {
        public string Timestamp { get; set; } = string.Empty;
        public string DeviceID  { get; set; } = string.Empty;
        public string Level     { get; set; } = "INFO";
        public string Message   { get; set; } = string.Empty;
    }
}
