using System;
using System.Runtime.InteropServices;

namespace GB28181SimulatorWpf.NativeApi
{
    // ============================================================
    //  Enumerations
    // ============================================================

    public enum GB28181CallbackType
    {
        Connecting           = 1,
        RegisterIng          = 2,
        RegisterTimeout      = 3,
        RegisterOk           = 4,
        RegisterAuthFail     = 5,
        StartAudioVideo      = 6,
        StopAudioVideo       = 7,
        TalkAudioData        = 8,
        StartPlayback        = 9,
        StopPlayback         = 10,
        StartDownload        = 11,
        StopDownload         = 12,
        Disconnect           = 13,
        SubscribeAlarm       = 14,
        SubscribeCatalog     = 15,
        SubscribeMobilePosition = 16,
    }

    // ============================================================
    //  Structs (mirror of C headers, pack=1 not needed for x64)
    // ============================================================

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct GB28181ClientAccessInfo
    {
        public int     enable;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string  serverID;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string  domainID;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string  name;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string  serverSipAddr;

        public int     serverSipPort;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string  deviceID;

        public int     deviceSipPort;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string  accessPwd;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string  deviceWanAddr;

        public int     deviceWanPort;
        public int     registerPeriod;
        public int     heartbeatPeriod;
        public int     maxHeartbeatCount;
        public int     catalogPacketSize;

        /// <summary>1=TCP, 2=UDP</summary>
        public int     sipProtocol;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string  charset;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct GB28181ChannelInfo
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string channelID;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string name;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string manufacturer;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string model;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string owner;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string civilCode;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string address;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string parentID;

        public int    registerWay;

        /// <summary>1 = Online</summary>
        public int    status;
    }

    // ============================================================
    //  Callback delegate
    // ============================================================

    /// <summary>
    /// Mirrors: int (*GB28181Client_Callback)(void* userptr, type, serverID, channelID,
    ///          char* data, int size, startTime, endTime, void* ext)
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int GB28181ClientCallback(
        IntPtr userptr,
        GB28181CallbackType type,
        [MarshalAs(UnmanagedType.LPStr)] string serverID,
        [MarshalAs(UnmanagedType.LPStr)] string channelID,
        IntPtr data,
        int size,
        [MarshalAs(UnmanagedType.LPStr)] string startTime,
        [MarshalAs(UnmanagedType.LPStr)] string endTime,
        IntPtr ext);

    // ============================================================
    //  P/Invoke declarations for libEasyGBD.dll
    // ============================================================

    public static class EasyGBDNative
    {
        private const string DllName = "libEasyGBD";

        /// <summary>Create a GB28181 client instance</summary>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "libGB28181Client_Create")]
        public static extern int Create(
            out IntPtr handle,
            IntPtr loggerHandle,
            GB28181ClientCallback callback,
            IntPtr userptr);

        /// <summary>Release a GB28181 client instance</summary>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "libGB28181Client_Release")]
        public static extern int Release(ref IntPtr handle);

        /// <summary>Add a SIP server access node</summary>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "libGB28181Client_AddAccessNode")]
        public static extern int AddAccessNode(
            IntPtr handle,
            ref GB28181ClientAccessInfo serverInfo);

        /// <summary>Add a channel to the device</summary>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "libGB28181Client_AddChannel")]
        public static extern int AddChannel(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string serverID,
            ref GB28181ChannelInfo channelInfo,
            [MarshalAs(UnmanagedType.Bool)] bool update);

        /// <summary>Delete a channel from the device</summary>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "libGB28181Client_DeleteChannel")]
        public static extern int DeleteChannel(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string serverID,
            [MarshalAs(UnmanagedType.LPStr)] string channelID,
            [MarshalAs(UnmanagedType.Bool)] bool update);

        /// <summary>Start the device (begin registration)</summary>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "libGB28181Client_Start")]
        public static extern int Start(IntPtr handle);

        /// <summary>Stop the device</summary>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "libGB28181Client_Stop")]
        public static extern int Stop(IntPtr handle);
    }
}
