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
      uuid: jest.fn().mockReturnValue("client-uuid"),
      streamId: jest.fn().mockReturnValue("client-stream-id"),
      onJoinConfirm: jest.fn(),
      onSessionUpdate: jest.fn(),
      onUserUpdate: jest.fn(),
      onAudioData: jest.fn(),
      onVideoData: jest.fn(),
      onTranscriptData: jest.fn(),
      onLeave: jest.fn(),
    })),

    // Global client singleton methods
    join: jest.fn().mockReturnValue(true),
    poll: jest.fn(),
    leave: jest.fn().mockReturnValue(true),
    release: jest.fn().mockReturnValue(true),
    uuid: jest.fn().mockReturnValue("global-uuid"),
    streamId: jest.fn().mockReturnValue("global-stream-id"),
    onJoinConfirm: jest.fn(),
    onSessionUpdate: jest.fn(),
    onUserUpdate: jest.fn(),
    onAudioData: jest.fn(),
    onVideoData: jest.fn(),
    onTranscriptData: jest.fn(),
    onLeave: jest.fn(),

    // Utility functions
    initialize: jest.fn().mockReturnValue(true),
    uninitialize: jest.fn().mockReturnValue(true),
    generateSignature: jest.fn().mockReturnValue("mock-signature"),
    isInitialized: jest.fn().mockReturnValue(true),

    // Constants (for completeness)
    MEDIA_TYPE_AUDIO: 1,
    MEDIA_TYPE_VIDEO: 2,
    MEDIA_TYPE_TRANSCRIPT: 4,
    MEDIA_TYPE_ALL: 7,
  };

  return mockFunctions;
});

describe('RTMS Node.JS Addon Unit Tests', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  // ---- Class-based approach tests ----
  describe('Class-based Client Approach', () => {
    let client;

    beforeEach(() => {
      client = new rtms.Client();
    });

    test('client.join with parameters joins a session correctly', () => {
      const result = client.join("uuid", "session_id", "signature", "server_url", 5000);
      expect(client.join).toHaveBeenCalledWith("uuid", "session_id", "signature", "server_url", 5000);
      expect(result).toBe(true);
    });

    test('client.join with options object joins a session correctly', () => {
      const options = {
        meeting_uuid: "uuid",
        rtms_stream_id: "session_id",
        server_urls: "server_url",
        signature: "signature",
        timeout: 5000,
        pollInterval: 100
      };
      
      const result = client.join(options);
      expect(client.join).toHaveBeenCalledWith(options);
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

    test('client.onAudioData sets the audio data callback correctly', () => {
      const callback = jest.fn();
      client.onAudioData(callback);
      expect(client.onAudioData).toHaveBeenCalledWith(callback);
    });

    test('client.onVideoData sets the video data callback correctly', () => {
      const callback = jest.fn();
      client.onVideoData(callback);
      expect(client.onVideoData).toHaveBeenCalledWith(callback);
    });

    test('client.onTranscriptData sets the transcript data callback correctly', () => {
      const callback = jest.fn();
      client.onTranscriptData(callback);
      expect(client.onTranscriptData).toHaveBeenCalledWith(callback);
    });

    test('client.onJoinConfirm sets the join confirmation callback correctly', () => {
      const callback = jest.fn();
      client.onJoinConfirm(callback);
      expect(client.onJoinConfirm).toHaveBeenCalledWith(callback);
    });

    test('client.onSessionUpdate sets the session update callback correctly', () => {
      const callback = jest.fn();
      client.onSessionUpdate(callback);
      expect(client.onSessionUpdate).toHaveBeenCalledWith(callback);
    });

    test('client.onUserUpdate sets the user update callback correctly', () => {
      const callback = jest.fn();
      client.onUserUpdate(callback);
      expect(client.onUserUpdate).toHaveBeenCalledWith(callback);
    });

    test('client.onLeave sets the leave callback correctly', () => {
      const callback = jest.fn();
      client.onLeave(callback);
      expect(client.onLeave).toHaveBeenCalledWith(callback);
    });

    test('client.release releases resources correctly', () => {
      const result = client.release();
      expect(client.release).toHaveBeenCalled();
      expect(result).toBe(true);
    });
  });

  // ---- Global singleton approach tests ----
  describe('Global Singleton Approach', () => {
    test('rtms.join with parameters joins a session correctly', () => {
      const result = rtms.join("uuid", "session_id", "signature", "server_url", 5000);
      expect(rtms.join).toHaveBeenCalledWith("uuid", "session_id", "signature", "server_url", 5000);
      expect(result).toBe(true);
    });

    test('rtms.join with options object joins a session correctly', () => {
      const options = {
        meeting_uuid: "uuid",
        rtms_stream_id: "session_id",
        server_urls: "server_url",
        signature: "signature",
        timeout: 5000,
        pollInterval: 100
      };
      
      const result = rtms.join(options);
      expect(rtms.join).toHaveBeenCalledWith(options);
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

    test('rtms.onTranscriptData sets the transcript data callback correctly', () => {
      const callback = jest.fn();
      rtms.onTranscriptData(callback);
      expect(rtms.onTranscriptData).toHaveBeenCalledWith(callback);
    });

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

    test('rtms.onLeave sets the leave callback correctly', () => {
      const callback = jest.fn();
      rtms.onLeave(callback);
      expect(rtms.onLeave).toHaveBeenCalledWith(callback);
    });

    test('rtms.leave releases resources correctly', () => {
      const result = rtms.leave();
      expect(rtms.leave).toHaveBeenCalled();
      expect(result).toBe(true);
    });
  });

  // ---- Utility function tests ----
  describe('Utility Functions', () => {
    test('rtms.initialize initializes the RTMS module', () => {
      rtms.initialize('/path/to/ca.pem');
      expect(rtms.initialize).toHaveBeenCalledWith('/path/to/ca.pem');
    });

    test('rtms.uninitialize uninitializes the RTMS module', () => {
      rtms.uninitialize();
      expect(rtms.uninitialize).toHaveBeenCalled();
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

  // ---- Usage pattern coexistence tests ----
  describe('Coexistence of both usage patterns', () => {
    let client;

    beforeEach(() => {
      client = new rtms.Client();
    });

    test('Both patterns can be used simultaneously', () => {
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

    test('Callbacks can be set on both patterns', () => {
      const globalCallback = jest.fn();
      const instanceCallback = jest.fn();
      
      rtms.onAudioData(globalCallback);
      client.onAudioData(instanceCallback);
      
      expect(rtms.onAudioData).toHaveBeenCalledWith(globalCallback);
      expect(client.onAudioData).toHaveBeenCalledWith(instanceCallback);
    });

    test('Each pattern has its own UUID and stream ID', () => {
      const globalUuid = rtms.uuid();
      const instanceUuid = client.uuid();
      
      const globalStreamId = rtms.streamId();
      const instanceStreamId = client.streamId();
      
      expect(globalUuid).toBe('global-uuid');
      expect(instanceUuid).toBe('client-uuid');
      
      expect(globalStreamId).toBe('global-stream-id');
      expect(instanceStreamId).toBe('client-stream-id');
    });
  });
});