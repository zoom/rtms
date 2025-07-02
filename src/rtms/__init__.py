# src/rtms/__init__.py - Enhanced version with background polling

import signal
import sys
import hmac
import hashlib
import threading
import time
import atexit
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

# Global polling state
_global_polling_thread = None
_global_polling_stop_event = None
_global_poll_rate = 10  # 10ms default, matching Node.js behavior

# Global webhook server
_webhook_server = None
_webhook_callback = None

def signal_handler(sig, frame):
    print("Shutting down cleanly...")
    stop_global_polling()
    # Stop webhook server
    global _webhook_server
    if _webhook_server:
        _webhook_server.shutdown()
    uninitialize()
    sys.exit(0)

def cleanup_at_exit():
    """Cleanup function called at exit"""
    stop_global_polling()
    try:
        uninitialize()
    except:
        pass

signal.signal(signal.SIGINT, signal_handler)
atexit.register(cleanup_at_exit)

def start_global_polling():
    """Start background polling thread for global client"""
    global _global_polling_thread, _global_polling_stop_event
    
    if _global_polling_thread and _global_polling_thread.is_alive():
        return  # Already running
    
    _global_polling_stop_event = threading.Event()
    
    def polling_loop():
        """Background polling loop"""
        while not _global_polling_stop_event.is_set():
            try:
                _poll()
                # Sleep for poll_rate milliseconds
                time.sleep(_global_poll_rate / 1000.0)
            except Exception as e:
                print(f"Error during global polling: {e}")
                break
    
    _global_polling_thread = threading.Thread(target=polling_loop, daemon=True)
    _global_polling_thread.start()
    print(f"Started global polling with interval: {_global_poll_rate}ms")

def stop_global_polling():
    """Stop background polling thread"""
    global _global_polling_thread, _global_polling_stop_event
    
    if _global_polling_stop_event:
        _global_polling_stop_event.set()
    
    if _global_polling_thread and _global_polling_thread.is_alive():
        _global_polling_thread.join(timeout=1.0)  # Wait up to 1 second
        print("Stopped global polling")
    
    _global_polling_thread = None
    _global_polling_stop_event = None

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
    
    def __init__(self):
        super().__init__()
        self._polling_thread = None
        self._polling_stop_event = None
        self._poll_rate = 10  # 10ms default
        self._joined = False
    
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
    
    def join(self, *args, **kwargs) -> bool:
        """Join a meeting and start background polling"""
        try:
            # Call parent join method
            if isinstance(args[0], dict):
                # Handle dictionary payload
                payload = args[0]
                poll_interval = payload.get('pollInterval', 10)
                self._poll_rate = poll_interval
                result = super().join(payload)
            else:
                # Handle individual parameters
                result = super().join(*args, **kwargs)
                # Check if pollInterval was passed as keyword argument
                self._poll_rate = kwargs.get('pollInterval', 10)
            
            if result:
                self._joined = True
                self._start_polling()
            
            return result
        except Exception as e:
            print(f"Error joining meeting: {e}")
            return False
    
    def _start_polling(self):
        """Start background polling thread for this client"""
        if self._polling_thread and self._polling_thread.is_alive():
            return  # Already running
        
        self._polling_stop_event = threading.Event()
        
        def polling_loop():
            """Background polling loop"""
            while not self._polling_stop_event.is_set():
                try:
                    super(Client, self).poll()
                    # Sleep for poll_rate milliseconds
                    time.sleep(self._poll_rate / 1000.0)
                except Exception as e:
                    print(f"Error during client polling: {e}")
                    break
        
        self._polling_thread = threading.Thread(target=polling_loop, daemon=True)
        self._polling_thread.start()
        print(f"Started client polling with interval: {self._poll_rate}ms")
    
    def _stop_polling(self):
        """Stop background polling thread"""
        if self._polling_stop_event:
            self._polling_stop_event.set()
        
        if self._polling_thread and self._polling_thread.is_alive():
            self._polling_thread.join(timeout=1.0)  # Wait up to 1 second
            print("Stopped client polling")
        
        self._polling_thread = None
        self._polling_stop_event = None
    
    def leave(self) -> bool:
        """Leave the meeting and release resources"""
        try:
            if self._joined:
                self._stop_polling()
                self._joined = False
            
            self.release()
            return True
        except Exception as e:
            print(f"Error leaving meeting: {e}")
            return False
    
    def __del__(self):
        """Cleanup when object is destroyed"""
        if hasattr(self, '_joined') and self._joined:
            self._stop_polling()

# Enhanced global functions with automatic polling

def initialize(ca_path: str = "") -> bool:
    """Initialize the RTMS SDK"""
    return Client.initialize(ca_path)

def uninitialize() -> bool:
    """Uninitialize the RTMS SDK"""
    stop_global_polling()
    return Client.uninitialize()

def join(uuid: str, stream_id: str, signature: str, server_urls: str, timeout: int = -1, poll_interval: int = 10) -> bool:
    """Join a meeting using the global client and start background polling"""
    global _global_poll_rate
    _global_poll_rate = poll_interval
    
    try:
        _join(uuid, stream_id, signature, server_urls, timeout)
        start_global_polling()
        return True
    except Exception as e:
        print(f"Error joining: {e}")
        return False

def leave() -> bool:
    """Leave the meeting using the global client"""
    try:
        stop_global_polling()
        _release()
        return True
    except Exception as e:
        print(f"Error leaving: {e}")
        return False

def poll() -> None:
    """Poll for events using the global client (manual polling)"""
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
        stop_global_polling()
        _release()
        return True
    except Exception as e:
        print(f"Error releasing: {e}")
        return False

# Re-export all the other functions and classes
def enable_audio(enable: bool) -> bool:
    return _enable_audio(enable)

def enable_video(enable: bool) -> bool:
    return _enable_video(enable)

def enable_transcript(enable: bool) -> bool:
    return _enable_transcript(enable)

def enable_deskshare(enable: bool) -> bool:
    return _enable_deskshare(enable)

# Parameter constants/enums (same as before)
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

def generate_signature(client: str, secret: str, meeting_uuid: str, stream_id: str) -> str:
    """Generate RTMS signature"""
    message = f"{client}{meeting_uuid}{stream_id}"
    signature = hmac.new(secret.encode(), message.encode(), hashlib.sha256).hexdigest()
    return signature