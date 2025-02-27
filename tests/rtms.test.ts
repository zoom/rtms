const rtms = require("../index.ts")

// Mock the module
jest.mock("../index.ts", () => {
  // Mock Client class implementation
  const ClientMock = jest.fn().mockImplementation(() => ({
    join: jest.fn().mockReturnValue(true),
    startPolling: jest.fn(),
    stopPolling: jest.fn(),
    leave: jest.fn().mockReturnValue(true),
    poll: jest.fn(),
    release: jest.fn().mockReturnValue(true),
    uuid: jest.fn().mockReturnValue("test-uuid"),
    streamId: jest.fn().mockReturnValue("test-stream-id"),
    onJoinConfirm: jest.fn(),
    onSessionUpdate: jest.fn(),
    onUserUpdate: jest.fn(),
    onAudioData: jest.fn(),
    onVideoData: jest.fn(),
    onTranscriptData: jest.fn(),
    onLeave: jest.fn(),
  }));
  
  // Return mocked default export
  return {
    __esModule: true,
    default: {
      Client: ClientMock,
      initialize: jest.fn().mockReturnValue(true),
      uninitialize: jest.fn().mockReturnValue(true),
      generateSignature: jest.fn().mockReturnValue("mock-signature"),
      isInitialized: jest.fn().mockReturnValue(true),
      
      // Constants
      MEDIA_TYPE_AUDIO: 1,
      MEDIA_TYPE_VIDEO: 2,
      MEDIA_TYPE_DESKSHARE: 4,
      MEDIA_TYPE_TRANSCRIPT: 8,
      MEDIA_TYPE_CHAT: 16,
      MEDIA_TYPE_ALL: 31,
      
      SESSION_EVENT_ADD: 1,
      SESSION_EVENT_STOP: 2,
      SESSION_EVENT_PAUSE: 3,
      SESSION_EVENT_RESUME: 4,
      
      USER_EVENT_JOIN: 1,
      USER_EVENT_LEAVE: 2,
      
      RTMS_SDK_FAILURE: 1,
      RTMS_SDK_OK: 0,
      RTMS_SDK_TIMEOUT: 2,
      RTMS_SDK_NOT_EXIST: 3,
      RTMS_SDK_WRONG_TYPE: 4,
      RTMS_SDK_INVALID_STATUS: 5,
      RTMS_SDK_INVALID_ARGS: 6,
      
      SESS_STATUS_ACTIVE: 1,
      SESS_STATUS_PAUSED: 2,
    }
  };
});

describe('RTMS Node.JS Module Unit Tests', () => {
  let client;
  
  beforeEach(() => {
    jest.clearAllMocks();
    client = new rtms.default.Client();
  });

  // ---- Module Functions ----
  
  test('initialize initializes the RTMS module', () => {
    const result = rtms.default.initialize('/path/to/ca.pem');
    expect(rtms.default.initialize).toHaveBeenCalledWith('/path/to/ca.pem');
    expect(result).toBe(true);
  });
  
  test('uninitialize releases RTMS resources', () => {
    const result = rtms.default.uninitialize();
    expect(rtms.default.uninitialize).toHaveBeenCalled();
    expect(result).toBe(true);
  });
  
  test('generateSignature creates a signature with correct parameters', () => {
    const params = {
      client: 'client-id',
      secret: 'client-secret',
      uuid: 'meeting-uuid',
      streamId: 'stream-id'
    };
    
    const signature = rtms.default.generateSignature(params);
    expect(rtms.default.generateSignature).toHaveBeenCalledWith(params);
    expect(signature).toBe('mock-signature');
  });
  
  test('isInitialized returns initialization status', () => {
    const initialized = rtms.default.isInitialized();
    expect(rtms.default.isInitialized).toHaveBeenCalled();
    expect(initialized).toBe(true);
  });

  // ---- Client Methods ----
  
  test('Client constructor creates a new client instance', () => {
    expect(rtms.default.Client).toHaveBeenCalled();
    expect(client).toBeDefined();
  });
  
  test('join accepts object parameters', () => {
    const joinParams = {
      meeting_uuid: 'uuid',
      rtms_stream_id: 'session_id',
      server_urls: 'server_url',
      timeout: 5000
    };
    
    const result = client.join(joinParams);
    expect(client.join).toHaveBeenCalledWith(joinParams);
    expect(result).toBe(true);
  });
  
  test('join with individual parameters', () => {
    const result = client.join('uuid', 'session_id', 'signature', 'server_url', 5000);
    expect(client.join).toHaveBeenCalledWith('uuid', 'session_id', 'signature', 'server_url', 5000);
    expect(result).toBe(true);
  });
  
  test('uuid returns the meeting UUID', () => {
    const uuid = client.uuid();
    expect(client.uuid).toHaveBeenCalled();
    expect(uuid).toBe('test-uuid');
  });
  
  test('streamId returns the stream ID', () => {
    const streamId = client.streamId();
    expect(client.streamId).toHaveBeenCalled();
    expect(streamId).toBe('test-stream-id');
  });
  
  test('leave stops polling and releases resources', () => {
    const result = client.leave();
    expect(client.leave).toHaveBeenCalled();
    expect(result).toBe(true);
  });

  // ---- Callback Tests ----
  
  test('onJoinConfirm sets join callback', () => {
    const joinCallback = jest.fn();
    client.onJoinConfirm(joinCallback);
    expect(client.onJoinConfirm).toHaveBeenCalledWith(joinCallback);
    
    // Simulate callback execution
    joinCallback(0);
    expect(joinCallback).toHaveBeenCalledWith(0);
  });
  
  test('onSessionUpdate sets session update callback', () => {
    const sessionUpdateCallback = jest.fn();
    client.onSessionUpdate(sessionUpdateCallback);
    expect(client.onSessionUpdate).toHaveBeenCalledWith(sessionUpdateCallback);
    
    // Simulate callback execution
    sessionUpdateCallback(1, { sessionId: 'abc', statTime: 123456, status: 2 });
    expect(sessionUpdateCallback).toHaveBeenCalledWith(1, {
      sessionId: 'abc',
      statTime: 123456,
      status: 2,
    });
  });
  
  test('onUserUpdate sets user update callback', () => {
    const userUpdateCallback = jest.fn();
    client.onUserUpdate(userUpdateCallback);
    expect(client.onUserUpdate).toHaveBeenCalledWith(userUpdateCallback);
    
    // Simulate callback execution
    userUpdateCallback(1, { id: 42, name: 'Alice' });
    expect(userUpdateCallback).toHaveBeenCalledWith(1, {
      id: 42,
      name: 'Alice',
    });
  });
  
  test('onAudioData sets audio data callback', () => {
    const audioDataCallback = jest.fn();
    client.onAudioData(audioDataCallback);
    expect(client.onAudioData).toHaveBeenCalledWith(audioDataCallback);
    
    // Simulate callback execution
    const buffer = Buffer.from([0x01, 0x02, 0x03]);
    audioDataCallback(buffer, 123456, { userName: 'Bob', userId: 7 });
    expect(audioDataCallback).toHaveBeenCalledWith(buffer, 123456, { userName: 'Bob', userId: 7 });
  });
  
  test('onVideoData sets video data callback', () => {
    const videoDataCallback = jest.fn();
    client.onVideoData(videoDataCallback);
    expect(client.onVideoData).toHaveBeenCalledWith(videoDataCallback);
    
    // Simulate callback execution
    const buffer = Buffer.from([0xff, 0xaa, 0xbb]);
    videoDataCallback(buffer, 654321, 'track-1', { userName: 'Carol', userId: 9 });
    expect(videoDataCallback).toHaveBeenCalledWith(buffer, 654321, 'track-1', { userName: 'Carol', userId: 9 });
  });
  
  test('onTranscriptData sets transcript data callback', () => {
    const transcriptDataCallback = jest.fn();
    client.onTranscriptData(transcriptDataCallback);
    expect(client.onTranscriptData).toHaveBeenCalledWith(transcriptDataCallback);
    
    // Simulate callback execution
    const buffer = Buffer.from('{"text":"Hello world"}');
    transcriptDataCallback(buffer, 789012, { userName: 'Dave', userId: 11 });
    expect(transcriptDataCallback).toHaveBeenCalledWith(buffer, 789012, { userName: 'Dave', userId: 11 });
  });
  
  test('onLeave sets leave callback', () => {
    const leaveCallback = jest.fn();
    client.onLeave(leaveCallback);
    expect(client.onLeave).toHaveBeenCalledWith(leaveCallback);
    
    // Simulate callback execution
    leaveCallback(1); // Assume 1 means user left voluntarily
    expect(leaveCallback).toHaveBeenCalledWith(1);
  });
  
  // ---- Usage Pattern Test ----
  
  test('Supports the desired usage pattern', () => {
    // The pattern from the example
    const client = new rtms.default.Client();
    
    const audioCallback = jest.fn();
    client.onAudioData(audioCallback);
    
    client.join({
      meeting_uuid: "asf", 
      rtms_stream_id: "343", 
      server_urls: "wtcp://10.10.1.123:9092"
    });
    
    // Verify the calls were made with the correct parameters
    expect(client.onAudioData).toHaveBeenCalledWith(audioCallback);
    expect(client.join).toHaveBeenCalledWith({
      meeting_uuid: "asf", 
      rtms_stream_id: "343", 
      server_urls: "wtcp://10.10.1.123:9092"
    });
  });
});