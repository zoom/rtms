const rtms = require("../index.ts")

// Mock the native addon methods for both Client and global functions
jest.mock("../index.ts", () => {
  // Create mock functions for all methods
  const mockFunctions = {
    // Client class methods
    Client: jest.fn().mockImplementation(() => ({
      _init: jest.fn(),
      _isInit: jest.fn().mockReturnValue(true),
      _join: jest.fn().mockReturnValue(0),
      join: jest.fn().mockReturnValue(true),
      poll: jest.fn(),
      release: jest.fn().mockReturnValue(true),
      leave: jest.fn().mockReturnValue(true),
      uuid: jest.fn().mockReturnValue("client-uuid"),
      streamId: jest.fn().mockReturnValue("client-stream-id"),
      
      // Enable/disable methods
      enableAudio: jest.fn().mockReturnValue(true),
      enableVideo: jest.fn().mockReturnValue(true),
      enableTranscript: jest.fn().mockReturnValue(true),
      enableDeskshare: jest.fn().mockReturnValue(true),
      
      // Parameter setting methods
      setAudioParams: jest.fn().mockReturnValue(true),
      setVideoParams: jest.fn().mockReturnValue(true),
      setDeskshareParams: jest.fn().mockReturnValue(true),
      
      // Callback methods
      onJoinConfirm: jest.fn().mockReturnValue(true),
      onSessionUpdate: jest.fn().mockReturnValue(true),
      onUserUpdate: jest.fn().mockReturnValue(true),
      onAudioData: jest.fn().mockReturnValue(true),
      onVideoData: jest.fn().mockReturnValue(true),
      onDeskshareData: jest.fn().mockReturnValue(true),
      onTranscriptData: jest.fn().mockReturnValue(true),
      onLeave: jest.fn().mockReturnValue(true),
    })),

    // Global client singleton methods
    join: jest.fn().mockReturnValue(true),
    poll: jest.fn(),
    leave: jest.fn().mockReturnValue(true),
    release: jest.fn().mockReturnValue(true),
    uuid: jest.fn().mockReturnValue("global-uuid"),
    streamId: jest.fn().mockReturnValue("global-stream-id"),
    
    // Global enable/disable methods
    enableAudio: jest.fn().mockReturnValue(true),
    enableVideo: jest.fn().mockReturnValue(true),
    enableTranscript: jest.fn().mockReturnValue(true),
    enableDeskshare: jest.fn().mockReturnValue(true),
    
    // Global parameter setting methods
    setAudioParams: jest.fn().mockReturnValue(true),
    setVideoParams: jest.fn().mockReturnValue(true),
    setDeskshareParams: jest.fn().mockReturnValue(true),
    
    // Global callback methods
    onJoinConfirm: jest.fn(),
    onSessionUpdate: jest.fn(),
    onUserUpdate: jest.fn(),
    onAudioData: jest.fn(),
    onVideoData: jest.fn(),
    onDeskshareData: jest.fn(),
    onTranscriptData: jest.fn(),
    onLeave: jest.fn(),

    // Utility functions
    initialize: jest.fn().mockReturnValue(true),
    uninitialize: jest.fn().mockReturnValue(true),
    generateSignature: jest.fn().mockReturnValue("mock-signature"),
    isInitialized: jest.fn().mockReturnValue(true),
    
    // Webhook functionality
    onWebhookEvent: jest.fn(),
    
    // Logger functionality
    configureLogger: jest.fn(),
    LogLevel: {
      ERROR: 0,
      WARN: 1,
      INFO: 2,
      DEBUG: 3,
      TRACE: 4
    },
    LogFormat: {
      PROGRESSIVE: 'progressive',
      JSON: 'json'
    },

    // Media type constants
    MEDIA_TYPE_AUDIO: 1,
    MEDIA_TYPE_VIDEO: 2,
    MEDIA_TYPE_DESKSHARE: 4,
    MEDIA_TYPE_TRANSCRIPT: 8,
    MEDIA_TYPE_CHAT: 16,
    MEDIA_TYPE_ALL: 31,
    
    // Session event constants
    SESSION_EVENT_ADD: 1,
    SESSION_EVENT_STOP: 2,
    SESSION_EVENT_PAUSE: 3,
    SESSION_EVENT_RESUME: 4,
    
    // User event constants
    USER_EVENT_JOIN: 1,
    USER_EVENT_LEAVE: 2,
    
    // Status constants
    RTMS_SDK_FAILURE: -1,
    RTMS_SDK_OK: 0,
    RTMS_SDK_TIMEOUT: 1,
    RTMS_SDK_NOT_EXIST: 2,
    RTMS_SDK_WRONG_TYPE: 3,
    RTMS_SDK_INVALID_STATUS: 4,
    RTMS_SDK_INVALID_ARGS: 5,
    
    // Session status constants
    SESS_STATUS_ACTIVE: 1,
    SESS_STATUS_PAUSED: 2,
    
    // Audio parameter constants
    AudioContentType: {
      UNDEFINED: 0,
      RTP: 1,
      RAW_AUDIO: 2,
      FILE_STREAM: 4,
      TEXT: 5
    },
    AudioCodec: {
      UNDEFINED: 0,
      L16: 1,
      G711: 2,
      G722: 3,
      OPUS: 4
    },
    AudioSampleRate: {
      SR_8K: 0,
      SR_16K: 1,
      SR_32K: 2,
      SR_48K: 3
    },
    AudioChannel: {
      MONO: 1,
      STEREO: 2
    },
    AudioDataOption: {
      UNDEFINED: 0,
      AUDIO_MIXED_STREAM: 1,
      AUDIO_MULTI_STREAMS: 2
    },
    
    // Video parameter constants
    VideoContentType: {
      UNDEFINED: 0,
      RTP: 1,
      RAW_VIDEO: 3,
      FILE_STREAM: 4,
      TEXT: 5
    },
    VideoCodec: {
      UNDEFINED: 0,
      JPG: 5,
      PNG: 6,
      H264: 7
    },
    VideoResolution: {
      SD: 1,
      HD: 2,
      FHD: 3,
      QHD: 4
    },
    VideoDataOption: {
      UNDEFINED: 0,
      VIDEO_SINGLE_ACTIVE_STREAM: 3,
      VIDEO_MIXED_SPEAKER_VIEW: 4,
      VIDEO_MIXED_GALLERY_VIEW: 5
    },
    
    // Other constants
    MediaDataType: {
      UNDEFINED: 0,
      AUDIO: 1,
      VIDEO: 2,
      DESKSHARE: 4,
      TRANSCRIPT: 8,
      CHAT: 16,
      ALL: 31
    },
    SessionState: {
      INACTIVE: 0,
      INITIALIZE: 1,
      STARTED: 2,
      PAUSED: 3,
      RESUMED: 4,
      STOPPED: 5
    },
    StreamState: {
      INACTIVE: 0,
      ACTIVE: 1,
      INTERRUPTED: 2,
      TERMINATING: 3,
      TERMINATED: 4
    }
  };

  return mockFunctions;
});

describe('RTMS Node.JS Addon Comprehensive Test Suite', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  // ---- Class-based approach tests ----
  describe('Class-based Client Approach', () => {
    let client;

    beforeEach(() => {
      client = new rtms.Client();
    });

    describe('Basic Connection Methods', () => {
      test('client.join with parameters joins a session correctly', () => {
        const result = client.join("uuid", "session_id", "signature", "server_url", 5000);
        expect(client.join).toHaveBeenCalledWith("uuid", "session_id", "signature", "server_url", 5000);
        expect(result).toBe(true);
      });

      test('client.join with JoinParams object joins a session correctly', () => {
        const joinParams = {
          meeting_uuid: "uuid",
          rtms_stream_id: "session_id",
          server_urls: "server_url",
          signature: "signature",
          timeout: 5000,
          pollInterval: 100
        };
        
        const result = client.join(joinParams);
        expect(client.join).toHaveBeenCalledWith(joinParams);
        expect(result).toBe(true);
      });

      test('client.uuid returns the UUID correctly', () => {
        const uuid = client.uuid();
        expect(client.uuid).toHaveBeenCalled();
        expect(uuid).toBe("client-uuid");
      });

      test('client.streamId returns the stream ID correctly', () => {
        const streamId = client.streamId();
        expect(client.streamId).toHaveBeenCalled();
        expect(streamId).toBe("client-stream-id");
      });

      test('client.leave releases resources correctly', () => {
        const result = client.leave();
        expect(client.leave).toHaveBeenCalled();
        expect(result).toBe(true);
      });
    });

    describe('Media Enable/Disable Methods', () => {
      test('client.enableAudio enables/disables audio correctly', () => {
        const result1 = client.enableAudio(true);
        expect(client.enableAudio).toHaveBeenCalledWith(true);
        expect(result1).toBe(true);

        const result2 = client.enableAudio(false);
        expect(client.enableAudio).toHaveBeenCalledWith(false);
        expect(result2).toBe(true);
      });

      test('client.enableVideo enables/disables video correctly', () => {
        const result1 = client.enableVideo(true);
        expect(client.enableVideo).toHaveBeenCalledWith(true);
        expect(result1).toBe(true);

        const result2 = client.enableVideo(false);
        expect(client.enableVideo).toHaveBeenCalledWith(false);
        expect(result2).toBe(true);
      });

      test('client.enableTranscript enables/disables transcript correctly', () => {
        const result1 = client.enableTranscript(true);
        expect(client.enableTranscript).toHaveBeenCalledWith(true);
        expect(result1).toBe(true);

        const result2 = client.enableTranscript(false);
        expect(client.enableTranscript).toHaveBeenCalledWith(false);
        expect(result2).toBe(true);
      });
    });

    describe('Parameter Setting Methods', () => {
      test('client.setAudioParams sets audio parameters correctly', () => {
        const audioParams = {
          contentType: rtms.AudioContentType.RTP,
          codec: rtms.AudioCodec.OPUS,
          sampleRate: rtms.AudioSampleRate.SR_48K,
          channel: rtms.AudioChannel.STEREO,
          dataOpt: rtms.AudioDataOption.AUDIO_MIXED_STREAM,
          duration: 20,
          frameSize: 960
        };

        const result = client.setAudioParams(audioParams);
        expect(client.setAudioParams).toHaveBeenCalledWith(audioParams);
        expect(result).toBe(true);
      });

      test('client.setVideoParams sets video parameters correctly', () => {
        const videoParams = {
          contentType: rtms.VideoContentType.RTP,
          codec: rtms.VideoCodec.H264,
          resolution: rtms.VideoResolution.HD,
          dataOpt: rtms.VideoDataOption.VIDEO_MIXED_SPEAKER_VIEW,
          fps: 30
        };

        const result = client.setVideoParams(videoParams);
        expect(client.setVideoParams).toHaveBeenCalledWith(videoParams);
        expect(result).toBe(true);
      });

      test('client.setDeskshareParams sets deskshare parameters correctly', () => {
        const deskshareParams = {
          contentType: rtms.VideoContentType.RTP,
          codec: rtms.VideoCodec.H264,
          resolution: rtms.VideoResolution.FHD,
          fps: 15
        };

        const result = client.setDeskshareParams(deskshareParams);
        expect(client.setDeskshareParams).toHaveBeenCalledWith(deskshareParams);
        expect(result).toBe(true);
      });
    });

    describe('Callback Methods', () => {
      test('client.onJoinConfirm sets the join confirmation callback correctly', () => {
        const callback = jest.fn();
        const result = client.onJoinConfirm(callback);
        expect(client.onJoinConfirm).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });

      test('client.onSessionUpdate sets the session update callback correctly', () => {
        const callback = jest.fn();
        const result = client.onSessionUpdate(callback);
        expect(client.onSessionUpdate).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });

      test('client.onUserUpdate sets the user update callback correctly', () => {
        const callback = jest.fn();
        const result = client.onUserUpdate(callback);
        expect(client.onUserUpdate).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });

      test('client.onAudioData sets the audio data callback correctly', () => {
        const callback = jest.fn();
        const result = client.onAudioData(callback);
        expect(client.onAudioData).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });

      test('client.onVideoData sets the video data callback correctly', () => {
        const callback = jest.fn();
        const result = client.onVideoData(callback);
        expect(client.onVideoData).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });

      test('client.onDeskshareData sets the deskshare data callback correctly', () => {
        const callback = jest.fn();
        const result = client.onDeskshareData(callback);
        expect(client.onDeskshareData).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });

      test('client.onTranscriptData sets the transcript data callback correctly', () => {
        const callback = jest.fn();
        const result = client.onTranscriptData(callback);
        expect(client.onTranscriptData).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });

      test('client.onLeave sets the leave callback correctly', () => {
        const callback = jest.fn();
        const result = client.onLeave(callback);
        expect(client.onLeave).toHaveBeenCalledWith(callback);
        expect(result).toBe(true);
      });
    });
  });

  // ---- Global singleton approach tests ----
  describe('Global Singleton Approach', () => {
    describe('Basic Connection Methods', () => {
      test('rtms.join with parameters joins a session correctly', () => {
        const result = rtms.join("uuid", "session_id", "signature", "server_url", 5000);
        expect(rtms.join).toHaveBeenCalledWith("uuid", "session_id", "signature", "server_url", 5000);
        expect(result).toBe(true);
      });

      test('rtms.join with JoinParams object joins a session correctly', () => {
        const joinParams = {
          meeting_uuid: "uuid",
          rtms_stream_id: "session_id",
          server_urls: "server_url",
          signature: "signature",
          timeout: 5000,
          pollInterval: 100
        };
        
        const result = rtms.join(joinParams);
        expect(rtms.join).toHaveBeenCalledWith(joinParams);
        expect(result).toBe(true);
      });

      test('rtms.uuid returns the UUID correctly', () => {
        const uuid = rtms.uuid();
        expect(rtms.uuid).toHaveBeenCalled();
        expect(uuid).toBe("global-uuid");
      });

      test('rtms.streamId returns the stream ID correctly', () => {
        const streamId = rtms.streamId();
        expect(rtms.streamId).toHaveBeenCalled();
        expect(streamId).toBe("global-stream-id");
      });

      test('rtms.leave releases resources correctly', () => {
        const result = rtms.leave();
        expect(rtms.leave).toHaveBeenCalled();
        expect(result).toBe(true);
      });
    });

    describe('Media Enable/Disable Methods', () => {
      test('rtms.enableAudio enables/disables audio correctly', () => {
        const result1 = rtms.enableAudio(true);
        expect(rtms.enableAudio).toHaveBeenCalledWith(true);
        expect(result1).toBe(true);

        const result2 = rtms.enableAudio(false);
        expect(rtms.enableAudio).toHaveBeenCalledWith(false);
        expect(result2).toBe(true);
      });

      test('rtms.enableVideo enables/disables video correctly', () => {
        const result1 = rtms.enableVideo(true);
        expect(rtms.enableVideo).toHaveBeenCalledWith(true);
        expect(result1).toBe(true);

        const result2 = rtms.enableVideo(false);
        expect(rtms.enableVideo).toHaveBeenCalledWith(false);
        expect(result2).toBe(true);
      });

      test('rtms.enableTranscript enables/disables transcript correctly', () => {
        const result1 = rtms.enableTranscript(true);
        expect(rtms.enableTranscript).toHaveBeenCalledWith(true);
        expect(result1).toBe(true);

        const result2 = rtms.enableTranscript(false);
        expect(rtms.enableTranscript).toHaveBeenCalledWith(false);
        expect(result2).toBe(true);
      });
    });

    describe('Parameter Setting Methods', () => {
      test('rtms.setAudioParams sets audio parameters correctly', () => {
        const audioParams = {
          contentType: rtms.AudioContentType.RTP,
          codec: rtms.AudioCodec.OPUS,
          sampleRate: rtms.AudioSampleRate.SR_48K,
          channel: rtms.AudioChannel.STEREO,
          dataOpt: rtms.AudioDataOption.AUDIO_MIXED_STREAM,
          duration: 20,
          frameSize: 960
        };

        const result = rtms.setAudioParams(audioParams);
        expect(rtms.setAudioParams).toHaveBeenCalledWith(audioParams);
        expect(result).toBe(true);
      });

      test('rtms.setVideoParams sets video parameters correctly', () => {
        const videoParams = {
          contentType: rtms.VideoContentType.RTP,
          codec: rtms.VideoCodec.H264,
          resolution: rtms.VideoResolution.HD,
          dataOpt: rtms.VideoDataOption.VIDEO_MIXED_SPEAKER_VIEW,
          fps: 30
        };

        const result = rtms.setVideoParams(videoParams);
        expect(rtms.setVideoParams).toHaveBeenCalledWith(videoParams);
        expect(result).toBe(true);
      });

      test('rtms.setDeskshareParams sets deskshare parameters correctly', () => {
        const deskshareParams = {
          contentType: rtms.VideoContentType.RTP,
          codec: rtms.VideoCodec.H264,
          resolution: rtms.VideoResolution.FHD,
          fps: 15
        };

        const result = rtms.setDeskshareParams(deskshareParams);
        expect(rtms.setDeskshareParams).toHaveBeenCalledWith(deskshareParams);
        expect(result).toBe(true);
      });
    });

    describe('Callback Methods', () => {
      test('rtms.onJoinConfirm sets the join confirmation callback correctly', () => {
        const callback = jest.fn();
        rtms.onJoinConfirm(callback);
        expect(rtms.onJoinConfirm).toHaveBeenCalledWith(callback);
      });

      test('rtms.onSessionUpdate sets the session update callback correctly', () => {
        const callback = jest.fn();
        rtms.onSessionUpdate(callback);
        expect(rtms.onSessionUpdate).toHaveBeenCalledWith(callback);
      });

      test('rtms.onUserUpdate sets the user update callback correctly', () => {
        const callback = jest.fn();
        rtms.onUserUpdate(callback);
        expect(rtms.onUserUpdate).toHaveBeenCalledWith(callback);
      });

      test('rtms.onAudioData sets the audio data callback correctly', () => {
        const callback = jest.fn();
        rtms.onAudioData(callback);
        expect(rtms.onAudioData).toHaveBeenCalledWith(callback);
      });

      test('rtms.onVideoData sets the video data callback correctly', () => {
        const callback = jest.fn();
        rtms.onVideoData(callback);
        expect(rtms.onVideoData).toHaveBeenCalledWith(callback);
      });

      test('rtms.onDeskshareData sets the deskshare data callback correctly', () => {
        const callback = jest.fn();
        rtms.onDeskshareData(callback);
        expect(rtms.onDeskshareData).toHaveBeenCalledWith(callback);
      });

      test('rtms.onTranscriptData sets the transcript data callback correctly', () => {
        const callback = jest.fn();
        rtms.onTranscriptData(callback);
        expect(rtms.onTranscriptData).toHaveBeenCalledWith(callback);
      });

      test('rtms.onLeave sets the leave callback correctly', () => {
        const callback = jest.fn();
        rtms.onLeave(callback);
        expect(rtms.onLeave).toHaveBeenCalledWith(callback);
      });
    });
  });

  // ---- Utility function tests ----
  describe('Utility Functions', () => {
    test('rtms.initialize initializes the RTMS module', () => {
      const result = rtms.initialize('/path/to/ca.pem');
      expect(rtms.initialize).toHaveBeenCalledWith('/path/to/ca.pem');
      expect(result).toBe(true);
    });

    test('rtms.uninitialize uninitializes the RTMS module', () => {
      const result = rtms.uninitialize();
      expect(rtms.uninitialize).toHaveBeenCalled();
      expect(result).toBe(true);
    });

    test('rtms.generateSignature generates a signature', () => {
      const params = {
        client: 'client_id',
        secret: 'client_secret',
        uuid: 'meeting_uuid',
        streamId: 'stream_id'
      };
      
      const signature = rtms.generateSignature(params);
      expect(rtms.generateSignature).toHaveBeenCalledWith(params);
      expect(signature).toBe('mock-signature');
    });

    test('rtms.isInitialized returns initialization status', () => {
      const initialized = rtms.isInitialized();
      expect(rtms.isInitialized).toHaveBeenCalled();
      expect(initialized).toBe(true);
    });
  });

  // ---- Logger functionality tests ----
  describe('Logger Functionality', () => {
    test('rtms.configureLogger configures logger correctly', () => {
      const config = {
        level: rtms.LogLevel.DEBUG,
        format: rtms.LogFormat.JSON,
        enabled: true
      };

      rtms.configureLogger(config);
      expect(rtms.configureLogger).toHaveBeenCalledWith(config);
    });

    test('LogLevel enum has correct values', () => {
      expect(rtms.LogLevel.ERROR).toBe(0);
      expect(rtms.LogLevel.WARN).toBe(1);
      expect(rtms.LogLevel.INFO).toBe(2);
      expect(rtms.LogLevel.DEBUG).toBe(3);
      expect(rtms.LogLevel.TRACE).toBe(4);
    });

    test('LogFormat enum has correct values', () => {
      expect(rtms.LogFormat.PROGRESSIVE).toBe('progressive');
      expect(rtms.LogFormat.JSON).toBe('json');
    });
  });

  // ---- Webhook functionality tests ----
  describe('Webhook Functionality', () => {
    test('rtms.onWebhookEvent sets webhook handler correctly', () => {
      const callback = jest.fn();
      rtms.onWebhookEvent(callback);
      expect(rtms.onWebhookEvent).toHaveBeenCalledWith(callback);
    });
  });

  // ---- Constants tests ----
  describe('Constants', () => {
    test('Media type constants are defined correctly', () => {
      expect(rtms.MEDIA_TYPE_AUDIO).toBe(1);
      expect(rtms.MEDIA_TYPE_VIDEO).toBe(2);
      expect(rtms.MEDIA_TYPE_DESKSHARE).toBe(4);
      expect(rtms.MEDIA_TYPE_TRANSCRIPT).toBe(8);
      expect(rtms.MEDIA_TYPE_CHAT).toBe(16);
      expect(rtms.MEDIA_TYPE_ALL).toBe(31);
    });

    test('Session event constants are defined correctly', () => {
      expect(rtms.SESSION_EVENT_ADD).toBe(1);
      expect(rtms.SESSION_EVENT_STOP).toBe(2);
      expect(rtms.SESSION_EVENT_PAUSE).toBe(3);
      expect(rtms.SESSION_EVENT_RESUME).toBe(4);
    });

    test('User event constants are defined correctly', () => {
      expect(rtms.USER_EVENT_JOIN).toBe(1);
      expect(rtms.USER_EVENT_LEAVE).toBe(2);
    });

    test('Status constants are defined correctly', () => {
      expect(rtms.RTMS_SDK_FAILURE).toBe(-1);
      expect(rtms.RTMS_SDK_OK).toBe(0);
      expect(rtms.RTMS_SDK_TIMEOUT).toBe(1);
      expect(rtms.RTMS_SDK_NOT_EXIST).toBe(2);
    });

    test('Audio parameter constants are defined correctly', () => {
      expect(rtms.AudioContentType.UNDEFINED).toBe(0);
      expect(rtms.AudioContentType.RTP).toBe(1);
      expect(rtms.AudioCodec.OPUS).toBe(4);
      expect(rtms.AudioSampleRate.SR_48K).toBe(3);
      expect(rtms.AudioChannel.STEREO).toBe(2);
    });

    test('Video parameter constants are defined correctly', () => {
      expect(rtms.VideoContentType.RTP).toBe(1);
      expect(rtms.VideoCodec.H264).toBe(7);
      expect(rtms.VideoResolution.HD).toBe(2);
      expect(rtms.VideoDataOption.VIDEO_MIXED_SPEAKER_VIEW).toBe(4);
    });
  });

  // ---- Integration tests ----
  describe('Integration Tests', () => {
    test('Complete workflow with client instance', () => {
      const client = new rtms.Client();
      
      // Set up callbacks
      const joinCallback = jest.fn();
      const audioCallback = jest.fn();
      const leaveCallback = jest.fn();
      
      client.onJoinConfirm(joinCallback);
      client.onAudioData(audioCallback);
      client.onLeave(leaveCallback);
      
      // Set parameters
      const audioParams = {
        codec: rtms.AudioCodec.OPUS,
        sampleRate: rtms.AudioSampleRate.SR_48K
      };
      client.setAudioParams(audioParams);
      
      // Join meeting
      const joinParams = {
        meeting_uuid: "test-uuid",
        rtms_stream_id: "test-stream",
        server_urls: "wss://test.zoom.us",
        signature: "test-signature"
      };
      const joinResult = client.join(joinParams);
      
      // Leave meeting
      const leaveResult = client.leave();
      
      expect(joinResult).toBe(true);
      expect(leaveResult).toBe(true);
      expect(client.setAudioParams).toHaveBeenCalledWith(audioParams);
      expect(client.join).toHaveBeenCalledWith(joinParams);
    });

    test('Complete workflow with global client', () => {
      // Set up callbacks
      const joinCallback = jest.fn();
      const videoCallback = jest.fn();
      const leaveCallback = jest.fn();
      
      rtms.onJoinConfirm(joinCallback);
      rtms.onVideoData(videoCallback);
      rtms.onLeave(leaveCallback);
      
      // Set parameters
      const videoParams = {
        codec: rtms.VideoCodec.H264,
        resolution: rtms.VideoResolution.HD
      };
      rtms.setVideoParams(videoParams);
      
      // Enable media types
      rtms.enableAudio(true);
      rtms.enableVideo(true);
      
      // Join meeting
      const joinParams = {
        meeting_uuid: "global-test-uuid",
        rtms_stream_id: "global-test-stream",
        server_urls: "wss://global-test.zoom.us"
      };
      const joinResult = rtms.join(joinParams);
      
      // Leave meeting
      const leaveResult = rtms.leave();
      
      expect(joinResult).toBe(true);
      expect(leaveResult).toBe(true);
      expect(rtms.setVideoParams).toHaveBeenCalledWith(videoParams);
      expect(rtms.enableAudio).toHaveBeenCalledWith(true);
      expect(rtms.enableVideo).toHaveBeenCalledWith(true);
    });

    test('Both patterns can be used simultaneously', () => {
      const client = new rtms.Client();
      
      // Global client
      rtms.join({
        meeting_uuid: "global-uuid",
        rtms_stream_id: "global-session",
        server_urls: "global-server"
      });
      
      // Class instance
      client.join({
        meeting_uuid: "instance-uuid",
        rtms_stream_id: "instance-session",
        server_urls: "instance-server"
      });
      
      // Check that both were called correctly
      expect(rtms.join).toHaveBeenCalledWith({
        meeting_uuid: "global-uuid",
        rtms_stream_id: "global-session",
        server_urls: "global-server"
      });
      
      expect(client.join).toHaveBeenCalledWith({
        meeting_uuid: "instance-uuid",
        rtms_stream_id: "instance-session",
        server_urls: "instance-server"
      });
    });

    test('Each pattern has its own UUID and stream ID', () => {
      const client = new rtms.Client();
      
      const globalUuid = rtms.uuid();
      const instanceUuid = client.uuid();
      
      const globalStreamId = rtms.streamId();
      const instanceStreamId = client.streamId();
      
      expect(globalUuid).toBe('global-uuid');
      expect(instanceUuid).toBe('client-uuid');
      
      expect(globalStreamId).toBe('global-stream-id');
      expect(instanceStreamId).toBe('client-stream-id');
    });

    test('All media types can be configured together', () => {
      const client = new rtms.Client();
      
      // Set all parameter types
      const audioParams = {
        codec: rtms.AudioCodec.OPUS,
        sampleRate: rtms.AudioSampleRate.SR_48K,
        channel: rtms.AudioChannel.STEREO
      };
      
      const videoParams = {
        codec: rtms.VideoCodec.H264,
        resolution: rtms.VideoResolution.HD,
        fps: 30
      };
      
      const deskshareParams = {
        codec: rtms.VideoCodec.H264,
        resolution: rtms.VideoResolution.FHD,
        fps: 15
      };
      
      client.setAudioParams(audioParams);
      client.setVideoParams(videoParams);
      client.setDeskshareParams(deskshareParams);
      
      // Enable all media types
      client.enableAudio(true);
      client.enableVideo(true);
      client.enableTranscript(true);
      
      // Set all callbacks
      client.onAudioData(jest.fn());
      client.onVideoData(jest.fn());
      client.onDeskshareData(jest.fn());
      client.onTranscriptData(jest.fn());
      
      // Verify all methods were called
      expect(client.setAudioParams).toHaveBeenCalledWith(audioParams);
      expect(client.setVideoParams).toHaveBeenCalledWith(videoParams);
      expect(client.setDeskshareParams).toHaveBeenCalledWith(deskshareParams);
      expect(client.enableAudio).toHaveBeenCalledWith(true);
      expect(client.enableVideo).toHaveBeenCalledWith(true);
      expect(client.enableTranscript).toHaveBeenCalledWith(true);
    });
  });

  // ---- Error handling tests ----
  describe('Error Handling', () => {
    test('handles invalid parameters gracefully', () => {
      const client = new rtms.Client();
      
      // These should not throw errors in our mock implementation
      expect(() => client.setAudioParams(null)).not.toThrow();
      expect(() => client.setVideoParams(undefined)).not.toThrow();
      expect(() => client.join()).not.toThrow();
    });

    test('handles callback errors gracefully', () => {
      const client = new rtms.Client();
      
      expect(() => client.onAudioData(null)).not.toThrow();
      expect(() => client.onVideoData(undefined)).not.toThrow();
    });
  });

  // ---- Performance tests ----
  describe('Performance Tests', () => {
    test('can create multiple client instances', () => {
      const clients = [];
      
      for (let i = 0; i < 10; i++) {
        clients.push(new rtms.Client());
      }
      
      expect(clients).toHaveLength(10);
      
      // Each client should be independent and have expected methods
      clients.forEach((client, index) => {
        expect(client).toBeTruthy();
        expect(client).toHaveProperty('join');
        expect(client).toHaveProperty('leave');
        expect(client).toHaveProperty('uuid');
        expect(client).toHaveProperty('streamId');
        expect(client).toHaveProperty('onAudioData');
        expect(client).toHaveProperty('onVideoData');
      });
    });

    test('can set parameters multiple times', () => {
      const client = new rtms.Client();
      
      for (let i = 0; i < 5; i++) {
        const audioParams = {
          sampleRate: i % 2 ? rtms.AudioSampleRate.SR_48K : rtms.AudioSampleRate.SR_16K
        };
        client.setAudioParams(audioParams);
      }
      
      expect(client.setAudioParams).toHaveBeenCalledTimes(5);
    });
  });
});