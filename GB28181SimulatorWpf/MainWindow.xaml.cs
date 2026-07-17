using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using System.Windows.Media;
using System.Windows.Threading;
using GB28181SimulatorWpf.Models;
using GB28181SimulatorWpf.Services;

namespace GB28181SimulatorWpf
{
    public partial class MainWindow : Window
    {
        // --------------------------------------------------------
        //  Fields
        // --------------------------------------------------------

        private readonly SimulatorService _service = new();
        private readonly ObservableCollection<LogEntry> _allLogs    = new();
        private readonly ObservableCollection<LogEntry> _shownLogs  = new();
        private readonly DispatcherTimer _uiTimer = new();

        private string _logFilter = string.Empty;

        // --------------------------------------------------------
        //  Constructor
        // --------------------------------------------------------

        public MainWindow()
        {
            InitializeComponent();

            // Bind grids
            GridDevices.ItemsSource = _service.Devices;
            GridLogs.ItemsSource    = _shownLogs;

            // Load saved config
            var cfg = SimulatorService.LoadConfig();
            BindConfigToUi(cfg);

            // UI refresh timer
            _uiTimer.Interval = TimeSpan.FromMilliseconds(120);
            _uiTimer.Tick += UiTimer_Tick;
            _uiTimer.Start();
        }

        // --------------------------------------------------------
        //  UI Timer — drains log queue & updates header counters
        // --------------------------------------------------------

        private void UiTimer_Tick(object? sender, EventArgs e)
        {
            // Drain the log queue
            bool hasNew = false;
            while (_service.LogQueue.TryDequeue(out var entry))
            {
                _allLogs.Add(entry);
                if (PassesFilter(entry))
                    _shownLogs.Add(entry);
                hasNew = true;
            }

            // Auto-scroll log grid unless paused
            if (hasNew && ChkPauseLog.IsChecked != true && _shownLogs.Count > 0)
            {
                GridLogs.ScrollIntoView(_shownLogs[^1]);
            }

            // Keep log list bounded (max 2000 rows)
            while (_allLogs.Count > 2000)  _allLogs.RemoveAt(0);
            while (_shownLogs.Count > 2000) _shownLogs.RemoveAt(0);

            // Update header counters
            int online    = _service.Devices.Count(d => d.State == DeviceRegState.Online);
            int streaming = _service.Devices.Sum(d => d.StreamingChannels);
            TxtTotalDevices.Text    = _service.Devices.Count.ToString();
            TxtOnlineCount.Text     = online.ToString();
            TxtStreamingCount.Text  = streaming.ToString();
        }

        // --------------------------------------------------------
        //  Button handlers
        // --------------------------------------------------------

        private void BtnStart_Click(object sender, RoutedEventArgs e)
        {
            var cfg = BuildConfigFromUi();
            if (cfg == null) return;

            _allLogs.Clear();
            _shownLogs.Clear();

            _service.Start(cfg);

            BtnStart.IsEnabled = false;
            BtnStop.IsEnabled  = true;
            SetRunStatus(true);
        }

        private void BtnStop_Click(object sender, RoutedEventArgs e)
        {
            _service.Stop();
            BtnStart.IsEnabled = true;
            BtnStop.IsEnabled  = false;
            SetRunStatus(false);
        }

        private void BtnSaveConfig_Click(object sender, RoutedEventArgs e)
        {
            var cfg = BuildConfigFromUi();
            if (cfg == null) return;
            SimulatorService.SaveConfig(cfg);
            MessageBox.Show("配置已保存！", "提示", MessageBoxButton.OK, MessageBoxImage.Information);
        }

        private void BtnClearDevices_Click(object sender, RoutedEventArgs e)
        {
            if (_service.IsRunning)
            {
                MessageBox.Show("请先停止模拟后再清除设备列表。", "提示", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            _service.Devices.Clear();
            TxtTotalDevices.Text = "0";
            TxtOnlineCount.Text  = "0";
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            _allLogs.Clear();
            _shownLogs.Clear();
        }

        private void BtnClearFilter_Click(object sender, RoutedEventArgs e)
        {
            TxtLogFilter.Text = string.Empty;
        }

        // --------------------------------------------------------
        //  Log filter
        // --------------------------------------------------------

        private void TxtLogFilter_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            _logFilter = TxtLogFilter.Text.Trim();
            RefreshLogFilter();
        }

        private void RefreshLogFilter()
        {
            _shownLogs.Clear();
            foreach (var entry in _allLogs)
                if (PassesFilter(entry))
                    _shownLogs.Add(entry);
        }

        private bool PassesFilter(LogEntry entry)
        {
            if (string.IsNullOrEmpty(_logFilter)) return true;
            return entry.DeviceID.Contains(_logFilter, StringComparison.OrdinalIgnoreCase)
                || entry.Message.Contains(_logFilter,  StringComparison.OrdinalIgnoreCase)
                || entry.Level.Contains(_logFilter,    StringComparison.OrdinalIgnoreCase);
        }

        // --------------------------------------------------------
        //  Config bind helpers
        // --------------------------------------------------------

        private void BindConfigToUi(AppConfig cfg)
        {
            TxtServerSipID.Text       = cfg.ServerSipID;
            TxtDomainID.Text          = cfg.ServerDomainID;
            TxtServerIP.Text          = cfg.ServerSipIP;
            TxtServerPort.Text        = cfg.ServerSipPort.ToString();
            TxtAccessPwd.Text         = cfg.AccessPwd;
            TxtDeviceSipID.Text       = cfg.DeviceSipID;
            TxtDeviceNum.Text         = cfg.DeviceNum.ToString();
            TxtChannelsPerDevice.Text  = cfg.ChannelsPerDevice.ToString();
            TxtDeviceLocalPort.Text    = cfg.DeviceLocalPort.ToString();
            TxtRegPeriod.Text         = cfg.RegisterPeriod.ToString();
            TxtHbPeriod.Text          = cfg.HeartbeatPeriod.ToString();
            TxtMaxHb.Text             = cfg.MaxHeartbeatCount.ToString();
            TxtMediaSource.Text       = cfg.MediaSource;
            // SipProtocol: 1=TCP(index 0), 2=UDP(index 1)
            ComboProtocol.SelectedIndex = cfg.SipProtocol == 1 ? 0 : 1;
        }

        private AppConfig? BuildConfigFromUi()
        {
            try
            {
                return new AppConfig
                {
                    ServerSipID       = TxtServerSipID.Text.Trim(),
                    ServerDomainID    = TxtDomainID.Text.Trim(),
                    ServerSipIP       = TxtServerIP.Text.Trim(),
                    ServerSipPort     = int.Parse(TxtServerPort.Text.Trim()),
                    AccessPwd         = TxtAccessPwd.Text.Trim(),
                    DeviceSipID       = TxtDeviceSipID.Text.Trim(),
                    DeviceNum         = int.Parse(TxtDeviceNum.Text.Trim()),
                    ChannelsPerDevice  = int.Parse(TxtChannelsPerDevice.Text.Trim()),
                    DeviceLocalPort   = int.Parse(TxtDeviceLocalPort.Text.Trim()),
                    RegisterPeriod    = int.Parse(TxtRegPeriod.Text.Trim()),
                    HeartbeatPeriod   = int.Parse(TxtHbPeriod.Text.Trim()),
                    MaxHeartbeatCount = int.Parse(TxtMaxHb.Text.Trim()),
                    MediaSource       = TxtMediaSource.Text.Trim(),
                    SipProtocol       = ComboProtocol.SelectedIndex == 0 ? 1 : 2,
                };
            }
            catch (Exception ex)
            {
                MessageBox.Show($"配置参数有误：{ex.Message}", "错误",
                                MessageBoxButton.OK, MessageBoxImage.Error);
                return null;
            }
        }

        // --------------------------------------------------------
        //  UI state helpers
        // --------------------------------------------------------

        private void SetRunStatus(bool running)
        {
            if (running)
            {
                RunStatusDot.Fill  = new SolidColorBrush(Color.FromRgb(16, 185, 129));
                TxtRunStatus.Text  = "运行中";
                TxtRunStatus.Foreground = new SolidColorBrush(Color.FromRgb(16, 185, 129));
            }
            else
            {
                RunStatusDot.Fill  = new SolidColorBrush(Color.FromRgb(71, 85, 105));
                TxtRunStatus.Text  = "已停止";
                TxtRunStatus.Foreground = new SolidColorBrush(Color.FromRgb(100, 116, 139));
            }
        }

        // --------------------------------------------------------
        //  Cleanup on close
        // --------------------------------------------------------

        protected override void OnClosed(EventArgs e)
        {
            _uiTimer.Stop();
            _service.Dispose();
            base.OnClosed(e);
        }
    }
}