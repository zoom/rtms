"""
Zoom RTMS SDK
-----------------------

.. currentmodule:: rtms

.. autosummary::
    :toctree: _generate

    initialize
    join
    leave
    poll
    uuid
    stream_id
    generate_signature
    set_debug
    Client
    Session
    Participant
    Metadata
"""

# Core initialization and connection functions
def initialize(ca: str) -> int:
    """
    Initialize Realtime Media Streams

    Takes the path to a CA Certificate
    """

def join(uuid: str, stream_id: str, signature: str, server_urls: str, timeout: int = -1) -> bool:
    """
    Join a Zoom RTMS session using the global client
    """

def leave() -> bool:
    """
    Leave the RTMS session and clean up resources
    """

def uninitialize() -> bool:
    """
    Uninitialize the RTMS SDK
    """

def uuid() -> str:
    """
    Get the UUID of the current meeting
    """

def stream_id() -> str:
    """
    Get the stream ID of the current session
    """

def generate_signature(client: str, secret: str, uuid: str, stream_id: str) -> str:
    """
    Generate a signature for RTMS authentication
    """

def set_debug(enable: bool = True) -> None:
    """
    Enable or disable debug logging
    """

# Event handling decorators
def on_webhook_event(callback=None, port=None, path=None):
    """
    Register a webhook event handler
    """

def on_join_confirm(func):
    """
    Decorator for join confirmation callback
    """

def on_session_update(func):
    """
    Decorator for session update callback
    """

def on_user_update(func):
    """
    Decorator for user update callback
    """

def on_audio_data(func):
    """
    Decorator for audio data callback
    """

def on_video_data(func):
    """
    Decorator for video data callback
    """

def on_transcript_data(func):
    """
    Decorator for transcript data callback
    """

def on_leave(func):
    """
    Decorator for leave callback
    """

# Constants
SDK_AUDIO: int
SDK_VIDEO: int
SDK_TRANSCRIPT: int
SDK_ALL: int

SESSION_ADD: int
SESSION_STOP: int
SESSION_PAUSE: int
SESSION_RESUME: int

USER_JOIN: int
USER_LEAVE: int

RTMS_SDK_FAILURE: int
RTMS_SDK_OK: int
RTMS_SDK_TIMEOUT: int
RTMS_SDK_NOT_EXIST: int
RTMS_SDK_WRONG_TYPE: int
RTMS_SDK_INVALID_STATUS: int
RTMS_SDK_INVALID_ARGS: int

SESS_STATUS_ACTIVE: int
SESS_STATUS_PAUSED: int

# Classes
class Session:
    """Information about a Zoom meeting session"""
    
    def session_id(self) -> str: ...
    def stat_time(self) -> int: ...
    def status(self) -> int: ...
    def is_active(self) -> bool: ...
    def is_paused(self) -> bool: ...

class Participant:
    """Information about a participant in a Zoom meeting"""
    
    def id(self) -> int: ...
    def name(self) -> str: ...

class Metadata:
    """Metadata about a participant in a Zoom meeting"""
    
    def user_name(self) -> str: ...
    def user_id(self) -> int: ...

class Client:
    """RTMS Client for connecting to Zoom real-time media streams"""
    
    def __init__(self) -> None: ...
    
    @staticmethod
    def initialize(ca_path: str) -> None: ...
    
    @staticmethod
    def uninitialize() -> None: ...
    
    def join(self, 
             uuid: str = None, 
             stream_id: str = None,
             server_urls: str = None,
             signature: str = None,
             timeout: int = -1,
             ca: str = None,
             client: str = None,
             secret: str = None,
             poll_interval: int = 10,
             **kwargs) -> bool: ...
    
    def poll(self) -> None: ...
    def release(self) -> None: ...
    def stop(self) -> bool: ...
    def leave(self) -> bool: ...
    def uuid(self) -> str: ...
    def stream_id(self) -> str: ...
    
    def on_webhook_event(self, callback=None, port=None, path=None): ...
    def on_join_confirm(self): ...
    def on_session_update(self): ...
    def on_user_update(self): ...
    def on_audio_data(self): ...
    def on_video_data(self): ...
    def on_transcript_data(self): ...
    def on_leave(self): ...
    
    # For direct callback setting
    def set_join_confirm_callback(self, func): ...
    def set_session_update_callback(self, func): ...
    def set_user_update_callback(self, func): ...
    def set_audio_data_callback(self, func): ...
    def set_video_data_callback(self, func): ...
    def set_transcript_data_callback(self, func): ...
    def set_leave_callback(self, func): ...