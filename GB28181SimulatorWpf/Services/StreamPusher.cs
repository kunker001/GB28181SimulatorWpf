using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using GB28181SimulatorWpf.NativeApi;

namespace GB28181SimulatorWpf.Services
{
    /// <summary>
    /// Maintains ONE EasyStreamClient connection per URL and fans out each
    /// received frame to every registered (deviceHandle, pusherHandle) pair.
    ///
    /// This avoids the "only one channel works" problem caused by the DLL or
    /// the RTSP source refusing duplicate connections from the same process.
    /// </summary>
    public sealed class StreamHub : IDisposable
    {
        // --------------------------------------------------------
        //  Pusher entry
        // --------------------------------------------------------

        private sealed class PusherEntry
        {
            public IntPtr DeviceHandle;
            public IntPtr PusherHandle;
            public bool   MediaInfoSet;
        }

        // --------------------------------------------------------
        //  State
        // --------------------------------------------------------

        private IntPtr _streamHandle = IntPtr.Zero;
        private readonly string _url;
        private readonly Action<string> _log;

        private readonly object _lock = new();
        private readonly List<PusherEntry> _pushers = new();

        private EasyStreamClientCallBack? _cbDelegate;
        private GCHandle _selfGch;

        // --------------------------------------------------------
        //  Construction
        // --------------------------------------------------------

        public StreamHub(string url, Action<string>? log = null)
        {
            _url = url;
            _log = log ?? (_ => { });
        }

        // --------------------------------------------------------
        //  Register / Unregister pushers
        // --------------------------------------------------------

        public void AddPusher(IntPtr deviceHandle, IntPtr pusherHandle)
        {
            lock (_lock)
            {
                _pushers.Add(new PusherEntry
                {
                    DeviceHandle = deviceHandle,
                    PusherHandle = pusherHandle,
                });
                _log($"AddPusher total={_pushers.Count}");

                // Start EasyStreamClient on first pusher
                if (_pushers.Count == 1)
                    StartStream();
            }
        }

        public void RemovePusher(IntPtr pusherHandle)
        {
            lock (_lock)
            {
                _pushers.RemoveAll(p => p.PusherHandle == pusherHandle);
                _log($"RemovePusher remaining={_pushers.Count}");

                if (_pushers.Count == 0)
                    StopStream();
            }
        }

        public bool IsEmpty
        {
            get { lock (_lock) { return _pushers.Count == 0; } }
        }

        // --------------------------------------------------------
        //  EasyStreamClient lifecycle
        // --------------------------------------------------------

        private void StartStream()
        {
            if (_streamHandle != IntPtr.Zero) return;

            _selfGch     = GCHandle.Alloc(this);
            _cbDelegate  = OnFrameReceived;

            int ret = EasyStreamNative.Init(out _streamHandle, 0);
            if (ret != 0 || _streamHandle == IntPtr.Zero)
            {
                _log($"EasyStreamClient_Init 失败 ret={ret}");
                _selfGch.Free();
                return;
            }

            EasyStreamNative.SetAudioEnable(_streamHandle, 1);
            EasyStreamNative.SetCallback(_streamHandle, _cbDelegate);

            ret = EasyStreamNative.OpenStream(
                _streamHandle, _url,
                EasyConstants.RTP_OVER_TCP,
                GCHandle.ToIntPtr(_selfGch),
                3, 10, 1);

            _log(ret == 0
                ? $"拉流已启动: {_url}"
                : $"OpenStream 失败 ret={ret} url={_url}");
        }

        private void StopStream()
        {
            if (_streamHandle == IntPtr.Zero) return;

            EasyStreamNative.Deinit(_streamHandle);
            _streamHandle = IntPtr.Zero;
            _log("拉流已停止");

            if (_selfGch.IsAllocated)
                _selfGch.Free();
        }

        // --------------------------------------------------------
        //  Frame callback (EasyStreamClient DLL thread)
        // --------------------------------------------------------

        private int OnFrameReceived(IntPtr channelPtr, int frameType, IntPtr pBuf, IntPtr pFrameInfo)
        {
            if (pFrameInfo == IntPtr.Zero) return 0;

            var fi = Marshal.PtrToStructure<EasyFrameInfo>(pFrameInfo);

            List<PusherEntry> snapshot;
            lock (_lock) { snapshot = new List<PusherEntry>(_pushers); }

            foreach (var entry in snapshot)
            {
                try { Dispatch(entry, frameType, pBuf, ref fi); }
                catch { /* ignore per-pusher errors */ }
            }

            return 0;
        }

        private unsafe void Dispatch(PusherEntry entry, int frameType, IntPtr pBuf, ref EasyFrameInfo fi)
        {
            // ── Media info ─────────────────────────────────────
            if (frameType == EasyConstants.MEDIA_INFO_FLAG)
            {
                var mi = new GB28181MediaInfo
                {
                    videoCodec         = fi.codec <= 0xFF ? fi.codec : EasyConstants.VIDEO_CODEC_H264,
                    videoFps           = fi.fps > 0 ? fi.fps : 25u,
                    audioCodec         = EasyConstants.AUDIO_CODEC_G711A,
                    audioSampleRate    = fi.sample_rate > 0 ? fi.sample_rate : 8000u,
                    audioChannel       = fi.channels > 0 ? fi.channels : 1u,
                    audioBitsPerSample = fi.bits_per_sample > 0 ? fi.bits_per_sample : 16u,
                };
                EasyGBDNative.SetMediaInfo(entry.DeviceHandle, entry.PusherHandle, &mi);
                entry.MediaInfoSet = true;
                return;
            }

            if (pBuf == IntPtr.Zero || fi.length == 0) return;

            // ── Video ──────────────────────────────────────────
            if (frameType == EasyConstants.VIDEO_FRAME_FLAG)
            {
                if (!entry.MediaInfoSet) SetMediaInfoLazy(entry, ref fi);

                var frame = new GB28181MediaFrameInfo
                {
                    codecID      = fi.codec,
                    frameSize    = fi.length,
                    frameType    = fi.type == 1 ? 1u : 0u,
                    pBuffer      = pBuf,
                    rtpTimestamp = fi.pts > 0 ? fi.pts
                                 : fi.timestamp_sec * 90000u + fi.timestamp_usec / 11u,
                    fps          = fi.fps,
                };
                EasyGBDNative.PushFrame(entry.DeviceHandle, entry.PusherHandle,
                    EasyConstants.MEDIA_TYPE_VIDEO, &frame, pBuf);
                return;
            }

            // ── Audio ──────────────────────────────────────────
            if (frameType == EasyConstants.AUDIO_FRAME_FLAG)
            {
                var frame = new GB28181MediaFrameInfo
                {
                    codecID      = fi.codec,
                    frameSize    = fi.length,
                    frameType    = 0,
                    samplerate   = (int)(fi.sample_rate > 0 ? fi.sample_rate : 8000u),
                    channels     = (int)(fi.channels > 0 ? fi.channels : 1u),
                    bitPerSample = (int)(fi.bits_per_sample > 0 ? fi.bits_per_sample : 16u),
                    pBuffer      = pBuf,
                    rtpTimestamp = fi.pts > 0 ? fi.pts
                                 : fi.timestamp_sec * 8000u + fi.timestamp_usec / 125u,
                };
                EasyGBDNative.PushFrame(entry.DeviceHandle, entry.PusherHandle,
                    EasyConstants.MEDIA_TYPE_AUDIO, &frame, pBuf);
            }
        }

        private unsafe void SetMediaInfoLazy(PusherEntry entry, ref EasyFrameInfo fi)
        {
            var mi = new GB28181MediaInfo
            {
                videoCodec         = fi.codec <= 0xFF ? fi.codec : EasyConstants.VIDEO_CODEC_H264,
                videoFps           = fi.fps > 0 ? fi.fps : 25u,
                audioCodec         = EasyConstants.AUDIO_CODEC_G711A,
                audioSampleRate    = 8000u,
                audioChannel       = 1u,
                audioBitsPerSample = 16u,
            };
            EasyGBDNative.SetMediaInfo(entry.DeviceHandle, entry.PusherHandle, &mi);
            entry.MediaInfoSet = true;
        }

        // --------------------------------------------------------
        //  IDisposable
        // --------------------------------------------------------

        public void Dispose()
        {
            lock (_lock)
            {
                _pushers.Clear();
                StopStream();
            }
        }
    }
}
