
import signal
import sys
import hmac
import hashlib
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
from typing import Dict, Any, Optional, Callable

from ._rtms import (
    # Classes
    Client as _Client, Session, Participant, Metadata,
    
    # Global functions
    _initialize, _uninitialize, _join, _poll, _release,
    _uuid, _stream_id, _enable_audio, _enable_video, _enable_transcript, _enable_deskshare,
    
    # Callback functions
    on_join_confirm, on_session_update, on_user_update, 
    on_audio_data, on_video_data, on_deskshare_data, on_transcript_data, on_leave,
    
    # Parameter functions
    set_audio_params, set_video_params, set_deskshare_params,
    
    # Constants
    SDK_AUDIO, SDK_VIDEO, SDK_DESKSHARE, SDK_TRANSCRIPT, SDK_ALL,
    SESSION_ADD, SESSION_STOP, SESSION_PAUSE, SESSION_RESUME,
    USER_JOIN, USER_LEAVE
)

# Parameter constants/enums
class AudioContentType:
    RAW_AUDIO = 0

class AudioCodec:
    OPUS = 0
    PCM = 1

class AudioSampleRate:
    SR_8K = 8000
    SR_16K = 16000
    SR_32K = 32000
    SR_44K = 44100
    SR_48K = 48000

class AudioChannel:
    MONO = 1
    STEREO = 2

class AudioDataOption:
    AUDIO_MIXED_STREAM = 0
    AUDIO_INDIVIDUAL_STREAM = 1

class VideoContentType:
    RAW_VIDEO = 0

class VideoCodec:
    H264 = 0
    VP8 = 1

class VideoResolution:
    HD = 0
    FHD = 1

class VideoDataOption:
    VIDEO_MIXED_STREAM = 0
    VIDEO_INDIVIDUAL_STREAM = 1

# Global webhook server
_webhook_server = None
_webhook_callback = None

def signal_handler(sig, frame):
    print("Shutting down cleanly...")
    # Stop webhook server
    global _webhook_server
    if _webhook_server:
        _webhook_server.shutdown()
    uninitialize()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

class WebhookHandler(BaseHTTPRequestHandler):
    """HTTP request handler for webhook events"""
    
    def do_POST(self):
        global _webhook_callback
        if _webhook_callback:
            try:
                content_length = int(self.headers.get('Content-Length', 0))
                post_data = self.rfile.read(content_length)
                webhook_data = json.loads(post_data.decode('utf-8'))
                
                # Call the webhook callback
                _webhook_callback(webhook_data)
                
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b'OK')
            except Exception as e:
                self.send_response(500)
                self.end_headers()
                self.wfile.write(f'Error: {str(e)}'.encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def log_message(self, format, *args):
        # Suppress log messages
        pass

def on_webhook_event(callback: Optional[Callable] = None, port: int = 8080, path: str = "/webhook"):
    """Set up webhook event handler"""
    def decorator(func):
        global _webhook_server, _webhook_callback
        
        _webhook_callback = func
        
        if not _webhook_server:
            server = HTTPServer(('0.0.0.0', port), WebhookHandler)
            _webhook_server = server
            
            # Start server in background thread
            def run_server():
                server.serve_forever()
            
            thread = threading.Thread(target=run_server, daemon=True)
            thread.start()
            print(f"Webhook server running on port {port}")
        
        return func
    
    if callback is not None:
        return decorator(callback)
    return decorator

class Client(_Client):
    """Enhanced RTMS Client with Python-specific features"""
    
    @staticmethod
    def initialize(ca_path: str = "") -> bool:
        """Initialize the RTMS SDK"""
        try:
            _initialize(ca_path)
            return True
        except:
            return False
    
    @staticmethod
    def uninitialize() -> bool:
        """Uninitialize the RTMS SDK"""
        try:
            _uninitialize()
            return True
        except:
            return False
    
    def leave(self) -> bool:
        """Leave the meeting and release resources"""
        try:
            # In Python, we don't have automatic polling to stop,
            # so just call release directly  
            self.release()
            return True
        except:
            return False
    
    def enable_audio(self, enable: bool) -> bool:
        """Enable or disable audio"""
        try:
            super().enable_audio(enable)
            return True
        except:
            return False
    
    def enable_video(self, enable: bool) -> bool:
        """Enable or disable video"""
        try:
            super().enable_video(enable)
            return True
        except:
            return False
    
    def enable_transcript(self, enable: bool) -> bool:
        """Enable or disable transcript"""
        try:
            super().enable_transcript(enable)
            return True
        except:
            return False
    
    def enable_deskshare(self, enable: bool) -> bool:
        """Enable or disable deskshare"""
        try:
            super().enable_deskshare(enable)
            return True
        except:
            return False
    
    def set_audio_params(self, params: Dict[str, Any]) -> bool:
        """Set audio parameters"""
        try:
            super().set_audio_params(params)
            return True
        except:
            return False
    
    def set_audio_parameters(self, params: Dict[str, Any]) -> bool:
        """Set audio parameters (alias for set_audio_params)"""
        return self.set_audio_params(params)
    
    def set_video_params(self, params: Dict[str, Any]) -> bool:
        """Set video parameters"""
        try:
            super().set_video_params(params)
            return True
        except:
            return False
    
    def set_video_parameters(self, params: Dict[str, Any]) -> bool:
        """Set video parameters (alias for set_video_params)"""
        return self.set_video_params(params)
    
    def set_deskshare_params(self, params: Dict[str, Any]) -> bool:
        """Set deskshare parameters"""
        try:
            super().set_deskshare_params(params)
            return True
        except:
            return False
    
    def set_deskshare_parameters(self, params: Dict[str, Any]) -> bool:
        """Set deskshare parameters (alias for set_deskshare_params)"""
        return self.set_deskshare_params(params)
    
    def join(self, uuid_or_payload=None, stream_id: Optional[str] = None, 
             signature: Optional[str] = None, server_urls: Optional[str] = None, 
             timeout: int = -1, **kwargs) -> bool:
        """Join a meeting with parameters or options dict"""
        try:
            # If first argument is a dict, treat as payload/options
            if isinstance(uuid_or_payload, dict):
                payload = uuid_or_payload
                # Extract parameters from payload
                uuid = payload.get('meeting_uuid') or payload.get('uuid')
                stream_id = payload.get('stream_id') 
                signature = payload.get('signature')
                server_urls = payload.get('server_urls')
                timeout = payload.get('timeout', -1)
                
                # Generate signature if not provided but we have client/secret
                if not signature and payload.get('client') and payload.get('secret'):
                    signature = generate_signature(
                        payload['client'], payload['secret'], uuid, stream_id
                    )
                
                return super().join(uuid, stream_id, signature, server_urls, timeout)
            else:
                # Individual parameters
                return super().join(uuid_or_payload, stream_id, signature, server_urls, timeout)
        except:
            return False

def initialize(ca_path: str = "") -> bool:
    """Initialize the RTMS SDK"""
    return Client.initialize(ca_path)

def uninitialize() -> bool:
    """Uninitialize the RTMS SDK"""
    return Client.uninitialize()

def join(uuid: str, stream_id: str, signature: str, server_urls: str, timeout: int = -1) -> bool:
    """Join a meeting using the global client"""
    try:
        _join(uuid, stream_id, signature, server_urls, timeout)
        return True
    except:
        return False

def leave() -> bool:
    """Leave the meeting using the global client"""
    try:
        # In Python, we don't have background polling to stop,
        # so we just call release directly
        _release()
        return True
    except:
        return False

def poll() -> None:
    """Poll for events using the global client"""
    _poll()

def uuid() -> str:
    """Get meeting UUID from the global client"""
    return _uuid()

def stream_id() -> str:
    """Get stream ID from the global client"""
    return _stream_id()

def release() -> bool:
    """Release resources using the global client"""
    try:
        _release()
        return True
    except:
        return False

def enable_audio(enable: bool) -> bool:
    """Enable/disable audio on the global client"""
    try:
        _enable_audio(enable)
        return True
    except:
        return False

def enable_video(enable: bool) -> bool:
    """Enable/disable video on the global client"""
    try:
        _enable_video(enable)
        return True
    except:
        return False

def enable_transcript(enable: bool) -> bool:
    """Enable/disable transcript on the global client"""
    try:
        _enable_transcript(enable)
        return True
    except:
        return False

def enable_deskshare(enable: bool) -> bool:
    """Enable/disable deskshare on the global client"""
    try:
        _enable_deskshare(enable)
        return True
    except:
        return False

def generate_signature(client_id: str, client_secret: str, meeting_uuid: str, rtms_stream_id: str) -> str:
    """Generate HMAC-SHA256 signature for RTMS authentication"""
    message = f"{client_id}{meeting_uuid}{rtms_stream_id}"
    signature = hmac.new(
        client_secret.encode('utf-8'),
        message.encode('utf-8'),
        hashlib.sha256
    ).hexdigest()
    return signature

# Default export
__all__ = [
    # Classes
    "Client", "Session", "Participant", "Metadata",
    
    # Core functions
    "initialize", "uninitialize", "join", "leave", "release", "poll", "uuid", "stream_id",
    
    # Enable functions  
    "enable_audio", "enable_video", "enable_transcript", "enable_deskshare",
    
    # Parameter functions
    "set_audio_params", "set_video_params", "set_deskshare_params",
    
    # Callback decorators
    "on_join_confirm", "on_session_update", "on_user_update",
    "on_audio_data", "on_video_data", "on_deskshare_data", "on_transcript_data", "on_leave",
    
    # Webhook
    "on_webhook_event",
    
    # Utility
    "generate_signature",
    
    # Parameter constants
    "AudioContentType", "AudioCodec", "AudioSampleRate", "AudioChannel", "AudioDataOption",
    "VideoContentType", "VideoCodec", "VideoResolution", "VideoDataOption",
    
    # Constants
    "SDK_AUDIO", "SDK_VIDEO", "SDK_DESKSHARE", "SDK_TRANSCRIPT", "SDK_ALL",
    "SESSION_ADD", "SESSION_STOP", "SESSION_PAUSE", "SESSION_RESUME", 
    "USER_JOIN", "USER_LEAVE"
]