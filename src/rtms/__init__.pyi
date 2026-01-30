"""
Zoom RTMS SDK - Python Type Stubs
Real-Time Media Streaming SDK for Python
"""

from typing import Callable, Dict, Any, Optional, List, Literal, TypedDict

# ============================================================================
# Data Classes
# ============================================================================

class Session:
    """Information about a Zoom meeting session"""
    @property
    def sessionId(self) -> str: ...
    @property
    def streamId(self) -> str: ...
    @property
    def meetingId(self) -> str: ...
    @property
    def statTime(self) -> int: ...
    @property
    def status(self) -> int: ...
    @property
    def isActive(self) -> bool: ...
    @property
    def isPaused(self) -> bool: ...

class Participant:
    """Information about a participant in a Zoom meeting"""
    @property
    def id(self) -> int: ...
    @property
    def name(self) -> str: ...

class Metadata:
    """Metadata about a participant in a Zoom meeting"""
    @property
    def userName(self) -> str: ...
    @property
    def userId(self) -> int: ...

# ============================================================================
# Parameter Classes
# ============================================================================

class AudioParams:
    """
    Audio streaming parameters with sensible defaults.

    Default values (work out-of-box for per-participant audio):
    - content_type: RAW_AUDIO (2)
    - codec: OPUS (4)
    - sample_rate: SR_48K (3)
    - channel: STEREO (2)
    - data_opt: AUDIO_MULTI_STREAMS (2) - enables userId in audio metadata
    - duration: 20 ms
    - frame_size: 960 samples (48kHz × 20ms)

    Users can omit setAudioParams() entirely or override individual fields.
    """
    def __init__(
        self,
        content_type: int = 2,
        codec: int = 4,
        sample_rate: int = 3,
        channel: int = 2,
        data_opt: int = 2,
        duration: int = 20,
        frame_size: int = 960
    ) -> None: ...

    @property
    def contentType(self) -> int: ...
    @contentType.setter
    def contentType(self, value: int) -> None: ...

    @property
    def codec(self) -> int: ...
    @codec.setter
    def codec(self, value: int) -> None: ...

    @property
    def sampleRate(self) -> int: ...
    @sampleRate.setter
    def sampleRate(self, value: int) -> None: ...

    @property
    def channel(self) -> int: ...
    @channel.setter
    def channel(self, value: int) -> None: ...

    @property
    def dataOpt(self) -> int: ...
    @dataOpt.setter
    def dataOpt(self, value: int) -> None: ...

    @property
    def duration(self) -> int: ...
    @duration.setter
    def duration(self, value: int) -> None: ...

    @property
    def frameSize(self) -> int: ...
    @frameSize.setter
    def frameSize(self, value: int) -> None: ...

class VideoParams:
    """Video streaming parameters"""
    def __init__(
        self,
        content_type: int = 0,
        codec: int = 0,
        resolution: int = 0,
        data_opt: int = 0,
        fps: int = 0
    ) -> None: ...

    @property
    def contentType(self) -> int: ...
    @contentType.setter
    def contentType(self, value: int) -> None: ...

    @property
    def codec(self) -> int: ...
    @codec.setter
    def codec(self, value: int) -> None: ...

    @property
    def resolution(self) -> int: ...
    @resolution.setter
    def resolution(self, value: int) -> None: ...

    @property
    def dataOpt(self) -> int: ...
    @dataOpt.setter
    def dataOpt(self, value: int) -> None: ...

    @property
    def fps(self) -> int: ...
    @fps.setter
    def fps(self, value: int) -> None: ...

class DeskshareParams:
    """Deskshare streaming parameters"""
    def __init__(
        self,
        content_type: int = 0,
        codec: int = 0,
        resolution: int = 0,
        fps: int = 0
    ) -> None: ...

    @property
    def contentType(self) -> int: ...
    @contentType.setter
    def contentType(self, value: int) -> None: ...

    @property
    def codec(self) -> int: ...
    @codec.setter
    def codec(self, value: int) -> None: ...

    @property
    def resolution(self) -> int: ...
    @resolution.setter
    def resolution(self, value: int) -> None: ...

    @property
    def fps(self) -> int: ...
    @fps.setter
    def fps(self, value: int) -> None: ...

# ============================================================================
# Callback Types
# ============================================================================

class EventParticipantInfo(TypedDict):
    """Participant info for event callbacks"""
    userId: int
    userName: str  # Optional in leave events

ParticipantEventCallback = Callable[
    [Literal['join', 'leave'], int, List[EventParticipantInfo]], None
]
"""Callback for participant join/leave events: (event, timestamp, participants)"""

ActiveSpeakerEventCallback = Callable[[int, int, str], None]
"""Callback for active speaker events: (timestamp, userId, userName)"""

SharingEventCallback = Callable[
    [Literal['start', 'stop'], int, Optional[int], Optional[str]], None
]
"""Callback for sharing events: (event, timestamp, userId?, userName?)"""

EventExCallback = Callable[[str], None]
"""Callback for raw JSON event data: (eventData)"""

# ============================================================================
# Client Class
# ============================================================================

class Client:
    """RTMS Client for connecting to Zoom real-time media streams"""

    def __init__(self) -> None: ...

    @staticmethod
    def initialize(ca_path: str, is_verify_cert: int = 1, agent: Optional[str] = None) -> None:
        """Initialize the RTMS SDK"""
        ...

    @staticmethod
    def uninitialize() -> None:
        """Uninitialize the RTMS SDK"""
        ...

    def join(
        self,
        meeting_uuid: Optional[str] = None,
        session_id: Optional[str] = None,
        rtms_stream_id: Optional[str] = None,
        server_urls: Optional[str] = None,
        signature: Optional[str] = None,
        timeout: int = -1,
        ca: Optional[str] = None,
        client: Optional[str] = None,
        secret: Optional[str] = None,
        poll_interval: int = 10,
        **kwargs: Any
    ) -> bool:
        """Join a Zoom RTMS session.

        For Meeting SDK events (meeting.rtms_started), use meeting_uuid.
        For Video SDK events (session.rtms_started), use session_id.
        """
        ...

    def poll(self) -> None:
        """Poll for events (call periodically)"""
        ...

    def release(self) -> None:
        """Release client resources"""
        ...

    def leave(self) -> bool:
        """Leave the RTMS session"""
        ...

    def stop(self) -> bool:
        """Stop the RTMS client (alias for leave)"""
        ...

    def uuid(self) -> str:
        """Get meeting UUID"""
        ...

    def streamId(self) -> str:
        """Get stream ID"""
        ...

    def enableAudio(self, enable: bool) -> None:
        """Enable/disable audio streaming"""
        ...

    def enableVideo(self, enable: bool) -> None:
        """Enable/disable video streaming"""
        ...

    def enableTranscript(self, enable: bool) -> None:
        """Enable/disable transcript streaming"""
        ...

    def enableDeskshare(self, enable: bool) -> None:
        """Enable/disable deskshare streaming"""
        ...

    def setAudioParams(self, params: AudioParams) -> None:
        """Set audio parameters"""
        ...

    def setVideoParams(self, params: VideoParams) -> None:
        """Set video parameters"""
        ...

    def setDeskshareParams(self, params: DeskshareParams) -> None:
        """Set deskshare parameters"""
        ...

    def onJoinConfirm(self, callback: Callable[[int], None]) -> None:
        """Register join confirm callback"""
        ...

    def onSessionUpdate(self, callback: Callable[[int, Session], None]) -> None:
        """Register session update callback"""
        ...

    def onParticipantEvent(self, callback: ParticipantEventCallback) -> bool:
        """
        Register participant join/leave event callback.

        This automatically subscribes to EVENT_PARTICIPANT_JOIN and EVENT_PARTICIPANT_LEAVE.

        Args:
            callback: Function called with (event, timestamp, participants)
                - event: 'join' or 'leave'
                - timestamp: Unix timestamp in milliseconds
                - participants: List of participant info dicts with userId and userName

        Returns:
            True if callback was set successfully
        """
        ...

    def onActiveSpeakerEvent(self, callback: ActiveSpeakerEventCallback) -> bool:
        """
        Register active speaker change event callback.

        This automatically subscribes to EVENT_ACTIVE_SPEAKER_CHANGE.

        Args:
            callback: Function called with (timestamp, userId, userName)
                - timestamp: Unix timestamp in milliseconds
                - userId: The user ID of the active speaker
                - userName: The display name of the active speaker

        Returns:
            True if callback was set successfully
        """
        ...

    def onSharingEvent(self, callback: SharingEventCallback) -> bool:
        """
        Register sharing start/stop event callback.

        This automatically subscribes to EVENT_SHARING_START and EVENT_SHARING_STOP.
        Note: These events only work when the RTMS app has DESKSHARE scope permission.

        Args:
            callback: Function called with (event, timestamp, userId, userName)
                - event: 'start' or 'stop'
                - timestamp: Unix timestamp in milliseconds
                - userId: User ID (only for 'start' events)
                - userName: Display name (only for 'start' events)

        Returns:
            True if callback was set successfully
        """
        ...

    def onEventEx(self, callback: EventExCallback) -> bool:
        """
        Register raw JSON event callback.

        This provides access to the raw JSON event data from the SDK.
        Use this when you need custom event handling or access to all event types.
        This callback is called IN ADDITION to typed callbacks, not instead of.

        Args:
            callback: Function called with raw JSON string

        Returns:
            True if callback was set successfully
        """
        ...

    def onAudioData(self, callback: Callable[[bytes, int, int, Metadata], None]) -> None:
        """Register audio data callback"""
        ...

    def onVideoData(self, callback: Callable[[bytes, int, int, Metadata], None]) -> None:
        """Register video data callback"""
        ...

    def onDeskshareData(self, callback: Callable[[bytes, int, int, Metadata], None]) -> None:
        """Register deskshare data callback"""
        ...

    def onTranscriptData(self, callback: Callable[[bytes, int, int, Metadata], None]) -> None:
        """Register transcript data callback"""
        ...

    def onLeave(self, callback: Callable[[int], None]) -> None:
        """Register leave callback"""
        ...

    def subscribeEvent(self, events: List[int]) -> bool:
        """
        Subscribe to receive specific event types.

        Note: Typed event callbacks (onParticipantEvent, onActiveSpeakerEvent, etc.)
        automatically subscribe to their respective events.

        Args:
            events: List of event type constants (e.g., [EVENT_PARTICIPANT_JOIN, EVENT_PARTICIPANT_LEAVE])

        Returns:
            True if subscription was successful
        """
        ...

    def unsubscribeEvent(self, events: List[int]) -> bool:
        """
        Unsubscribe from specific event types.

        Args:
            events: List of event type constants (e.g., [EVENT_PARTICIPANT_JOIN, EVENT_PARTICIPANT_LEAVE])

        Returns:
            True if unsubscription was successful
        """
        ...

    def on_webhook_event(
        self,
        callback: Optional[Callable[[Dict[str, Any]], None]] = None,
        port: Optional[int] = None,
        path: Optional[str] = None
    ) -> Callable[[Dict[str, Any]], None]:
        """Register webhook event handler"""
        ...

# ============================================================================
# Constants - Media Types
# ============================================================================

MEDIA_TYPE_AUDIO: int
MEDIA_TYPE_VIDEO: int
MEDIA_TYPE_DESKSHARE: int
MEDIA_TYPE_TRANSCRIPT: int
MEDIA_TYPE_CHAT: int
MEDIA_TYPE_ALL: int

# ============================================================================
# Constants - Session Events
# ============================================================================

SESSION_EVENT_ADD: int
SESSION_EVENT_STOP: int
SESSION_EVENT_PAUSE: int
SESSION_EVENT_RESUME: int

# ============================================================================
# Constants - Event Types (for subscribeEvent/unsubscribeEvent)
# These match RTMS_EVENT_TYPE from Zoom's C SDK
# ============================================================================

EVENT_UNDEFINED: int
EVENT_FIRST_PACKET_TIMESTAMP: int
EVENT_ACTIVE_SPEAKER_CHANGE: int
EVENT_PARTICIPANT_JOIN: int
EVENT_PARTICIPANT_LEAVE: int
EVENT_SHARING_START: int
EVENT_SHARING_STOP: int
EVENT_MEDIA_CONNECTION_INTERRUPTED: int
EVENT_CONSUMER_ANSWERED: int
EVENT_CONSUMER_END: int
EVENT_USER_ANSWERED: int
EVENT_USER_END: int
EVENT_USER_HOLD: int
EVENT_USER_UNHOLD: int

# ============================================================================
# Constants - Status Codes
# ============================================================================

RTMS_SDK_OK: int
RTMS_SDK_FAILURE: int
RTMS_SDK_TIMEOUT: int
RTMS_SDK_NOT_EXIST: int
RTMS_SDK_WRONG_TYPE: int
RTMS_SDK_INVALID_STATUS: int
RTMS_SDK_INVALID_ARGS: int

SESS_STATUS_ACTIVE: int
SESS_STATUS_PAUSED: int

# ============================================================================
# Parameter Dictionaries
# ============================================================================

AudioContentType: Dict[str, int]
AudioCodec: Dict[str, int]
AudioSampleRate: Dict[str, int]
AudioChannel: Dict[str, int]
AudioDataOption: Dict[str, int]

VideoContentType: Dict[str, int]
VideoCodec: Dict[str, int]
VideoResolution: Dict[str, int]
VideoDataOption: Dict[str, int]

MediaDataType: Dict[str, int]
SessionState: Dict[str, int]
StreamState: Dict[str, int]
EventType: Dict[str, int]
MessageType: Dict[str, int]
StopReason: Dict[str, int]

# ============================================================================
# SDK Initialization Functions
# ============================================================================

def initialize(ca_path: Optional[str] = None) -> bool:
    """
    Initialize the RTMS SDK.

    Note: The SDK is automatically initialized when you create a Client instance.
    You only need to call this if you want to initialize with a custom CA path.

    Args:
        ca_path: Path to the CA certificate file.

    Returns:
        True if initialization was successful
    """
    ...

def uninitialize() -> bool:
    """
    Uninitialize the RTMS SDK.

    Call this when you're done using the SDK to release resources.

    Returns:
        True if uninitialization was successful
    """
    ...

# ============================================================================
# Utility Functions
# ============================================================================

def generate_signature(client: str, secret: str, uuid: str, stream_id: str) -> str:
    """
    Generate a signature for RTMS authentication.

    Args:
        client: Zoom OAuth client ID
        secret: Zoom OAuth client secret
        uuid: Meeting UUID
        stream_id: RTMS stream ID

    Returns:
        Hex-encoded HMAC-SHA256 signature
    """
    ...

# ============================================================================
# Webhook Functions
# ============================================================================

def onWebhookEvent(
    callback: Optional[Callable[[Dict[str, Any]], None]] = None,
    port: Optional[int] = None,
    path: Optional[str] = None
) -> Callable[[Dict[str, Any]], None]:
    """
    Start a webhook server to receive events from Zoom.

    This function creates an HTTP server that listens for webhook events from Zoom.
    When a webhook event is received, it parses the JSON payload and passes it to
    the provided callback function.

    Can be used as a decorator or a direct function call.

    Args:
        callback: Function to call when a webhook is received
        port: Port to listen on. Defaults to ZM_RTMS_PORT env var or 8080
        path: URL path to listen on. Defaults to ZM_RTMS_PATH env var or '/'

    Returns:
        Decorator function if used as a decorator
    """
    ...

# Alias for backwards compatibility
on_webhook_event = onWebhookEvent

# ============================================================================
# Event Loop Functions
# ============================================================================

def run(poll_interval: float = 0.01, stop_on_empty: bool = False) -> None:
    """
    Start the RTMS event loop.

    This function blocks and handles:
    - Polling all active clients
    - Processing pending operations from other threads (like webhook handlers)
    - Graceful shutdown on KeyboardInterrupt

    With this function, you can create clients and call join() directly from
    webhook handlers without manual queue management.

    Args:
        poll_interval: Time in seconds between poll cycles (default: 0.01 = 10ms)
        stop_on_empty: If True, stop when no clients remain (default: False)

    Example:
        >>> import rtms
        >>>
        >>> clients = {}
        >>>
        >>> @rtms.onWebhookEvent
        >>> def handle(payload):
        >>>     client = rtms.Client()
        >>>     clients[payload['payload']['rtms_stream_id']] = client
        >>>     client.onTranscriptData(lambda d,s,t,m: print(m.userName, d))
        >>>     client.join(payload['payload'])
        >>>
        >>> rtms.run()  # Blocks until interrupted
    """
    ...

def stop() -> None:
    """
    Signal the event loop to stop.

    Call this from another thread to gracefully stop the rtms.run() loop.
    """
    ...

# ============================================================================
# Logging
# ============================================================================

class LogLevel:
    """Available log levels for RTMS SDK logging"""
    ERROR: int
    WARN: int
    INFO: int
    DEBUG: int
    TRACE: int

class LogFormat:
    """Available log output formats"""
    PROGRESSIVE: str
    JSON: str

def log_debug(component: str, message: str, details: Optional[Dict[str, Any]] = None) -> None:
    """Log a debug message"""
    ...

def log_info(component: str, message: str, details: Optional[Dict[str, Any]] = None) -> None:
    """Log an info message"""
    ...

def log_warn(component: str, message: str, details: Optional[Dict[str, Any]] = None) -> None:
    """Log a warning message"""
    ...

def log_error(component: str, message: str, details: Optional[Dict[str, Any]] = None) -> None:
    """Log an error message"""
    ...

def configure_logger(options: Dict[str, Any]) -> None:
    """Configure the logger with the specified options"""
    ...
