import os
import json
import hmac
import hashlib
import threading
import time
import sys
import traceback
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Callable, Dict, Any, Optional, Union
from enum import IntEnum, Enum
from pathlib import Path

from ._rtms import (
    # Classes
    Client as _ClientBase, Session, Participant, Metadata,
    
    # Main functions
    _initialize, _uninitialize, 
    
    # Media type constants
    SDK_AUDIO, SDK_VIDEO, SDK_TRANSCRIPT, SDK_ALL,
    
    # Event constants
    SESSION_ADD, SESSION_STOP, SESSION_PAUSE, SESSION_RESUME,
    USER_JOIN, USER_LEAVE,
    
    # Status code constants
    RTMS_SDK_FAILURE, RTMS_SDK_OK, RTMS_SDK_TIMEOUT,
    RTMS_SDK_NOT_EXIST, RTMS_SDK_WRONG_TYPE,
    RTMS_SDK_INVALID_STATUS, RTMS_SDK_INVALID_ARGS,
    SESS_STATUS_ACTIVE, SESS_STATUS_PAUSED,
    
    # Parameter setting functions
    set_audio_parameters,
    set_video_parameters,
    
    # Parameter dictionaries - import directly with their original names
    AudioContentType, AudioCodec, AudioSampleRate, AudioChannel, AudioDataOption,
    VideoContentType, VideoCodec, VideoResolution, VideoDataOption,
    MediaDataType, SessionState, StreamState, EventType, MessageType, StopReason
)


# Set up logging
_log_level = os.getenv('ZM_RTMS_LOG_LEVEL', 'debug').lower()
_log_format = os.getenv('ZM_RTMS_LOG_FORMAT', 'progressive').lower()


class LogLevel(IntEnum):
    """Available log levels for RTMS SDK logging"""
    ERROR = 0
    WARN = 1
    INFO = 2
    DEBUG = 3
    TRACE = 4

class LogFormat(str, Enum):
    """Available log output formats"""
    PROGRESSIVE = 'progressive'
    JSON = 'json'

# Global webhook server
_webhook_server = None

def _log(level, component, message, details=None):
    """Log a message at the specified level"""

    # Convert level name to enum value if it's a string
    if isinstance(level, str):
        level_map = {
            'error': LogLevel.ERROR,
            'warn': LogLevel.WARN,
            'info': LogLevel.INFO,
            'debug': LogLevel.DEBUG,
            'trace': LogLevel.TRACE
        }
        level = level_map.get(level.lower(), LogLevel.INFO)
    
    # Check if we should log this level
    target_level = LogLevel[_log_level.upper()] if _log_level.upper() in LogLevel.__members__ else LogLevel.INFO
    if level > target_level:
        return
    
    # Format the log message
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    level_names = ['ERROR', 'WARN', 'INFO', 'DEBUG', 'TRACE']
    
    if _log_format == 'json':
        log_data = {
            'timestamp': timestamp,
            'level': level_names[level].lower(),
            'component': component,
            'message': message
        }
        if details:
            log_data['details'] = details
        log_str = json.dumps(log_data)
    else:
        # Progressive format
        padded_component = component.ljust(8)
        padded_level = level_names[level].ljust(5)
        log_str = f"{padded_component} | {timestamp} | {padded_level} | {message}"
    
    # Output to the appropriate stream
    if level <= LogLevel.ERROR:
        print(log_str, file=sys.stderr)
    else:
        print(log_str)

def log_debug(component, message, details=None):
    """Log a debug message"""
    _log(LogLevel.DEBUG, component, message, details)

def log_info(component, message, details=None):
    """Log an info message"""
    _log(LogLevel.INFO, component, message, details)

def log_warn(component, message, details=None):
    """Log a warning message"""
    _log(LogLevel.WARN, component, message, details)

def log_error(component, message, details=None):
    """Log an error message"""
    _log(LogLevel.ERROR, component, message, details)

def configure_logger(options: dict):
    """Configure the logger with the specified options"""
    global _log_level, _log_format
    
    if 'level' in options:
        _log_level = options['level']
    
    if 'format' in options:
        _log_format = options['format']
    
    log_info('logger', f"Logger configured: level={_log_level}, format={_log_format}")

    log_debug("rtms", "Importing _rtms module")

# Find a suitable CA certificate file for SSL verification
def find_ca_certificate(specified_path=None):
    """
    Finds a suitable CA certificate file for SSL verification
    
    Args:
        specified_path: Optional explicit path to a CA certificate
    
    Returns:
        Path to a CA certificate or empty string if none found
    """
    # Use explicit path if provided
    if specified_path and os.path.exists(specified_path):
        log_debug('rtms', f"Using specified CA certificate: {specified_path}")
        return specified_path
    
    # Check environment variable
    env_path = os.getenv('ZM_RTMS_CA', '')
    if env_path and os.path.exists(env_path):
        log_debug('rtms', f"Using CA certificate from environment variable: {env_path}")
        return env_path
    
    # Try common system certificate locations
    common_locations = [
        '/etc/ssl/certs/ca-certificates.crt',  # Debian/Ubuntu
        '/etc/pki/tls/certs/ca-bundle.crt',    # Fedora/RHEL
        '/etc/ssl/ca-bundle.pem',              # OpenSUSE
        '/etc/pki/tls/cacert.pem',             # CentOS
        '/etc/ssl/cert.pem',                   # Alpine/macOS
        '/usr/local/etc/openssl/cert.pem',     # macOS Homebrew
    ]
    
    for location in common_locations:
        if os.path.exists(location):
            log_debug('rtms', f"Found system CA certificate: {location}")
            return location
    
    log_warn('rtms', 'No CA certificate found, operation may fail')
    return ''

def generate_signature(client, secret, uuid, rtms_stream_id):
    """Generate a signature for RTMS authentication"""
    client_id = os.getenv("ZM_RTMS_CLIENT", client)
    client_secret = os.getenv("ZM_RTMS_SECRET", secret)

    if not client_id:
        raise EnvironmentError("Client ID cannot be blank")
    elif not client_secret:
        raise EnvironmentError("Client Secret cannot be blank")

    message = f"{client_id},{uuid},{rtms_stream_id}"

    signature = hmac.new(
        client_secret.encode('utf-8'),
        message.encode('utf-8'),
        hashlib.sha256
    ).hexdigest()

    return signature

class WebhookHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # Validate request path
        if self.path != self.server.webhook_path:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")
            return

        # Read and parse request body
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length)

        try:
            payload = json.loads(body.decode('utf-8'))
            log_debug(f"webhook", f"Received webhook payload: {payload}")
            self.server.webhook_callback(payload) 

            self.send_response(200)
            self.end_headers()
            self.wfile.write(b'Webhook received successfully')
        except json.JSONDecodeError as e:
            log_error("webhook", f"Error parsing webhook JSON: {e}")
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Invalid JSON received")
        except Exception as e:
            log_error("webhook", f"Error processing webhook: {e}")
            self.send_response(500)
            self.end_headers()
            self.wfile.write(f"Internal error: {str(e)}".encode('utf-8'))
    
    def log_message(self, format, *args):
        # Override to suppress default logging
        pass

class WebhookServer:
    def __init__(self, port=8080, path='/'):
        self.port = port
        self.path = path
        self.server = None
        self.server_thread = None
        self.callback = None
    
    def start(self, callback):
        if self.server:
            return False
        
        try:
            class CustomWebhookServer(HTTPServer):
                def __init__(self, server_address, RequestHandlerClass, webhook_callback, webhook_path):
                    super().__init__(server_address, RequestHandlerClass)
                    self.webhook_callback = webhook_callback
                    self.webhook_path = webhook_path
            
            self.callback = callback
            self.server = CustomWebhookServer(('0.0.0.0', self.port), WebhookHandler, callback, self.path)
            
            log_debug("webhook", f"Starting webhook server on port {self.port} path {self.path}")
            self.server_thread = threading.Thread(target=self.server.serve_forever, daemon=True)
            self.server_thread.start()
            
            return True
        except Exception as e:
            log_error("webhook", f"Error starting webhook server: {e}")
            traceback.print_exc()
            return False
    
    def stop(self):
        if not self.server:
            return
         
        try:
            log_debug("webhook", "Shutting down webhook server")
            self.server.shutdown()
            self.server.server_close()
            self.server = None
            self.server_thread = None
        except Exception as e:
            log_error("webhook", f"Error shutting down webhook server: {e}")

class Client(_ClientBase):
    """
    RTMS Client - provides real-time media streaming capabilities
    
    This client allows you to join Zoom meetings and process audio, video, 
    and transcript data in real-time.
    """
    
    def __init__(self):
        """Initialize a new RTMS client"""
        super().__init__()
        self._polling_thread = None
        self._polling_interval = 10  # milliseconds
        self._running = False
        self._webhook_server = None
        
    def join(self, 
             meeting_uuid: str = None, 
             rtms_stream_id: str = None, 
             server_urls: str = None, 
             signature: str = None, 
             timeout: int = -1, 
             ca: str = None, 
             client: str = None, 
             secret: str = None,
             poll_interval: int = 10,
             **kwargs):
        """
        Join a RTMS session.
        
        Can be called with positional arguments or with a dictionary of parameters.
        
        Args:
            meeting_uuid (str): Meeting UUID
            rtms_stream_id (str): RTMS stream ID
            server_urls (str): Server URLs (comma-separated)
            signature (str, optional): Authentication signature. If not provided, will be generated
            timeout (int, optional): Timeout in milliseconds. Defaults to -1 (no timeout).
            ca (str, optional): CA certificate path. Defaults to system CA files.
            client (str, optional): Client ID. If empty, uses ZM_RTMS_CLIENT env var.
            secret (str, optional): Client secret. If empty, uses ZM_RTMS_SECRET env var.
            poll_interval (int, optional): Polling interval in milliseconds. Defaults to 10.
            **kwargs: Additional arguments passed to join
            
        Returns:
            bool: True if joined successfully, False otherwise
        """

        try:
            # Support for both dictionary-style and parameter-style calls
            if len(kwargs) > 0:
                # If additional kwargs are provided, merge them with the named parameters
                params = {
                    'meeting_uuid': meeting_uuid,
                    'rtms_stream_id': rtms_stream_id,
                    'server_urls': server_urls,
                    'signature': signature,
                    'timeout': timeout,
                    'ca': ca,
                    'client': client,
                    'secret': secret,
                    'poll_interval': poll_interval
                }
                # Update with any additional kwargs
                params.update(kwargs)
                return self._join_with_params(**params)
            
            # Check if uuid is actually a dictionary (first param)
            if isinstance(meeting_uuid, dict):
                return self._join_with_params(**meeting_uuid)
                
            # Otherwise, use the parameters directly
            return self._join_with_params(
                meeting_uuid=meeting_uuid,
                rtms_stream_id=rtms_stream_id,
                server_urls=server_urls,
                signature=signature,
                timeout=timeout,
                ca=ca,
                client=client,
                secret=secret,
                poll_interval=poll_interval
            )
        except Exception as e:
            log_error("client", f"Error in join: {e}")
            traceback.print_exc()
            return False
    
    def _join_with_params(self, **params):
        """
        Internal method to join with parameter dictionary
        """
        try:
            # Extract parameters with defaults
            meeting_uuid = params.get('meeting_uuid')
            rtms_stream_id = params.get('rtms_stream_id')
            server_urls = params.get('server_urls')
            signature = params.get('signature')
            timeout = params.get('timeout', -1)
            ca = params.get('ca')
            client = params.get('client', os.getenv('ZM_RTMS_CLIENT')) 
            secret = params.get('secret', os.getenv('ZM_RTMS_SECRET')) 
            poll_interval = params.get('poll_interval', 10)
            

            if not meeting_uuid:
                raise ValueError("Meeting UUID is required")
            if not rtms_stream_id:
                raise ValueError("RTMS Stream ID is required")
            if not server_urls:
                raise ValueError("Server URLs is required")
                
            # Initialize RTMS if not already initialized
            self._initialize_rtms(ca)
            
            # Generate signature if not provided
            if not signature:
                try:
                    signature = generate_signature(client, secret, meeting_uuid, rtms_stream_id)
                except Exception as e:
                    log_error("client", f"Error generating signature: {e}")
                    raise
            
            # Store polling interval
            self._polling_interval = poll_interval
            
            # Join the meeting
            log_info("client", f"Joining meeting: {meeting_uuid}")
            super().join(meeting_uuid, rtms_stream_id, signature, server_urls, timeout)
            
            # Start polling thread
            self.poll()
            
            log_info("client", "Successfully joined meeting")
            return True
        except Exception as e:
            log_error("client", f"Error joining meeting: {e}")
            traceback.print_exc()
            return False
    
    def _initialize_rtms(self, ca_path=None):
        """Initialize the RTMS SDK with the best available CA certificate"""
        try:
            # Find the best CA certificate
            ca_path = find_ca_certificate(ca_path)
            
            # Initialize the SDK
            log_debug("client", f"Initializing RTMS with CA: {ca_path}")
            _initialize(ca_path)
            return True
        except Exception as e:
            log_error("client", f"Error initializing RTMS: {e}")
            traceback.print_exc()
            # Try with an empty path as a last resort
            try:
                log_debug("client", "Trying initialization with empty CA path")
                _initialize("")
                return True
            except Exception as e2:
                log_error("client", f"Failed to initialize with empty CA path: {e2}")
                raise e  # Raise the original error
    
    def _polling_worker(self):
        """Background thread for polling RTMS client"""
        log_debug("client", "Starting polling worker thread")
        
        threading.current_thread().name = "RTMS_Polling_Thread"
        
        while self._running:
            try:
                # Use a shorter timeout to ensure responsive shutdown
                for _ in range(10):  # Split sleep into smaller chunks for more responsive shutdown
                    if not self._running:
                        break
                    time.sleep(self._polling_interval / 1000 / 10)  # Convert to seconds and divide by 10
                
                if not self._running:
                    break
                    
                # Call poll if we're still running
                self.poll()
            except Exception as e:
                log_error("client", f"Error during polling: {e}")
                # Don't break the loop on errors, just log and continue
                time.sleep(0.5)  # Add small delay to avoid tight loop if there are persistent errors
                
        log_debug("client", "Polling worker thread stopped")
    
    def _start_polling(self):
        """Start the polling thread"""
        if self._polling_thread and self._polling_thread.is_alive():
            return
        
        self._running = True
        self._polling_thread = threading.Thread(target=self._polling_worker)
        self._polling_thread.daemon = True
        self._polling_thread.start()
    
    def _stop_polling(self):
        """Stop the polling thread"""
        self._running = False
        if self._polling_thread and self._polling_thread.is_alive():
            try:
                self._polling_thread.join(timeout=1)
            except Exception as e:
                log_error("client", f"Error stopping polling thread: {e}")
    
    def stop(self):
        """
        Stop RTMS client and release resources.
        
        This is equivalent to calling leave(), but with a more intuitive name.
        """
        return self.leave()
    
    def leave(self):
        """
        Leave the RTMS session and stop all threads.
        
        Returns:
            bool: True if left successfully
        """
        log_info("client", "Leaving RTMS session")
        
        # Stop polling thread
        self._stop_polling()
        
        # Stop webhook server if we have one
        if self._webhook_server:
            self._webhook_server.stop()
            self._webhook_server = None
        
        try:
            # Release RTMS resources
            self.release()
            return True
        except Exception as e:
            log_error("client", f"Error releasing RTMS resources: {e}")
            traceback.print_exc()
            return False
    
    def on_webhook_event(self, callback=None, port=None, path=None):
        """
        Register a webhook event handler.
        
        This can be used as a decorator or a direct method call:
        
        @client.on_webhook_event(port=8080, path='/webhook')
        def handle_webhook(payload):
            print(f"Received webhook: {payload}")
            
        Args:
            callback (callable, optional): Function to call when a webhook is received
            port (int, optional): Port to listen on. Defaults to ZM_RTMS_PORT env var or 8080
            path (str, optional): URL path to listen on. Defaults to ZM_RTMS_PATH env var or '/'
            
        Returns:
            callable: Decorator function if used as a decorator
        """
        # If used as a decorator without arguments
        if callback is not None and callable(callback):
            # Start webhook server with provided callback
            self._start_webhook_server(callback)
            return callback
        
        # If used as a decorator with arguments or as a method call
        def decorator(func):
            self._start_webhook_server(func, port, path)
            return func
        
        return decorator
    
    def _start_webhook_server(self, callback, port=None, path=None):
        """
        Start the webhook server
        
        Args:
            callback (callable): Function to call when a webhook is received
            port (int, optional): Port to listen on. Defaults to ZM_RTMS_PORT env var or 8080
            path (str, optional): URL path to listen on. Defaults to ZM_RTMS_PATH env var or '/'
        """
        if not self._webhook_server:
            port = port or int(os.getenv('ZM_RTMS_PORT', '8080'))
            path = path or os.getenv('ZM_RTMS_PATH', '/')
            
            self._webhook_server = WebhookServer(port, path)
        
        self._webhook_server.start(callback)
        print(f"🚀 Listening for webhook events at http://localhost:{self._webhook_server.port}{self._webhook_server.path}")

# For backward compatibility, create a global client and expose its methods
_global_client = Client()

def join(*args, **kwargs):
    """
    Join a RTMS session using the global client.
    
    See Client.join for documentation.
    """
    return _global_client.join(*args, **kwargs)

def leave():
    """
    Leave the RTMS session using the global client.
    
    See Client.leave for documentation.
    """
    return _global_client.leave()

def set_audio_parameters(params):
    """
    Set audio parameters for the global client.
    
    Args:
        params (dict): Dictionary of audio parameters
    """
    return _global_client.set_audio_parameters(params)

def set_video_parameters(params):
    """
    Set video parameters for the global client.
    
    Args:
        params (dict): Dictionary of video parameters
    """
    return _global_client.set_video_parameters(params)

def uninitialize():
    """
    Uninitialize the RTMS SDK.
    """
    try:
        _uninitialize()
        return True
    except Exception as e:
        log_error("rtms", f"Error uninitializing RTMS SDK: {e}")
        traceback.print_exc()
        return False

def uuid():
    """
    Get the UUID of the current meeting using the global client.
    """
    try:
        return _global_client.uuid()
    except Exception as e:
        log_error("rtms", f"Error getting UUID: {e}")
        return ""

def stream_id():
    """
    Get the stream ID of the current meeting using the global client.
    """
    try:
        return _global_client.stream_id()
    except Exception as e:
        log_error("rtms", f"Error getting stream ID: {e}")
        return ""

def on_webhook_event(*args, **kwargs):
    """
    Register a webhook event handler using the global client.
    
    See Client.on_webhook_event for documentation.
    """
    return _global_client.on_webhook_event(*args, **kwargs)

# Create proper decorator functions for global client
def on_join_confirm(func):
    """Decorator for join confirmation callback"""
    _global_client.set_join_confirm_callback(func)
    return func

def on_session_update(func):
    """Decorator for session update callback"""
    _global_client.set_session_update_callback(func)
    return func

def on_user_update(func):
    """Decorator for user update callback"""
    _global_client.set_user_update_callback(func)
    return func

def on_audio_data(func):
    """Decorator for audio data callback"""
    _global_client.set_audio_data_callback(func)
    return func

def on_video_data(func):
    """Decorator for video data callback"""
    _global_client.set_video_data_callback(func)
    return func

def on_transcript_data(func):
    """Decorator for transcript data callback"""
    _global_client.set_transcript_data_callback(func)
    return func

def on_leave(func):
    """Decorator for leave callback"""
    _global_client.set_leave_callback(func)
    return func

def initialize(ca_path=None):
    """
    Initialize the RTMS SDK.
    
    Args:
        ca_path (str, optional): Path to the CA certificate file.
    """
    ca_path = find_ca_certificate(ca_path)
    try:
        _initialize(ca_path)
        return True
    except Exception as e:
        log_error("rtms", f"Error initializing RTMS SDK: {e}")
        return False

__all__ = [
    # Classes
    "Client",
    "Session",
    "Participant",
    "Metadata",
    "LogLevel",
    "LogFormat",
    "MediaType",
    "SessionEvent",
    "UserEvent",
    "SessionStatus",
    
    # Constants
    "SDK_AUDIO",
    "SDK_VIDEO",
    "SDK_TRANSCRIPT",
    "SDK_ALL",
    "SESSION_ADD",
    "SESSION_STOP",
    "SESSION_PAUSE",
    "SESSION_RESUME",
    "USER_JOIN",
    "USER_LEAVE",
    "RTMS_SDK_FAILURE",
    "RTMS_SDK_OK",
    "RTMS_SDK_TIMEOUT",
    "RTMS_SDK_NOT_EXIST",
    "RTMS_SDK_WRONG_TYPE",
    "RTMS_SDK_INVALID_STATUS",
    "RTMS_SDK_INVALID_ARGS",
    "SESS_STATUS_ACTIVE",
    "SESS_STATUS_PAUSED",
    
    # Core functions
    "join",
    "leave",
    "initialize",
    "uninitialize",
    "uuid", 
    "stream_id",
    "generate_signature",
    "set_audio_parameters",
    "set_video_parameters",
    
    # Logging functions
    "log_debug",
    "log_info",
    "log_warn",
    "log_error",
    
    # Callback decorators
    "on_webhook_event",
    "on_join_confirm",
    "on_session_update",
    "on_user_update",
    "on_audio_data",
    "on_video_data",
    "on_transcript_data",
    "on_leave"
]