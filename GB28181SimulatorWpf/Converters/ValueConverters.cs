using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;
using GB28181SimulatorWpf.Models;

namespace GB28181SimulatorWpf.Converters
{
    /// <summary>Converts DeviceRegState → status dot fill color</summary>
    public class StateToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is DeviceRegState state)
            {
                return state switch
                {
                    DeviceRegState.Online       => new SolidColorBrush(Color.FromRgb(16,  185, 129)), // green
                    DeviceRegState.Connecting   => new SolidColorBrush(Color.FromRgb(251, 191,  36)), // amber
                    DeviceRegState.Registering  => new SolidColorBrush(Color.FromRgb(56,  189, 248)), // sky
                    DeviceRegState.AuthFail     => new SolidColorBrush(Color.FromRgb(239,  68,  68)), // red
                    DeviceRegState.Timeout      => new SolidColorBrush(Color.FromRgb(249, 115,  22)), // orange
                    DeviceRegState.Disconnected => new SolidColorBrush(Color.FromRgb(239,  68,  68)), // red
                    _                           => new SolidColorBrush(Color.FromRgb(100, 116, 139)), // slate
                };
            }
            return Brushes.Gray;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            => throw new NotImplementedException();
    }

    /// <summary>Converts log Level string → foreground color</summary>
    public class LogLevelToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (value as string) switch
            {
                "ERROR" => new SolidColorBrush(Color.FromRgb(248,  113, 113)), // red-400
                "WARN"  => new SolidColorBrush(Color.FromRgb(251,  191,  36)), // amber
                "OK"    => new SolidColorBrush(Color.FromRgb( 52,  211, 153)), // emerald
                "INFO"  => new SolidColorBrush(Color.FromRgb(148,  163, 184)), // slate-400
                _       => new SolidColorBrush(Color.FromRgb(148,  163, 184)),
            };
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            => throw new NotImplementedException();
    }

    /// <summary>Converts bool → Visibility</summary>
    public class BoolToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
            => value is true ? Visibility.Visible : Visibility.Collapsed;

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            => (value is Visibility v) && v == Visibility.Visible;
    }
}
