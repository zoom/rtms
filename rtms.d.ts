/**
 * Zoom Realtime Media Streams (RTMS) SDK
 *
 * The RTMS SDK provides access to real-time audio, video, and transcript data
 * from Zoom meetings. Use the Client class to connect to meetings and receive
 * real-time media streams.
 *
 * @packageDocumentation
 * @module rtms
 */

/**
 * Available log levels for RTMS SDK logging
 * 
 * @category Common
 */
export enum LogLevel {
  /** Error messages only */
  ERROR = 0,
  /** Error and warning messages */
  WARN = 1,
  /** Error, warning, and informational messages (default) */
  INFO = 2,
  /** All messages including debug information */
  DEBUG = 3,
  /** All messages including detailed trace information */
  TRACE = 4
}

/**
 * Available log output formats
 * 
 * @category Common
 */
export enum LogFormat {
  /** Human-readable progressive format for console output */
  PROGRESSIVE = 'progressive',
  /** Machine-readable JSON format for log processing */
  JSON = 'json'
}

/**
 * Configuration options for the RTMS logger
 * 
 * @category Common
 */
export interface LoggerConfig {
  /** The minimum log level to display */
  level: LogLevel;
  /** The format for log output */
  format: LogFormat;
  /** Whether logging is enabled */
  enabled: boolean;
}

/**
 * Media type flags that can be combined with bitwise OR (|)
 * 
 * @example
 * // Enable both audio and video, but not transcript
 * const mediaTypes = rtms.MEDIA_TYPE_AUDIO | rtms.MEDIA_TYPE_VIDEO;
 * 
 * @category Constants
 */
/** Flag to enable audio stream processing */
export const MEDIA_TYPE_AUDIO: number;
/** Flag to enable video stream processing */
export const MEDIA_TYPE_VIDEO: number;
/** Flag to enable desktop sharing stream processing */
export const MEDIA_TYPE_DESKSHARE: number;
/** Flag to enable transcript stream processing */
export const MEDIA_TYPE_TRANSCRIPT: number;
/** Flag to enable chat message processing */
export const MEDIA_TYPE_CHAT: number;
/** Flag to enable all supported media types */
export const MEDIA_TYPE_ALL: number;

/**
 * Session event constants
 * 
 * @category Constants
 */
/** Event constant indicating a session has been added */
export const SESSION_EVENT_ADD: number;
/** Event constant indicating a session has been stopped */
export const SESSION_EVENT_STOP: number;
/** Event constant indicating a session has been paused */
export const SESSION_EVENT_PAUSE: number;
/** Event constant indicating a session has been resumed */
export const SESSION_EVENT_RESUME: number;

/**
 * User event constants (for onUserUpdate callback)
 *
 * @category Constants
 */
/** Event constant indicating a user has joined */
export const USER_JOIN: number;
/** Event constant indicating a user has left */
export const USER_LEAVE: number;

/**
 * Event types for subscribeEvent/unsubscribeEvent
 * Used with onEventEx callback or typed event callbacks
 * These match RTMS_EVENT_TYPE from Zoom's C SDK
 *
 * @category Constants
 */
/** Event constant for undefined events */
export const EVENT_UNDEFINED: number;
/** Event constant for first packet timestamp */
export const EVENT_FIRST_PACKET_TIMESTAMP: number;
/** Event constant for active speaker changes */
export const EVENT_ACTIVE_SPEAKER_CHANGE: number;
/** Event constant for participant join */
export const EVENT_PARTICIPANT_JOIN: number;
/** Event constant for participant leave */
export const EVENT_PARTICIPANT_LEAVE: number;
/** Event constant for screen sharing start */
export const EVENT_SHARING_START: number;
/** Event constant for screen sharing stop */
export const EVENT_SHARING_STOP: number;
/** Event constant for media connection interrupted */
export const EVENT_MEDIA_CONNECTION_INTERRUPTED: number;
/** Event constant for consumer answered (phone calls) */
export const EVENT_CONSUMER_ANSWERED: number;
/** Event constant for consumer end (phone calls) */
export const EVENT_CONSUMER_END: number;
/** Event constant for user answered (phone calls) */
export const EVENT_USER_ANSWERED: number;
/** Event constant for user end (phone calls) */
export const EVENT_USER_END: number;
/** Event constant for user hold (phone calls) */
export const EVENT_USER_HOLD: number;
/** Event constant for user unhold (phone calls) */
export const EVENT_USER_UNHOLD: number;

/**
 * SDK status codes
 * 
 * @category Constants
 */
/** Status code indicating an SDK operation failed */
export const RTMS_SDK_FAILURE: number;
/** Status code indicating an SDK operation succeeded */
export const RTMS_SDK_OK: number;
/** Status code indicating an SDK operation timed out */
export const RTMS_SDK_TIMEOUT: number;
/** Status code indicating a requested resource does not exist */
export const RTMS_SDK_NOT_EXIST: number;
/** Status code indicating an incorrect type was provided */
export const RTMS_SDK_WRONG_TYPE: number;
/** Status code indicating an invalid status */
export const RTMS_SDK_INVALID_STATUS: number;
/** Status code indicating invalid arguments were provided */
export const RTMS_SDK_INVALID_ARGS: number;

/**
 * Session status constants
 * 
 * @category Constants
 */
/** Session status indicating the session is active */
export const SESS_STATUS_ACTIVE: number;
/** Session status indicating the session is paused */
export const SESS_STATUS_PAUSED: number;

//-----------------------------------------------------------------------------------
// Data type interfaces
//-----------------------------------------------------------------------------------

/**
 * Configuration options for media stream types to receive
 * 
 * @category Common Interfaces
 */
export interface MediaTypes {
  /** Whether to receive audio streams */
  audio: boolean;
  /** Whether to receive video streams */
  video?: boolean;
  /** Whether to receive transcript data */
  transcript?: boolean;
}

/**
 * Metadata information about a participant in a Zoom meeting
 * 
 * @category Data Interfaces
 */
export interface Metadata {
  /** The display name of the Zoom participant */
  userName: string;
  /** The user ID of the Zoom participant */
  userId: number;
}

/**
 * Information about a participant in a Zoom meeting
 * 
 * @category Data Interfaces
 */
export interface ParticipantInfo {
  /** The unique identifier for this participant */
  id: number;
  /** The display name of this participant */
  name: string;
}

/**
 * Information about a Zoom meeting session
 * 
 * @category Data Interfaces
 */
export interface SessionInfo {
  /** The unique identifier for this session */
  sessionId: string;
  /** The stream ID for this session */
  streamId: string;
  /** The meeting ID for this session */
  meetingId: string;
  /** The start time of this session (Unix timestamp) */
  statTime: number;
  /** The current status of this session (SESS_STATUS_*) */
  status: number;
  /** Whether this session is currently active */
  isActive: boolean;
  /** Whether this session is currently paused */
  isPaused: boolean;
}

/**
 * Configuration parameters for audio streams with sensible defaults
 *
 * Default values (work out-of-box for per-participant audio):
 * - contentType: RAW_AUDIO (2)
 * - codec: OPUS (4)
 * - sampleRate: SR_48K (3)
 * - channel: STEREO (2)
 * - dataOpt: AUDIO_MULTI_STREAMS (2) - enables userId in audio metadata
 * - duration: 20 ms
 * - frameSize: 960 samples (48kHz × 20ms)
 *
 * Users can omit setAudioParams() entirely or override individual fields.
 *
 * @category Media Configuration
 */
export interface AudioParams {
  /** The type of audio content (default: RAW_AUDIO = 2) */
  contentType?: number;
  /** The audio codec to use (default: OPUS = 4) */
  codec?: number;
  /** The sample rate SR_8K = 0, SR_16K = 1, SR_32K = 2, SR_48K = 3 (default: SR_48K = 3) */
  sampleRate?: number;
  /** The number of audio channels (1=mono, 2=stereo) (default: STEREO = 2) */
  channel?: number;
  /** Additional data options for audio processing (default: AUDIO_MULTI_STREAMS = 2) */
  dataOpt?: number;
  /** The duration of each audio frame in milliseconds (default: 20 ms) */
  duration?: number;
  /** The size of each audio frame in samples (default: 960 samples) */
  frameSize?: number;
}

/**
 * Configuration parameters for video streams
 * 
 * @category Media Configuration
 */
export interface VideoParams {
  /** The type of video content */
  contentType?: number;
  /** The video codec to use */
  codec?: number;
  /** The video resolution */
  resolution?: number;
  /** Additional data options for video processing */
  dataOpt?: number;
  /** The video frame rate (frames per second) */
  fps?: number;
}

/**
 * Configuration parameters for video streams
 * 
 * @category Media Configuration
 */
export interface DeskshareParams {
  /** The type of deskshare content */
  contentType?: number;
  /** The deskshare codec to use */
  codec?: number;
  /** The deskshare resolution */
  resolution?: number;
  /** The video frame rate (frames per second) */
  fps?: number;
}

/**
 * Configuration parameters for transcript streams
 *
 * @category Media Configuration
 */
export interface TranscriptParams {
  /** Source language ID. Use TranscriptLanguage constants. Default: NONE (auto-detect after 30s) */
  srcLanguage?: number;
  /** Enable Language Identification. Default: true */
  enableLid?: boolean;
}

/**
 * Language ID constants for use with TranscriptParams.srcLanguage
 *
 * @example
 * ```typescript
 * const params = new rtms.TranscriptParams();
 * params.setSrcLanguage(rtms.TranscriptLanguage.ENGLISH);
 * ```
 *
 * @category Constants
 */
export const TranscriptLanguage: {
  readonly NONE: number;
  readonly ARABIC: number;
  readonly BENGALI: number;
  readonly CANTONESE: number;
  readonly CATALAN: number;
  readonly CHINESE_SIMPLIFIED: number;
  readonly CHINESE_TRADITIONAL: number;
  readonly CZECH: number;
  readonly DANISH: number;
  readonly DUTCH: number;
  readonly ENGLISH: number;
  readonly ESTONIAN: number;
  readonly FINNISH: number;
  readonly FRENCH_CANADA: number;
  readonly FRENCH_FRANCE: number;
  readonly GERMAN: number;
  readonly HEBREW: number;
  readonly HINDI: number;
  readonly HUNGARIAN: number;
  readonly INDONESIAN: number;
  readonly ITALIAN: number;
  readonly JAPANESE: number;
  readonly KOREAN: number;
  readonly MALAY: number;
  readonly PERSIAN: number;
  readonly POLISH: number;
  readonly PORTUGUESE: number;
  readonly ROMANIAN: number;
  readonly RUSSIAN: number;
  readonly SPANISH: number;
  readonly SWEDISH: number;
  readonly TAGALOG: number;
  readonly TAMIL: number;
  readonly TELUGU: number;
  readonly THAI: number;
  readonly TURKISH: number;
  readonly UKRAINIAN: number;
  readonly VIETNAMESE: number;
};


//-----------------------------------------------------------------------------------
// Parameter interfaces
//-----------------------------------------------------------------------------------

/**
 * Parameters for joining a Zoom RTMS session
 *
 * For Meeting SDK events (meeting.rtms_started), use meeting_uuid.
 * For Webinar events (webinar.rtms_started), use webinar_uuid.
 * For Video SDK events (session.rtms_started), use session_id.
 * Priority: meeting_uuid > webinar_uuid > session_id.
 *
 * @category Common Interfaces
 */
export interface JoinParams {
  /** The UUID of the Zoom meeting (for Meeting SDK events) */
  meeting_uuid?: string;
  /** The UUID of the Zoom webinar (for Webinar events) */
  webinar_uuid?: string;
  /** The session ID (for Video SDK events) - used when meeting_uuid is not provided */
  session_id?: string;
  /** The engagement ID (for ZCC events) - used when meeting_uuid is not provided */
  engagement_id?: string;
  /** The RTMS stream ID for this connection */
  rtms_stream_id: string;
  /** The server URL(s) to connect to */
  server_urls: string;
  /** The authentication signature (optional if client and secret are provided) */
  signature?: string;
  /** The Zoom OAuth client ID (defaults to ZM_RTMS_CLIENT environment variable) */
  client?: string;
  /** The Zoom OAuth client secret (defaults to ZM_RTMS_SECRET environment variable) */
  secret?: string;
  /** The path to a CA certificate file (defaults to system CA) */
  ca?: string;
  /** The timeout for the join operation in milliseconds */
  timeout?: number;
  /** The interval between poll operations in milliseconds */
  pollInterval?: number;
  /** Whether to verify TLS certificates (1 = verify, 0 = don't verify, defaults to 1) */
  is_verify_cert?: number;
  /** User agent string to send in requests */
  agent?: string;
}

/**
 * Parameters for generating an authentication signature
 *
 * @category Common Interfaces
 */
export interface SignatureParams {
  /** The Zoom OAuth client ID */
  client: string;
  /** The Zoom OAuth client secret */
  secret: string;
  /** The identifier: meeting_uuid for Meeting SDK, session_id for Video SDK */
  uuid: string;
  /** The RTMS stream ID for this connection */
  streamId: string;
}

//-----------------------------------------------------------------------------------
// Callback types
//-----------------------------------------------------------------------------------

/**
 * Callback function for processing webhook events
 * 
 * @param payload The JSON payload of the webhook event
 * 
 * @category Callback Types
 */
export type WebhookCallback = (payload: Record<string, any>) => void;

/**
 * Enhanced callback function for processing webhook events with raw HTTP access
 * 
 * This callback provides access to the raw HTTP request and response objects,
 * allowing developers to handle webhook validation challenges and customize
 * responses as needed.
 * 
 * @param payload The JSON payload of the webhook event
 * @param req The raw HTTP request object
 * @param res The raw HTTP response object
 * 
 * @category Callback Types
 */
export type RawWebhookCallback = (
  payload: Record<string, any>,
  req: import('http').IncomingMessage,
  res: import('http').ServerResponse
) => void;

/**
 * Callback function for when a join operation is confirmed
 * 
 * @param reason The reason code for the join confirmation
 * 
 * @category Callback Types
 */
export type JoinConfirmCallback = (reason: number) => void;

/**
 * Callback function for session update events
 * 
 * @param op The operation type (SESSION_EVENT_*)
 * @param sessionInfo Information about the updated session
 * 
 * @category Callback Types
 */
export type SessionUpdateCallback = (op: number, sessionInfo: SessionInfo) => void;

/**
 * Callback function for user update events (participant join/leave at SDK level)
 *
 * @param op The operation type (USER_JOIN or USER_LEAVE constant)
 * @param participant Information about the participant
 *
 * @category Callback Types
 */
export type UserUpdateCallback = (op: number, participant: ParticipantInfo) => void;

/**
 * Participant info for event callbacks
 *
 * @category Callback Types
 */
export interface EventParticipantInfo {
  userId: number;
  userName?: string;
}

/**
 * Callback function for participant join/leave events
 *
 * @param event The event type ('join' or 'leave')
 * @param timestamp Unix timestamp in milliseconds
 * @param participants Array of participant info objects
 *
 * @category Callback Types
 */
export type ParticipantEventCallback = (
  event: 'join' | 'leave',
  timestamp: number,
  participants: EventParticipantInfo[]
) => void;

/**
 * Callback function for active speaker change events
 *
 * @param timestamp Unix timestamp in milliseconds
 * @param userId The user ID of the active speaker
 * @param userName The display name of the active speaker
 *
 * @category Callback Types
 */
export type ActiveSpeakerEventCallback = (
  timestamp: number,
  userId: number,
  userName: string
) => void;

/**
 * Callback function for sharing start/stop events
 *
 * @param event The event type ('start' or 'stop')
 * @param timestamp Unix timestamp in milliseconds
 * @param userId The user ID (only for 'start' events)
 * @param userName The display name (only for 'start' events)
 *
 * @category Callback Types
 */
export type SharingEventCallback = (
  event: 'start' | 'stop',
  timestamp: number,
  userId?: number,
  userName?: string
) => void;

/**
 * Callback function for receiving deskshare data
 * 
 * @param buffer The raw deskshare data buffer
 * @param size The size of the deskshare data in bytes
 * @param timestamp The timestamp of the deskshare data
 * @param metadata Metadata about the participant who sent the audio
 * 
 * @category Callback Types
 */
export type DeskshareCallback = (buffer: Buffer, size: number, timestamp: number, metadata: Metadata) => void;

/**
 * Callback function for receiving audio data
 * 
 * @param buffer The raw audio data buffer
 * @param size The size of the audio data in bytes
 * @param timestamp The timestamp of the audio data
 * @param metadata Metadata about the participant who sent the audio
 * 
 * @category Callback Types
 */
export type AudioDataCallback = (buffer: Buffer, size: number, timestamp: number, metadata: Metadata) => void;

/**
 * Callback function for receiving video data
 * 
 * @param buffer The raw video data buffer
 * @param size The size of the video data in bytes
 * @param timestamp The timestamp of the video data
 * @param metadata Metadata about the participant who sent the video
 * 
 * @category Callback Types
 */
export type VideoDataCallback = (buffer: Buffer, size: number, timestamp: number, metadata: Metadata) => void;

/**
 * Callback function for receiving transcript data
 * 
 * @param buffer The raw transcript data buffer
 * @param size The size of the transcript data in bytes
 * @param timestamp The timestamp of the transcript data
 * @param metadata Metadata about the participant who sent the transcript
 * 
 * @category Callback Types
 */
export type TranscriptDataCallback = (buffer: Buffer, size: number, timestamp: number, metadata: Metadata) => void;

/**
 * Callback function for leave events
 * 
 * @param reason The reason code for the leave event
 * 
 * @category Callback Types
 */
export type LeaveCallback = (reason: number) => void;

/**
 * Callback function for extended event data
 * 
 * @param eventData The extended event data as a string
 * 
 * @category Callback Types
 */
export type EventExCallback = (eventData: string) => void;

//-----------------------------------------------------------------------------------
// Client class
//-----------------------------------------------------------------------------------

/**
 * RTMS Client: Core interface for connecting to Zoom real-time media streams
 * 
 * The Client class provides the main interface for connecting to and processing
 * Zoom RTMS streams. Use this approach when you need to:
 * - Connect to multiple meetings simultaneously
 * - Process different media types (audio, video, transcript)
 * - Handle session and user events
 * 
 * @example
 * ```typescript
 * import rtms from "@zoom/rtms";
 * 
 * const client = new rtms.Client();
 * 
 * // Set up event handlers
 * client.onJoinConfirm((reason) => {
 *   console.log(`Join confirmed with reason: ${reason}`);
 * });
 * 
 * client.onAudioData((buffer, size, timestamp, metadata) => {
 *   console.log(`Received ${size} bytes of audio from ${metadata.userName}`);
 *   // Process audio data...
 * });
 * 
 * // Join the meeting
 * client.join({
 *   meeting_uuid: "abc123-meeting-uuid",
 *   rtms_stream_id: "xyz789-stream-id",
 *   server_urls: "wss://rtms.zoom.us",
 *   pollInterval: 10 // milliseconds
 * });
 * 
 * // Later, leave the meeting
 * client.leave();
 * ```
 * 
 * @category Client Instance
 */
export class Client {
  /**
   * Creates a new RTMS Client instance
   * 
   * Each Client instance represents a connection to a single Zoom meeting.
   * You can create multiple Client instances to connect to different meetings.
   */
  constructor();
  
  /**
   * Initializes the RTMS SDK with the specified CA certificate path
   * 
   * This static method must be called before creating any Client instances.
   * It's automatically called by the join method if not already initialized.
   * 
   * @param caPath Path to the CA certificate file (defaults to system CA)
   * @returns true if initialization succeeds
   */
  static initialize(caPath?: string): boolean;
  
  /**
   * Uninitializes the RTMS SDK and releases resources
   * 
   * This static method should be called when you're done using the SDK.
   * It releases all system resources allocated by the SDK.
   * 
   * @returns true if uninitialization succeeds
   */
  static uninitialize(): boolean;
  
  /**
   * Joins a Zoom RTMS session with parameters object
   * 
   * This method establishes a connection to a Zoom RTMS stream.
   * After joining, callback methods will be invoked as events occur.
   * 
   * @param options An object containing join parameters
   * @returns true if the join operation succeeds
   * 
   * @example
   * ```typescript
   * client.join({
   *   meeting_uuid: "abc123-meeting-uuid",
   *   rtms_stream_id: "xyz789-stream-id",
   *   server_urls: "wss://rtms.zoom.us",
   *   pollInterval: 10
   * });
   * ```
   */
  join(options: JoinParams): boolean;
  
  /**
   * Joins a Zoom RTMS session with individual parameters
   * 
   * This method establishes a connection to a Zoom RTMS stream.
   * After joining, callback methods will be invoked as events occur.
   * 
   * @param meetingUuid The UUID of the Zoom meeting
   * @param rtmsStreamId The RTMS stream ID for this connection
   * @param signature The authentication signature
   * @param serverUrls The server URL(s) to connect to
   * @param timeout The timeout for the join operation in milliseconds
   * @returns true if the join operation succeeds
   * 
   * @example
   * ```typescript
   * // Generate signature
   * const signature = rtms.generateSignature({
   *   client: "client_id",
   *   secret: "client_secret",
   *   uuid: "abc123-meeting-uuid",
   *   streamId: "xyz789-stream-id"
   * });
   * 
   * // Join with explicit parameters
   * client.join(
   *   "abc123-meeting-uuid",
   *   "xyz789-stream-id",
   *   signature,
   *   "wss://rtms.zoom.us"
   * );
   * ```
   */
  join(meetingUuid: string, rtmsStreamId: string, signature: string, serverUrls: string, timeout?: number): boolean;
  
  /**
   * Manually polls for events from the RTMS server
   * 
   * This method is automatically called by the SDK's internal polling
   * mechanism. You typically don't need to call this manually.
   * 
   * @returns true if the poll operation succeeds
   */
  poll(): boolean;
  
  /**
   * Releases client resources
   * 
   * This method disconnects from the server and releases resources.
   * It's automatically called by the leave method.
   * 
   * @returns true if the release operation succeeds
   */
  release(): boolean;
  
  /**
   * Leaves the current session and releases resources
   * 
   * This method disconnects from the Zoom RTMS stream,
   * stops background polling, and releases resources.
   * 
   * @returns true if the leave operation succeeds
   * 
   * @example
   * ```typescript
   * // Leave the meeting when done
   * client.leave();
   * ```
   */
  leave(): boolean;
  
  /**
   * Gets the UUID of the current meeting
   * 
   * @returns The meeting UUID
   */
  uuid(): string;
  
  /**
   * Gets the stream ID of the current connection
   * 
   * @returns The RTMS stream ID
   */
  streamId(): string;
  
  /**
   * Sets audio parameters for the client (OPTIONAL)
   *
   * This method configures audio processing parameters. AudioParams now has
   * sensible defaults, so calling this method is optional. Defaults enable
   * per-participant audio with userId in metadata (dataOpt=AUDIO_MULTI_STREAMS).
   *
   * You can override individual fields without setting all parameters:
   * @example
   * ```typescript
   * // Option 1: Use defaults (no call needed)
   * await client.join(...);
   *
   * // Option 2: Override just one field
   * client.setAudioParams({ channel: rtms.AudioChannel.MONO });
   * ```
   *
   * @param params Audio parameters configuration (merges with defaults)
   * @returns true if the operation succeeds
   * @throws Error if parameters are invalid or incompatible
   */
  setAudioParams(params: AudioParams): boolean;
  
  /**
   * Sets video parameters for the client
   * 
   * This method configures video processing parameters.
   * 
   * @param params Video parameters configuration
   * @returns true if the operation succeeds
   */
  setVideoParams(params: VideoParams): boolean;


    /**
   * Sets deskshare parameters for the client
   * 
   * This method configures deskshare video processing parameters.
   * 
   * @param params Deskshare parameter configuration
   * @returns true if the operation succeeds
   */
    setDeskshareParams(params: DeskshareParams): boolean;

  /**
   * Configures a proxy for SDK connections.
   *
   * @param proxy_type Proxy protocol type (e.g. `'http'`, `'https'`)
   * @param proxy_url Full proxy URL including host and port
   * @returns true if the operation succeeds
   */
  setProxy(proxy_type: string, proxy_url: string): boolean;

  /**
   * Subscribe or unsubscribe from an individual participant's video stream.
   *
   * @param userId The participant's user ID
   * @param subscribe true to subscribe, false to unsubscribe
   * @returns true if the operation succeeds
   */
  subscribeVideo(userId: number, subscribe: boolean): boolean;

  /**
   * Sets a callback fired when participants' video state changes.
   *
   * @param callback Invoked with an array of user IDs and a boolean indicating video on/off
   * @returns true if registration succeeds
   */
  onParticipantVideo(callback: (users: number[], isOn: boolean) => void): boolean;

  /**
   * Sets a callback fired in response to a `subscribeVideo` call.
   *
   * @param callback Invoked with userId, status code, and an error string (empty on success)
   * @returns true if registration succeeds
   */
  onVideoSubscribed(callback: (userId: number, status: number, error: string) => void): boolean;

  /**
   * Sets a callback for join confirmation events
   * 
   * This callback is triggered when the join operation is confirmed by the server.
   * 
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   * 
   * @example
   * ```typescript
   * client.onJoinConfirm((reason) => {
   *   console.log(`Join confirmed with reason code: ${reason}`);
   *   // 0 = success, other values indicate specific error conditions
   * });
   * ```
   */
  onJoinConfirm(callback: JoinConfirmCallback): boolean;
  
  /**
   * Sets a callback for session update events
   * 
   * This callback is triggered when session information is updated.
   * It provides details about session status changes (add, stop, pause, resume).
   * 
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   * 
   * @example
   * ```typescript
   * client.onSessionUpdate((op, sessionInfo) => {
   *   // op = SESSION_EVENT_ADD, SESSION_EVENT_STOP, etc.
   *   console.log(`Session ${sessionInfo.sessionId} updated: ${op}`);
   *   console.log(`Status: ${sessionInfo.isActive ? 'active' : 'inactive'}`);
   * });
   * ```
   */
  onSessionUpdate(callback: SessionUpdateCallback): boolean;

  /**
   * Sets a callback for raw SDK user update events (participant join/leave)
   *
   * This is the low-level SDK callback. For most use cases, prefer onParticipantEvent
   * which provides a more convenient API with automatic event subscription.
   *
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   *
   * @example
   * ```typescript
   * client.onUserUpdate((op, participant) => {
   *   console.log(`User ${participant.name} (${participant.id}) - op: ${op}`);
   * });
   * ```
   */
  onUserUpdate(callback: UserUpdateCallback): boolean;

  /**
   * Sets a callback for participant join/leave events
   *
   * This automatically subscribes to EVENT_PARTICIPANT_JOIN and EVENT_PARTICIPANT_LEAVE.
   * Events are delivered as parsed objects, not raw JSON.
   *
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   *
   * @example
   * ```typescript
   * client.onParticipantEvent((event, timestamp, participants) => {
   *   if (event === 'join') {
   *     participants.forEach(p => console.log(`User joined: ${p.userName} (${p.userId})`));
   *   } else {
   *     participants.forEach(p => console.log(`User left: ${p.userId}`));
   *   }
   * });
   * ```
   */
  onParticipantEvent(callback: ParticipantEventCallback): boolean;

  /**
   * Sets a callback for active speaker change events
   *
   * This automatically subscribes to EVENT_ACTIVE_SPEAKER_CHANGE.
   *
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   *
   * @example
   * ```typescript
   * client.onActiveSpeakerEvent((timestamp, userId, userName) => {
   *   console.log(`Active speaker: ${userName} (${userId})`);
   * });
   * ```
   */
  onActiveSpeakerEvent(callback: ActiveSpeakerEventCallback): boolean;

  /**
   * Sets a callback for sharing start/stop events
   *
   * This automatically subscribes to EVENT_SHARING_START and EVENT_SHARING_STOP.
   * Note: These events only work when the RTMS app has DESKSHARE scope permission.
   *
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   *
   * @example
   * ```typescript
   * client.onSharingEvent((event, timestamp, userId, userName) => {
   *   if (event === 'start') {
   *     console.log(`${userName} started sharing`);
   *   } else {
   *     console.log('Sharing stopped');
   *   }
   * });
   * ```
   */
  onSharingEvent(callback: SharingEventCallback): boolean;

  /**
   * Sets a callback for media connection interrupted events
   *
   * This automatically subscribes to EVENT_MEDIA_CONNECTION_INTERRUPTED.
   *
   * @param callback The callback function to invoke with the event timestamp
   * @returns true if the callback was set successfully
   *
   * @example
   * ```typescript
   * client.onMediaConnectionInterrupted((timestamp) => {
   *   console.log(`Media connection interrupted at ${timestamp}`);
   * });
   * ```
   */
  onMediaConnectionInterrupted(callback: (timestamp: number) => void): boolean;

  /**
   * Sets a callback for raw event data
   *
   * This provides access to the raw JSON event data from the SDK.
   * Use this when you need custom event handling or access to all event types.
   * This callback is called IN ADDITION to typed callbacks, not instead of.
   *
   * @param callback The callback function to invoke with raw JSON string
   * @returns true if the callback was set successfully
   *
   * @example
   * ```typescript
   * client.onEventEx((eventData) => {
   *   const data = JSON.parse(eventData);
   *   console.log('Raw event:', data);
   * });
   * ```
   */
  onEventEx(callback: EventExCallback): boolean;

  /**
   * Sets a callback for receiving deskshare data
   * 
   * This callback is triggered when data is received from the meeting.
   * It provides the raw audio data buffer and metadata about the sender.
   * 
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   * 
   * @example
   * ```typescript
   * client.onDsData((buffer, size, timestamp, metadata) => {
   *   console.log(`Received ${size} bytes of deskshare data from ${metadata.userName}`);
   *   
   *   // Process the data
   *   // buffer - Raw deskshare data (Buffer)
   *   // size - Size of the deskshare data in bytes
   *   // timestamp - Timestamp of the deskshare data
   *   // metadata - Information about the sender
   * });
   * ```
   */
  onDeskshareData(callback: DeskshareDataCallback): boolean;
  

  /**
   * Sets a callback for receiving audio data
   * 
   * This callback is triggered when audio data is received from the meeting.
   * It provides the raw audio data buffer and metadata about the sender.
   * 
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   * 
   * @example
   * ```typescript
   * client.onAudioData((buffer, size, timestamp, metadata) => {
   *   console.log(`Received ${size} bytes of audio from ${metadata.userName}`);
   *   
   *   // Process the audio data
   *   // buffer - Raw audio data (Buffer)
   *   // size - Size of the audio data in bytes
   *   // timestamp - Timestamp of the audio data
   *   // metadata - Information about the sender
   * });
   * ```
   */
  onAudioData(callback: AudioDataCallback): boolean;
  
  /**
   * Sets a callback for receiving video data
   * 
   * This callback is triggered when video data is received from the meeting.
   * It provides the raw video data buffer, track ID, and metadata about the sender.
   * 
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   * 
   * @example
   * ```typescript
   * client.onVideoData((buffer, size, timestamp, trackId, metadata) => {
   *   console.log(`Received ${size} bytes of video from ${metadata.userName}`);
   *   console.log(`Track ID: ${trackId}`);
   *   
   *   // Process the video data
   *   // buffer - Raw video data (Buffer)
   *   // size - Size of the video data in bytes
   *   // timestamp - Timestamp of the video data
   *   // trackId - ID of the video track
   *   // metadata - Information about the sender
   * });
   * ```
   */
  onVideoData(callback: VideoDataCallback): boolean;
  
  /**
   * Sets a callback for receiving transcript data
   * 
   * This callback is triggered when transcript data is received from the meeting.
   * It provides the raw transcript data buffer and metadata about the sender.
   * 
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   * 
   * @example
   * ```typescript
   * client.onTranscriptData((buffer, size, timestamp, metadata) => {
   *   // Convert buffer to string (assuming UTF-8 encoding)
   *   const text = buffer.toString('utf8');
   *   console.log(`Transcript from ${metadata.userName}: ${text}`);
   * });
   * ```
   */
  onTranscriptData(callback: TranscriptDataCallback): boolean;
  
  /**
   * Sets a callback for leave events
   * 
   * This callback is triggered when the client leaves the meeting,
   * either voluntarily or due to an error.
   * 
   * @param callback The callback function to invoke
   * @returns true if the callback was set successfully
   * 
   * @example
   * ```typescript
   * client.onLeave((reason) => {
   *   console.log(`Left meeting with reason code: ${reason}`);
   *   // Clean up resources or reconnect based on the reason code
   * });
   * ```
   */
  onLeave(callback: LeaveCallback): boolean;

  /**
   * Subscribe to receive specific event types
   *
   * This method allows you to subscribe to specific event types from the SDK.
   * Note: Typed event callbacks (onParticipantEvent, onActiveSpeakerEvent, etc.)
   * automatically subscribe to their respective events.
   *
   * @param events Array of event type constants (EVENT_PARTICIPANT_JOIN, etc.)
   * @returns true if subscription was successful
   *
   * @example
   * ```typescript
   * // Subscribe to participant events
   * client.subscribeEvent([rtms.EVENT_PARTICIPANT_JOIN, rtms.EVENT_PARTICIPANT_LEAVE]);
   * ```
   */
  subscribeEvent(events: number[]): boolean;

  /**
   * Unsubscribe from specific event types
   *
   * This method allows you to unsubscribe from specific event types.
   *
   * @param events Array of event type constants to unsubscribe from
   * @returns true if unsubscription was successful
   *
   * @example
   * ```typescript
   * // Unsubscribe from participant events
   * client.unsubscribeEvent([rtms.EVENT_PARTICIPANT_JOIN, rtms.EVENT_PARTICIPANT_LEAVE]);
   * ```
   */
  unsubscribeEvent(events: number[]): boolean;
}

//-----------------------------------------------------------------------------------
// Webhook and Utility Functions
//-----------------------------------------------------------------------------------

/**
 * Creates a request handler for webhook events that can be mounted on existing HTTP servers
 *
 * This function returns a Node.js request handler compatible with Express, Fastify,
 * and other HTTP frameworks. It allows you to integrate Zoom webhook handling with
 * your existing application routes on a shared port.
 *
 * The handler validates that requests are POST requests to the specified path,
 * parses JSON payloads, and invokes your callback with the webhook data.
 *
 * @param callback Function to call when webhook events are received (WebhookCallback or RawWebhookCallback)
 * @param path The URL path to listen on (e.g., '/zoom/webhook')
 * @returns A request handler function compatible with http.Server
 *
 * @example
 * ```typescript
 * import express from 'express';
 * import rtms from '@zoom/rtms';
 *
 * const app = express();
 * app.use(express.json());
 *
 * // Your application routes
 * app.get('/health', (req, res) => {
 *   res.json({ status: 'healthy' });
 * });
 *
 * // Mount Zoom webhook handler on the same server
 * const webhookHandler = rtms.createWebhookHandler(
 *   (payload) => {
 *     console.log(`Received webhook: ${payload.event}`);
 *
 *     if (payload.event === "meeting.rtms.started") {
 *       // Create a client and join the meeting
 *       const client = new rtms.Client();
 *       client.join({
 *         meeting_uuid: payload.payload.meeting_uuid,
 *         rtms_stream_id: payload.payload.rtms_stream_id,
 *         server_urls: payload.payload.server_urls
 *       });
 *     }
 *   },
 *   '/zoom/webhook'
 * );
 *
 * // Mount the handler on your Express app
 * app.post('/zoom/webhook', webhookHandler);
 *
 * // Single port for all routes (Cloud Run, Kubernetes, etc.)
 * const PORT = process.env.PORT || 8080;
 * app.listen(PORT, () => {
 *   console.log(`Server listening on port ${PORT}`);
 * });
 * ```
 *
 * @category Common Functions
 */
export function createWebhookHandler(
  callback: WebhookCallback | RawWebhookCallback,
  path: string
): (req: import('http').IncomingMessage, res: import('http').ServerResponse) => void;

/**
 * Sets up a webhook server to receive events from Zoom
 *
 * This function creates an HTTP or HTTPS server that listens for webhook events from Zoom.
 * When a webhook event is received, it parses the JSON payload and passes it to
 * the provided callback function.
 *
 * For secure HTTPS connections, provide the following environment variables:
 * - ZM_RTMS_CERT: Path to SSL certificate file
 * - ZM_RTMS_KEY: Path to SSL certificate key file
 * - ZM_RTMS_CA_WEBHOOK: (Optional) Path to CA certificate for client verification
 *
 * The callback can be either a basic WebhookCallback (receives only the payload)
 * or a RawWebhookCallback (receives payload, request, and response objects for
 * custom handling of webhook validation challenges).
 *
 * @param callback Function to call when webhook events are received
 *
 * @example
 * ```typescript
 * import rtms from '@zoom/rtms';
 *
 * // Basic webhook handler (automatically responds with 200 OK)
 * rtms.onWebhookEvent((payload) => {
 *   if (payload.event === "meeting.rtms.started") {
 *     console.log(`RTMS started for meeting: ${payload.meeting_uuid}`);
 *     // Create a client and join the meeting
 *     const client = new rtms.Client();
 *     client.join({...});
 *   }
 * });
 * ```
 *
 * @category Common Functions
 */
export function onWebhookEvent(callback: WebhookCallback | RawWebhookCallback): void;

/**
 * Configure the RTMS logger
 * 
 * This function allows you to configure the logging behavior of the RTMS SDK.
 * You can change the log level, format, and enable/disable logging.
 * 
 * @param options Configuration options for the logger
 * 
 * @example
 * ```typescript
 * // Enable debug logging with JSON format
 * rtms.configureLogger({
 *   level: rtms.LogLevel.DEBUG,
 *   format: rtms.LogFormat.JSON,
 *   enabled: true
 * });
 * 
 * // Disable logging
 * rtms.configureLogger({
 *   enabled: false
 * });
 * ```
 * 
 * @category Common Functions
 */
export function configureLogger(options: Partial<LoggerConfig>): void;

/**
 * Generates an HMAC-SHA256 signature for RTMS authentication
 * 
 * This function creates a signature required for authenticating with Zoom RTMS servers.
 * It uses HMAC-SHA256 with the client secret as the key and a concatenated string of
 * client ID, meeting UUID, and stream ID as the message.
 * 
 * @param params Parameters for generating the signature
 * @returns Hex-encoded HMAC-SHA256 signature
 * @throws ReferenceError if client ID or secret is empty
 * 
 * @example
 * ```typescript
 * const signature = rtms.generateSignature({
 *   client: "your_client_id",
 *   secret: "your_client_secret",
 *   uuid: "meeting_uuid",
 *   streamId: "rtms_stream_id"
 * });
 * ```
 * 
 * @category Common Functions
 */
export function generateSignature(params: SignatureParams): string;

/**
 * Checks if the RTMS SDK has been initialized
 * 
 * @returns true if the SDK is initialized
 * 
 * @category Common Functions
 */
export function isInitialized(): boolean;


// In rtms.d.ts

// Audio parameter interfaces
export interface AudioContentType {
  UNDEFINED: number;
  RTP: number;
  RAW_AUDIO: number;
  FILE_STREAM: number;
  TEXT: number;
}

export interface AudioCodec {
  UNDEFINED: number;
  L16: number;
  G711: number;
  G722: number;
  OPUS: number;
}

export interface AudioSampleRate {
  SR_8K: number;
  SR_16K: number;
  SR_32K: number;
  SR_48K: number;
}

export interface AudioChannel {
  MONO: number;
  STEREO: number;
}

export interface AudioDataOption {
  UNDEFINED: number;
  AUDIO_MIXED_STREAM: number;
  AUDIO_MULTI_STREAMS: number;
}

// Video parameter interfaces
export interface VideoContentType {
  UNDEFINED: number;
  RTP: number;
  RAW_VIDEO: number;
  FILE_STREAM: number;
  TEXT: number;
}

export interface VideoCodec {
  UNDEFINED: number;
  JPG: number;
  PNG: number;
  H264: number;
}

export interface VideoResolution {
  SD: number;
  HD: number;
  FHD: number;
  QHD: number;
}

export interface VideoDataOption {
  UNDEFINED: number;
  VIDEO_SINGLE_ACTIVE_STREAM: number;
  VIDEO_MIXED_SPEAKER_VIEW: number;
  VIDEO_MIXED_GALLERY_VIEW: number;
}

export interface MediaDataType {
  UNDEFINED: number;
  AUDIO: number;
  VIDEO: number;
  DESKSHARE: number;
  TRANSCRIPT: number;
  CHAT: number;
  ALL: number;
}

export interface SessionState {
  INACTIVE: number;
  INITIALIZE: number;
  STARTED: number;
  PAUSED: number;
  RESUMED: number;
  STOPPED: number;
}

export interface StreamState {
  INACTIVE: number;
  ACTIVE: number;
  INTERRUPTED: number;
  TERMINATING: number;
  TERMINATED: number;
}

export interface EventType {
  UNDEFINED: number;
  FIRST_PACKET_TIMESTAMP: number;
  ACTIVE_SPEAKER_CHANGE: number;
  PARTICIPANT_JOIN: number;
  PARTICIPANT_LEAVE: number;
  SHARING_START: number;
  SHARING_STOP: number;
  MEDIA_CONNECTION_INTERRUPTED: number;
  CONSUMER_ANSWERED: number;
  CONSUMER_END: number;
  USER_ANSWERED: number;
  USER_END: number;
  USER_HOLD: number;
  USER_UNHOLD: number;
}

export interface MessageType {
  UNDEFINED: number;
  SIGNALING_HAND_SHAKE_REQ: number;
  SIGNALING_HAND_SHAKE_RESP: number;
  DATA_HAND_SHAKE_REQ: number;
  DATA_HAND_SHAKE_RESP: number;
  EVENT_SUBSCRIPTION: number;
  EVENT_UPDATE: number;
  CLIENT_READY_ACK: number;
  STREAM_STATE_UPDATE: number;
  SESSION_STATE_UPDATE: number;
  SESSION_STATE_REQ: number;
  SESSION_STATE_RESP: number;
  KEEP_ALIVE_REQ: number;
  KEEP_ALIVE_RESP: number;
  MEDIA_DATA_AUDIO: number;
  MEDIA_DATA_VIDEO: number;
  MEDIA_DATA_SHARE: number;
  MEDIA_DATA_TRANSCRIPT: number;
  MEDIA_DATA_CHAT: number;
}

export interface StopReason {
  UNDEFINED: number;
  STOP_BC_HOST_TRIGGERED: number;
  STOP_BC_USER_TRIGGERED: number;
  STOP_BC_USER_LEFT: number;
  STOP_BC_USER_EJECTED: number;
  STOP_BC_APP_DISABLED_BY_HOST: number;
  STOP_BC_MEETING_ENDED: number;
  STOP_BC_STREAM_CANCELED: number;
  STOP_BC_STREAM_REVOKED: number;
  STOP_BC_ALL_APPS_DISABLED: number;
  STOP_BC_INTERNAL_EXCEPTION: number;
  STOP_BC_CONNECTION_TIMEOUT: number;
  STOP_BC_MEETING_CONNECTION_INTERRUPTED: number;
  STOP_BC_SIGNAL_CONNECTION_INTERRUPTED: number;
  STOP_BC_DATA_CONNECTION_INTERRUPTED: number;
  STOP_BC_SIGNAL_CONNECTION_CLOSED_ABNORMALLY: number;
  STOP_BC_DATA_CONNECTION_CLOSED_ABNORMALLY: number;
  STOP_BC_EXIT_SIGNAL: number;
  STOP_BC_AUTHENTICATION_FAILURE: number;
}

/**
 * Default export of the RTMS module
 *
 * This object contains all exported functions and classes for the RTMS SDK.
 * Use the Client class for connecting to Zoom RTMS streams.
 */
declare const rtms: {
  // Class-based API
  Client: typeof Client;
  onWebhookEvent: typeof onWebhookEvent;
  createWebhookHandler: typeof createWebhookHandler;

  // Utility functions
  generateSignature: typeof generateSignature;
  isInitialized: typeof isInitialized;
  configureLogger: typeof configureLogger;

  // Enums
  LogLevel: typeof LogLevel;
  LogFormat: typeof LogFormat;

  // Constants
  MEDIA_TYPE_AUDIO: number;
  MEDIA_TYPE_VIDEO: number;
  MEDIA_TYPE_DESKSHARE: number;
  MEDIA_TYPE_TRANSCRIPT: number;
  MEDIA_TYPE_CHAT: number;
  MEDIA_TYPE_ALL: number;

  SESSION_EVENT_ADD: number;
  SESSION_EVENT_STOP: number;
  SESSION_EVENT_PAUSE: number;
  SESSION_EVENT_RESUME: number;

  USER_JOIN: number;
  USER_LEAVE: number;

  EVENT_UNDEFINED: number;
  EVENT_FIRST_PACKET_TIMESTAMP: number;
  EVENT_ACTIVE_SPEAKER_CHANGE: number;
  EVENT_PARTICIPANT_JOIN: number;
  EVENT_PARTICIPANT_LEAVE: number;
  EVENT_SHARING_START: number;
  EVENT_SHARING_STOP: number;
  EVENT_MEDIA_CONNECTION_INTERRUPTED: number;
  EVENT_CONSUMER_ANSWERED: number;
  EVENT_CONSUMER_END: number;
  EVENT_USER_ANSWERED: number;
  EVENT_USER_END: number;
  EVENT_USER_HOLD: number;
  EVENT_USER_UNHOLD: number;

  RTMS_SDK_FAILURE: number;
  RTMS_SDK_OK: number;
  RTMS_SDK_TIMEOUT: number;
  RTMS_SDK_NOT_EXIST: number;
  RTMS_SDK_WRONG_TYPE: number;
  RTMS_SDK_INVALID_STATUS: number;
  RTMS_SDK_INVALID_ARGS: number;

  SESS_STATUS_ACTIVE: number;
  SESS_STATUS_PAUSED: number;

  AudioContentType: AudioContentType;
  AudioCodec: AudioCodec;
  AudioSampleRate: AudioSampleRate;
  AudioChannel: AudioChannel;
  AudioDataOption: AudioDataOption;
  VideoContentType: VideoContentType;
  VideoCodec: VideoCodec;
  VideoResolution: VideoResolution;
  VideoDataOption: VideoDataOption;
  MediaDataType: MediaDataType;
  SessionState: SessionState;
  StreamState: StreamState;
  EventType: EventType;
  MessageType: MessageType;
  StopReason: StopReason;
};

export default rtms;