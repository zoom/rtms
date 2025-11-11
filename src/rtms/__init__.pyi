"""
Zoom RTMS SDK - Python Type Stubs
Real-Time Media Streaming SDK for Python
"""

from typing import Callable, Dict, Any, Optional

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
    """Audio streaming parameters"""
    def __init__(
        self,
        content_type: int = 0,
        codec: int = 0,
        sample_rate: int = 0,
        channel: int = 0,
        data_opt: int = 0,
        duration: int = 0,
        frame_size: int = 0
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
        """Join a Zoom RTMS session"""
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

    def onUserUpdate(self, callback: Callable[[int, Participant], None]) -> None:
        """Register user update callback"""
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

    def onEventEx(self, callback: Callable[[str], None]) -> None:
        """Register extended event callback"""
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
# Constants - Events
# ============================================================================

SESSION_EVENT_ADD: int
SESSION_EVENT_STOP: int
SESSION_EVENT_PAUSE: int
SESSION_EVENT_RESUME: int

USER_EVENT_JOIN: int
USER_EVENT_LEAVE: int

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
# Global Functions
# ============================================================================

def initialize(ca_path: Optional[str] = None) -> bool:
    """Initialize the RTMS SDK"""
    ...

def uninitialize() -> bool:
    """Uninitialize the RTMS SDK"""
    ...

def join(
    meeting_uuid: Optional[str] = None,
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
    """Join a Zoom RTMS session using the global client"""
    ...

def leave() -> bool:
    """Leave the RTMS session"""
    ...

def uuid() -> str:
    """Get the UUID of the current meeting"""
    ...

def stream_id() -> str:
    """Get the stream ID of the current session"""
    ...

def generate_signature(client: str, secret: str, uuid: str, stream_id: str) -> str:
    """Generate a signature for RTMS authentication"""
    ...

def setAudioParams(params: AudioParams) -> None:
    """Set audio parameters for the global client"""
    ...

def setVideoParams(params: VideoParams) -> None:
    """Set video parameters for the global client"""
    ...

def setDeskshareParams(params: DeskshareParams) -> None:
    """Set deskshare parameters for the global client"""
    ...

# ============================================================================
# Callback Decorators
# ============================================================================

def on_webhook_event(
    callback: Optional[Callable[[Dict[str, Any]], None]] = None,
    port: Optional[int] = None,
    path: Optional[str] = None
) -> Callable[[Dict[str, Any]], None]:
    """Register a webhook event handler"""
    ...

def onJoinConfirm(func: Callable[[int], None]) -> Callable[[int], None]:
    """Decorator for join confirmation callback"""
    ...

def onSessionUpdate(func: Callable[[int, Session], None]) -> Callable[[int, Session], None]:
    """Decorator for session update callback"""
    ...

def onUserUpdate(func: Callable[[int, Participant], None]) -> Callable[[int, Participant], None]:
    """Decorator for user update callback"""
    ...

def onAudioData(func: Callable[[bytes, int, int, Metadata], None]) -> Callable[[bytes, int, int, Metadata], None]:
    """Decorator for audio data callback"""
    ...

def onVideoData(func: Callable[[bytes, int, int, Metadata], None]) -> Callable[[bytes, int, int, Metadata], None]:
    """Decorator for video data callback"""
    ...

def onDeskshareData(func: Callable[[bytes, int, int, Metadata], None]) -> Callable[[bytes, int, int, Metadata], None]:
    """Decorator for deskshare data callback"""
    ...

def onTranscriptData(func: Callable[[bytes, int, int, Metadata], None]) -> Callable[[bytes, int, int, Metadata], None]:
    """Decorator for transcript data callback"""
    ...

def onLeave(func: Callable[[int], None]) -> Callable[[int], None]:
    """Decorator for leave callback"""
    ...

def onEventEx(func: Callable[[str], None]) -> Callable[[str], None]:
    """Decorator for extended event callback"""
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
