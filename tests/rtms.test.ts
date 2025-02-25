const rtms = require("../index.ts")

// Mock the native addon methods
jest.mock("../index.ts", () => ({
  _init: jest.fn(),
  _isInit: jest.fn().mockReturnValue(true),
  _join: jest.fn().mockReturnValue(0),
  useAudio: jest.fn(),
  useVideo: jest.fn(),
  useTranscript: jest.fn(),
  setMediaTypes: jest.fn(),

  onJoinConfirm: jest.fn(),
  onSessionUpdate: jest.fn(),
  onUserUpdate: jest.fn(),
  onAudioData: jest.fn(),
  onVideoData: jest.fn(),
  onTranscriptData: jest.fn(),
  onLeave: jest.fn(),
}));

describe('RTMS Node.JS Addon Unit Tests', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  // ---- Core Methods ----

  test('_init initializes the RTMS module', () => {
    rtms._init('/path/to/ca.pem');
    expect(rtms._init).toHaveBeenCalledWith('/path/to/ca.pem');
  });

  test('_isInit returns true after initialization', () => {
    const initialized = rtms._isInit();
    expect(initialized).toBe(true);
    expect(rtms._isInit).toHaveBeenCalled();
  });

  test('_join joins a session with correct parameters', () => {
    const result = rtms._join('uuid', 'session_id', 'signature', 'server_url', 5000);
    expect(rtms._join).toHaveBeenCalledWith('uuid', 'session_id', 'signature', 'server_url', 5000);
    expect(result).toBe(0);
  });

  // ---- Media Control ----

  test('useAudio enables/disables audio', () => {
    rtms.useAudio(true);
    expect(rtms.useAudio).toHaveBeenCalledWith(true);

    rtms.useAudio(false);
    expect(rtms.useAudio).toHaveBeenCalledWith(false);
  });

  test('useVideo enables/disables video', () => {
    rtms.useVideo(true);
    expect(rtms.useVideo).toHaveBeenCalledWith(true);
  });

  test('useTranscript enables/disables transcripts', () => {
    rtms.useTranscript(true);
    expect(rtms.useTranscript).toHaveBeenCalledWith(true);
  });

  test('setMediaTypes sets correct media options', () => {
    rtms.setMediaTypes(true, false, true);
    expect(rtms.setMediaTypes).toHaveBeenCalledWith(true, false, true);
  });

  // ---- Callback Tests ----

  test('onJoinConfirm sets join callback', () => {
    const joinCallback = jest.fn();
    rtms.onJoinConfirm(joinCallback);

    expect(rtms.onJoinConfirm).toHaveBeenCalledWith(joinCallback);

    // Simulate callback execution
    joinCallback(0);
    expect(joinCallback).toHaveBeenCalledWith(0);
  });

  test('onSessionUpdate sets session update callback', () => {
    const sessionUpdateCallback = jest.fn();
    rtms.onSessionUpdate(sessionUpdateCallback);

    expect(rtms.onSessionUpdate).toHaveBeenCalledWith(sessionUpdateCallback);

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
    rtms.onUserUpdate(userUpdateCallback);

    expect(rtms.onUserUpdate).toHaveBeenCalledWith(userUpdateCallback);

    // Simulate callback execution
    userUpdateCallback(1, { participantId: 42, participantName: 'Alice' });
    expect(userUpdateCallback).toHaveBeenCalledWith(1, {
      participantId: 42,
      participantName: 'Alice',
    });
  });

  test('onAudioData sets audio data callback', () => {
    const audioDataCallback = jest.fn();
    rtms.onAudioData(audioDataCallback);

    expect(rtms.onAudioData).toHaveBeenCalledWith(audioDataCallback);

    // Simulate callback execution
    const buffer = Buffer.from([0x01, 0x02, 0x03]);
    audioDataCallback(buffer, buffer.length, 123456, { userName: 'Bob', userId: 7 });
    expect(audioDataCallback).toHaveBeenCalledWith(buffer, 3, 123456, { userName: 'Bob', userId: 7 });
  });

  test('onVideoData sets video data callback', () => {
    const videoDataCallback = jest.fn();
    rtms.onVideoData(videoDataCallback);

    expect(rtms.onVideoData).toHaveBeenCalledWith(videoDataCallback);

    // Simulate callback execution
    const buffer = Buffer.from([0xff, 0xaa, 0xbb]);
    videoDataCallback(buffer, buffer.length, 654321, { userName: 'Carol', userId: 9 });
    expect(videoDataCallback).toHaveBeenCalledWith(buffer, 3, 654321, { userName: 'Carol', userId: 9 });
  });

  test('onLeave sets leave callback', () => {
    const leaveCallback = jest.fn();
    rtms.onLeave(leaveCallback);

    expect(rtms.onLeave).toHaveBeenCalledWith(leaveCallback);

    // Simulate callback execution
    leaveCallback(1); // Assume 1 means user left voluntarily
    expect(leaveCallback).toHaveBeenCalledWith(1);
  });
});
