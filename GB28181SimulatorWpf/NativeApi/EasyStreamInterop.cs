using System.Runtime.InteropServices;

namespace GB28181SimulatorWpf.NativeApi
{
    // ============================================================
    //  EASY_FRAME_INFO  (mirrors EasyTypes.h)
    //  Total size = 64 bytes (MSVC x64 default alignment)
    // ============================================================

    [StructLayout(LayoutKind.Sequential)]
    public struct EasyFrameInfo
    {
        public uint  codec;              // 4  @0
        public uint  type;               // 4  @4
        public byte  fps;                // 1  @8
        private byte _pad1;             // 1  @9  (align ushort)
        public ushort width;             // 2  @10
        public ushort height;            // 2  @12
        private ushort _pad2;           // 2  @14 (align uint)
        public uint  reserved1;          // 4  @16
        public uint  reserved2;          // 4  @20
        public uint  reserved3;          // 4  @24
        public uint  sample_rate;        // 4  @28
        public uint  channels;           // 4  @32
        public uint  bits_per_sample;    // 4  @36
        public uint  length;             // 4  @40  ← frame data size
        public uint  timestamp_usec;     // 4  @44
        public uint  timestamp_sec;      // 4  @48
        public uint  pts;                // 4  @52
        public float bitrate;            // 4  @56
        public float losspacket;         // 4  @60
        // Total: 64 bytes
    }

    // ============================================================
    //  EASY_MEDIA_INFO_T  (mirrors EasyTypes.h)
    // ============================================================

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct EasyMediaInfo
    {
        public uint videoCodec;
        public uint videoFps;
        public uint audioCodec;
        public uint audioSampleRate;
        public uint audioChannel;
        public uint audioBitsPerSample;
        public uint vpsLength;
        public uint spsLength;
        public uint ppsLength;
        public uint seiLength;
        public fixed byte vps[256];
        public fixed byte sps[256];
        public fixed byte pps[128];
        public fixed byte sei[128];
    }

    // ============================================================
    //  GB28181_MEDIA_INFO_T  (mirrors EasyGB28181DeviceAPI.h)
    // ============================================================

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct GB28181MediaInfo
    {
        public uint videoCodec;
        public uint videoFps;
        public uint videoQueueSize;      // set 0 = use DLL default

        public uint audioCodec;
        public uint audioSampleRate;
        public uint audioChannel;
        public uint audioBitsPerSample;
        public uint audioQueueSize;      // set 0 = use DLL default

        public uint metadataCodec;
        public uint metadataQueueSize;

        public uint vpsLength;
        public uint spsLength;
        public uint ppsLength;
        public uint seiLength;

        public fixed byte vps[256];
        public fixed byte sps[256];
        public fixed byte pps[128];
        public fixed byte sei[128];
    }

    // ============================================================
    //  GB28181_MEDIA_FRAME_INFO_T  (mirrors EasyGB28181DeviceAPI.h)
    // ============================================================

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct GB28181MediaFrameInfo
    {
        public uint  codecID;
        public uint  frameSize;
        public uint  frameType;     // 0=P/audio, 1=I-frame
        public int   samplerate;
        public int   channels;
        public int   bitPerSample;
        public IntPtr pBuffer;      // unsigned char* (raw pointer)
        public uint  rtpTimestamp;
        public uint  fps;
        public fixed byte timestamp[6];  // BCD time, leave zero
    }

    // ============================================================
    //  Callback delegate for libEasyStreamClient
    // ============================================================

    /// <summary>
    /// EasyStreamClientCallBack: called by libEasyStreamClient.dll for each frame.
    /// channelPtr = userPtr passed to OpenStream
    /// frameType  = EASY_SDK_VIDEO_FRAME_FLAG / EASY_SDK_AUDIO_FRAME_FLAG /
    ///              EASY_SDK_MEDIA_INFO_FLAG / EASY_SDK_EVENT_FRAME_FLAG
    /// pBuf       = raw frame data pointer (valid only during this callback)
    /// pFrameInfo = pointer to EASY_FRAME_INFO struct
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate int EasyStreamClientCallBack(
        IntPtr channelPtr,
        int    frameType,
        IntPtr pBuf,
        IntPtr pFrameInfo);

    // ============================================================
    //  Frame type / codec constants  (from EasyTypes.h)
    // ============================================================

    public static class EasyConstants
    {
        public const int VIDEO_FRAME_FLAG      = 0x00000001;
        public const int AUDIO_FRAME_FLAG      = 0x00000002;
        public const int EVENT_FRAME_FLAG      = 0x00000004;
        public const int MEDIA_INFO_FLAG       = 0x00000020;

        public const uint VIDEO_CODEC_H264     = 0x1C;
        public const uint VIDEO_CODEC_H265     = 0xAE;
        public const uint AUDIO_CODEC_AAC      = 0x15002;
        public const uint AUDIO_CODEC_G711U    = 0x10006;
        public const uint AUDIO_CODEC_G711A    = 0x10007;
        public const uint AUDIO_CODEC_G726     = 0x1100B;

        public const int RTP_OVER_TCP          = 0x01;
        public const int RTP_OVER_UDP          = 0x02;

        public const uint MEDIA_TYPE_VIDEO     = 0x00000001;
        public const uint MEDIA_TYPE_AUDIO     = 0x00000002;
    }

    // ============================================================
    //  P/Invoke for libEasyStreamClient.dll
    // ============================================================

    public static class EasyStreamNative
    {
        private const string DllName = "libEasyStreamClient";

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "EasyStreamClient_Init")]
        public static extern int Init(out IntPtr handle, int loglevel);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "EasyStreamClient_Deinit")]
        public static extern int Deinit(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "EasyStreamClient_SetAudioEnable")]
        public static extern int SetAudioEnable(IntPtr handle, int enable);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "EasyStreamClient_SetCallback")]
        public static extern int SetCallback(IntPtr handle, EasyStreamClientCallBack callback);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, EntryPoint = "EasyStreamClient_OpenStream",
                   CharSet = CharSet.Ansi)]
        public static extern int OpenStream(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string url,
            int    connType,
            IntPtr userPtr,
            int    reconnect,
            int    timeout,
            int    useExtraData);
    }

    // ============================================================
    //  Additional P/Invoke entries for libEasyGBD.dll
    // ============================================================

    public static partial class EasyGBDNative
    {
        private const string DllNameGBD = "libEasyGBD";

        /// <summary>Set media codec info before starting to push frames</summary>
        [DllImport(DllNameGBD, CallingConvention = CallingConvention.StdCall,
                   EntryPoint = "libGB28181Client_SetMediaInfo")]
        public static extern unsafe int SetMediaInfo(
            IntPtr handle,
            IntPtr streamHandle,
            GB28181MediaInfo* pMediaInfo);

        /// <summary>Push one video or audio frame to the GB28181 stream</summary>
        [DllImport(DllNameGBD, CallingConvention = CallingConvention.StdCall,
                   EntryPoint = "libGB28181Client_PushFrame")]
        public static extern unsafe int PushFrame(
            IntPtr handle,
            IntPtr streamHandle,
            uint   mediaType,
            GB28181MediaFrameInfo* pFrameInfo,
            IntPtr frameData);
    }
}
