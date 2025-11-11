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
    AudioParams, VideoParams, DeskshareParams,

    # Media type constants
    MEDIA_TYPE_AUDIO, MEDIA_TYPE_VIDEO, MEDIA_TYPE_DESKSHARE,
    MEDIA_TYPE_TRANSCRIPT, MEDIA_TYPE_CHAT, MEDIA_TYPE_ALL,

    # Event constants
    SESSION_EVENT_ADD, SESSION_EVENT_STOP, SESSION_EVENT_PAUSE, SESSION_EVENT_RESUME,
    USER_EVENT_JOIN, USER_EVENT_LEAVE,

    # Status code constants
    RTMS_SDK_FAILURE, RTMS_SDK_OK, RTMS_SDK_TIMEOUT,
    RTMS_SDK_NOT_EXIST, RTMS_SDK_WRONG_TYPE,
    RTMS_SDK_INVALID_STATUS, RTMS_SDK_INVALID_ARGS,
    SESS_STATUS_ACTIVE, SESS_STATUS_PAUSED,

    # Parameter dictionaries - import directly with their original names
    AudioContentType, AudioCodec, AudioSampleRate, AudioChannel, AudioDataOption,
    VideoContentType, VideoCodec, VideoResolution, VideoDataOption,
    MediaDataType, SessionState, StreamState, EventType, MessageType, StopReason
)


# Set up logging
_log_level = os.getenv('ZM_RTMS_LOG_LEVEL', 'debug').lower()
_log_format = os.getenv('ZM_RTMS_LOG_FORMAT', 'progressive').lower()
_log_enabled = os.getenv('ZM_RTMS_LOG_ENABLED', 'true').lower() != 'false'


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

    # Check if logging is enabled
    if not _log_enabled:
        return

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
    global _log_level, _log_format, _log_enabled

    if 'level' in options:
        _log_level = options['level']

    if 'format' in options:
        _log_format = options['format']

    if 'enabled' in options:
        _log_enabled = options['enabled']

    log_info('logger', f"Logger configured: level={_log_level}, format={_log_format}, enabled={_log_enabled}")

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

class WebhookResponse:
    """Wrapper for HTTP response to allow custom responses in raw webhook callbacks"""
    def __init__(self, handler):
        self.handler = handler
        self._status = 200
        self._headers = {}
        self._sent = False

    def set_status(self, code):
        """Set HTTP status code"""
        self._status = code

    def set_header(self, name, value):
        """Set HTTP response header"""
        self._headers[name] = value

    def send(self, data):
        """Send response with data"""
        if self._sent:
            return

        self.handler.send_response(self._status)
        for name, value in self._headers.items():
            self.handler.send_header(name, value)
        if 'Content-Type' not in self._headers:
            self.handler.send_header('Content-Type', 'application/json')
        self.handler.end_headers()

        if isinstance(data, dict):
            self.handler.wfile.write(json.dumps(data).encode('utf-8'))
        elif isinstance(data, str):
            self.handler.wfile.write(data.encode('utf-8'))
        elif isinstance(data, bytes):
            self.handler.wfile.write(data)

        self._sent = True

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

            # Check callback arity to determine if it's a raw callback
            import inspect
            sig = inspect.signature(self.server.webhook_callback)
            param_count = len(sig.parameters)

            response = WebhookResponse(self)

            if param_count >= 3:
                # Raw webhook callback - pass payload, request, response
                self.server.webhook_callback(payload, self, response)

                # If callback didn't send response, send default
                if not response._sent:
                    response.send({'status': 'ok'})
            else:
                # Simple webhook callback - just pass payload
                self.server.webhook_callback(payload)

                # Send default success response
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(b'{"status": "ok"}')

        except json.JSONDecodeError as e:
            log_error("webhook", f"Error parsing webhook JSON: {e}")
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Invalid JSON received")
        except Exception as e:
            log_error("webhook", f"Error processing webhook: {e}")
            traceback.print_exc()
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

# Parameter validation functions
def _validate_audio_params(params):
    """Validate audio parameters and provide helpful error messages"""
    if not hasattr(params, '__dict__'):
        return  # Not a parameter object

    errors = []

    # Check codec
    if hasattr(params, 'codec') and params.codec is not None:
        valid_codecs = [v for k, v in AudioCodec.__dict__.items() if not k.startswith('_')]
        if params.codec not in valid_codecs:
            errors.append(
                f"Invalid audio codec: {params.codec}. "
                f"Use rtms.AudioCodec constants (e.g., rtms.AudioCodec.OPUS)"
            )

    # Check sample rate
    if hasattr(params, 'sampleRate') and params.sampleRate is not None:
        valid_rates = [v for k, v in AudioSampleRate.__dict__.items() if not k.startswith('_')]
        if params.sampleRate not in valid_rates:
            errors.append(
                f"Invalid audio sampleRate: {params.sampleRate}. "
                f"Use rtms.AudioSampleRate constants (e.g., rtms.AudioSampleRate.SR_48K)"
            )

    # Check channel
    if hasattr(params, 'channel') and params.channel is not None:
        valid_channels = [v for k, v in AudioChannel.__dict__.items() if not k.startswith('_')]
        if params.channel not in valid_channels:
            errors.append(
                f"Invalid audio channel: {params.channel}. "
                f"Use rtms.AudioChannel constants (e.g., rtms.AudioChannel.STEREO)"
            )

    # Check content type
    if hasattr(params, 'contentType') and params.contentType is not None:
        valid_types = [v for k, v in AudioContentType.__dict__.items() if not k.startswith('_')]
        if params.contentType not in valid_types:
            errors.append(
                f"Invalid audio contentType: {params.contentType}. "
                f"Use rtms.AudioContentType constants (e.g., rtms.AudioContentType.RAW_AUDIO)"
            )

    # Check data option
    if hasattr(params, 'dataOpt') and params.dataOpt is not None:
        valid_opts = [v for k, v in AudioDataOption.__dict__.items() if not k.startswith('_')]
        if params.dataOpt not in valid_opts:
            errors.append(
                f"Invalid audio dataOpt: {params.dataOpt}. "
                f"Use rtms.AudioDataOption constants (e.g., rtms.AudioDataOption.AUDIO_MIXED_STREAM)"
            )

    if errors:
        raise ValueError("\n".join(errors))


def _validate_video_params(params):
    """Validate video parameters and provide helpful error messages"""
    if not hasattr(params, '__dict__'):
        return

    errors = []

    # Check codec
    if hasattr(params, 'codec') and params.codec is not None:
        valid_codecs = [v for k, v in VideoCodec.__dict__.items() if not k.startswith('_')]
        if params.codec not in valid_codecs:
            errors.append(
                f"Invalid video codec: {params.codec}. "
                f"Use rtms.VideoCodec constants (e.g., rtms.VideoCodec.H264)"
            )

    # Check resolution
    if hasattr(params, 'resolution') and params.resolution is not None:
        valid_resolutions = [v for k, v in VideoResolution.__dict__.items() if not k.startswith('_')]
        if params.resolution not in valid_resolutions:
            errors.append(
                f"Invalid video resolution: {params.resolution}. "
                f"Use rtms.VideoResolution constants (e.g., rtms.VideoResolution.HD)"
            )

    # Check content type
    if hasattr(params, 'contentType') and params.contentType is not None:
        valid_types = [v for k, v in VideoContentType.__dict__.items() if not k.startswith('_')]
        if params.contentType not in valid_types:
            errors.append(
                f"Invalid video contentType: {params.contentType}. "
                f"Use rtms.VideoContentType constants (e.g., rtms.VideoContentType.RAW_VIDEO)"
            )

    # Check data option
    if hasattr(params, 'dataOpt') and params.dataOpt is not None:
        valid_opts = [v for k, v in VideoDataOption.__dict__.items() if not k.startswith('_')]
        if params.dataOpt not in valid_opts:
            errors.append(
                f"Invalid video dataOpt: {params.dataOpt}. "
                f"Use rtms.VideoDataOption constants (e.g., rtms.VideoDataOption.VIDEO_SINGLE_ACTIVE_STREAM)"
            )

    if errors:
        raise ValueError("\n".join(errors))


def _validate_deskshare_params(params):
    """Validate deskshare parameters and provide helpful error messages"""
    # Deskshare uses same parameter types as video
    _validate_video_params(params)


class Client(_ClientBase):
    """
    RTMS Client - provides real-time media streaming capabilities

    This client allows you to join Zoom meetings and process audio, video,
    and transcript data in real-time.
    """

    _sdk_initialized = False

    def __init__(self):
        """Initialize a new RTMS client"""
        # Ensure SDK is initialized before creating client instance
        if not Client._sdk_initialized:
            try:
                ca_path = find_ca_certificate()
                log_debug("client", f"Initializing SDK with CA: {ca_path}")
                _ClientBase.initialize(ca_path, 1, "python-rtms")
                Client._sdk_initialized = True
                log_debug("client", "SDK initialized successfully")
            except Exception as e:
                log_error("client", f"SDK initialization failed: {e}")
                # Try with empty path as fallback
                try:
                    log_debug("client", "Trying SDK initialization with empty CA path")
                    _ClientBase.initialize("", 1, "python-rtms")
                    Client._sdk_initialized = True
                    log_debug("client", "SDK initialized with empty CA path")
                except Exception as e2:
                    log_error("client", f"SDK initialization failed completely: {e2}")
                    raise RuntimeError(f"Failed to initialize RTMS SDK: {e2}")

        super().__init__()
        self._polling_thread = None
        self._polling_interval = 10  # milliseconds
        self._running = False
        self._webhook_server = None
        self._main_thread_id = threading.get_ident()  # Remember the main thread ID
        self._join_queue = []  # Queue for pending join requests
        self._join_lock = threading.Lock()  # Lock for join queue

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
        Internal method to join with parameter dictionary.

        IMPORTANT: Due to SDK threading constraints, the actual join() must be called
        from the same thread that initialized the SDK (the main thread). If this is
        called from a different thread (e.g., webhook handler), we queue the request.
        """
        # Check if we're on the main thread
        current_thread_id = threading.get_ident()
        if current_thread_id != self._main_thread_id:
            log_debug("client", f"Join called from non-main thread {current_thread_id}, queuing request")
            # Queue the join request for processing on the main thread
            with self._join_lock:
                self._join_queue.append(params)
            log_debug("client", "Join request queued, will be processed by main thread")
            return True  # Return immediately; actual join happens later

        # We're on the main thread, proceed with join
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
            self._start_polling()

            log_info("client", "Successfully joined meeting")
            return True
        except Exception as e:
            log_error("client", f"Error joining meeting: {e}")
            traceback.print_exc()
            return False

    def _process_join_queue(self):
        """Process any pending join requests from the queue"""
        with self._join_lock:
            if not self._join_queue:
                return
            # Process all pending requests
            requests = self._join_queue[:]
            self._join_queue.clear()

        for params in requests:
            log_debug("client", "Processing queued join request")
            self._join_with_params(**params)

    def _initialize_rtms(self, ca_path=None):
        """Initialize the RTMS SDK with the best available CA certificate"""
        try:
            # Find the best CA certificate
            ca_path = find_ca_certificate(ca_path)

            # Initialize the SDK
            log_debug("client", f"Initializing RTMS with CA: {ca_path}")
            _ClientBase.initialize(ca_path)
            return True
        except Exception as e:
            log_error("client", f"Error initializing RTMS: {e}")
            traceback.print_exc()
            # Try with an empty path as a last resort
            try:
                log_debug("client", "Trying initialization with empty CA path")
                _ClientBase.initialize("")
                return True
            except Exception as e2:
                log_error("client", f"Failed to initialize with empty CA path: {e2}")
                raise e  # Raise the original error

    def _poll_if_needed(self):
        """
        Poll the RTMS client if needed.

        IMPORTANT: Due to SDK threading constraints, poll() must be called from the
        main thread (same thread that initialized the SDK). This should be called
        periodically from the main loop.
        """
        if self._running:
            try:
                super().poll()
            except Exception as e:
                log_error("client", f"Error during polling: {e}")

    def _start_polling(self):
        """Mark that polling should begin (will be done from main thread)"""
        self._running = True
        log_debug("client", "Polling enabled - call _poll_if_needed() from main loop")

    def _stop_polling(self):
        """Stop polling"""
        self._running = False
        log_debug("client", "Polling stopped")

    def stop(self):
        """
        Stop RTMS client and release resources.

        This is equivalent to calling leave(), but with a more intuitive name.
        """
        return self.leave()

    def setAudioParams(self, params):
        """
        Set audio parameters with validation.

        Args:
            params (AudioParams): Audio parameters object

        Returns:
            bool: True if parameters were set successfully

        Raises:
            ValueError: If parameters are invalid
        """
        _validate_audio_params(params)
        return super().setAudioParams(params)

    def setVideoParams(self, params):
        """
        Set video parameters with validation.

        Args:
            params (VideoParams): Video parameters object

        Returns:
            bool: True if parameters were set successfully

        Raises:
            ValueError: If parameters are invalid
        """
        _validate_video_params(params)
        return super().setVideoParams(params)

    def setDeskshareParams(self, params):
        """
        Set deskshare parameters with validation.

        Args:
            params (DeskshareParams): Deskshare parameters object

        Returns:
            bool: True if parameters were set successfully

        Raises:
            ValueError: If parameters are invalid
        """
        _validate_deskshare_params(params)
        return super().setDeskshareParams(params)

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
            super().release()
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
_global_client = None

def _get_global_client():
    """Get or create the global client instance"""
    global _global_client
    if _global_client is None:
        _global_client = Client()
    return _global_client

def join(*args, **kwargs):
    """
    Join a RTMS session using the global client.

    See Client.join for documentation.
    """
    return _get_global_client().join(*args, **kwargs)

def leave():
    """
    Leave the RTMS session using the global client.

    See Client.leave for documentation.
    """
    return _get_global_client().leave()

def setAudioParams(params):
    """
    Set audio parameters for the global client.

    Args:
        params (AudioParams): Audio parameters object

    Raises:
        ValueError: If parameters are invalid
    """
    _validate_audio_params(params)
    return _get_global_client().setAudioParams(params)

def setVideoParams(params):
    """
    Set video parameters for the global client.

    Args:
        params (VideoParams): Video parameters object

    Raises:
        ValueError: If parameters are invalid
    """
    _validate_video_params(params)
    return _get_global_client().setVideoParams(params)

def setDeskshareParams(params):
    """
    Set deskshare parameters for the global client.

    Args:
        params (DeskshareParams): Deskshare parameters object

    Raises:
        ValueError: If parameters are invalid
    """
    _validate_deskshare_params(params)
    return _get_global_client().setDeskshareParams(params)

def uninitialize():
    """
    Uninitialize the RTMS SDK.
    """
    try:
        _ClientBase.uninitialize()
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
    return _get_global_client().on_webhook_event(*args, **kwargs)

# Create proper decorator functions for global client
def onJoinConfirm(func):
    """Decorator for join confirmation callback"""
    _get_global_client().onJoinConfirm(func)
    return func

def onSessionUpdate(func):
    """Decorator for session update callback"""
    _get_global_client().onSessionUpdate(func)
    return func

def onUserUpdate(func):
    """Decorator for user update callback"""
    _get_global_client().onUserUpdate(func)
    return func

def onAudioData(func):
    """Decorator for audio data callback"""
    _get_global_client().onAudioData(func)
    return func

def onVideoData(func):
    """Decorator for video data callback"""
    _get_global_client().onVideoData(func)
    return func

def onDeskshareData(func):
    """Decorator for deskshare data callback"""
    _get_global_client().onDeskshareData(func)
    return func

def onTranscriptData(func):
    """Decorator for transcript data callback"""
    _get_global_client().onTranscriptData(func)
    return func

def onLeave(func):
    """Decorator for leave callback"""
    _get_global_client().onLeave(func)
    return func

def onEventEx(func):
    """Decorator for extended event callback"""
    _get_global_client().onEventEx(func)
    return func

def initialize(ca_path=None):
    """
    Initialize the RTMS SDK.

    Args:
        ca_path (str, optional): Path to the CA certificate file.
    """
    ca_path = find_ca_certificate(ca_path)
    try:
        _ClientBase.initialize(ca_path)
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
    "AudioParams",
    "VideoParams",
    "DeskshareParams",
    "LogLevel",
    "LogFormat",

    # Constants - Media Types
    "MEDIA_TYPE_AUDIO",
    "MEDIA_TYPE_VIDEO",
    "MEDIA_TYPE_DESKSHARE",
    "MEDIA_TYPE_TRANSCRIPT",
    "MEDIA_TYPE_CHAT",
    "MEDIA_TYPE_ALL",

    # Constants - Events
    "SESSION_EVENT_ADD",
    "SESSION_EVENT_STOP",
    "SESSION_EVENT_PAUSE",
    "SESSION_EVENT_RESUME",
    "USER_EVENT_JOIN",
    "USER_EVENT_LEAVE",

    # Constants - Status Codes
    "RTMS_SDK_FAILURE",
    "RTMS_SDK_OK",
    "RTMS_SDK_TIMEOUT",
    "RTMS_SDK_NOT_EXIST",
    "RTMS_SDK_WRONG_TYPE",
    "RTMS_SDK_INVALID_STATUS",
    "RTMS_SDK_INVALID_ARGS",
    "SESS_STATUS_ACTIVE",
    "SESS_STATUS_PAUSED",

    # Parameter dictionaries
    "AudioContentType",
    "AudioCodec",
    "AudioSampleRate",
    "AudioChannel",
    "AudioDataOption",
    "VideoContentType",
    "VideoCodec",
    "VideoResolution",
    "VideoDataOption",
    "MediaDataType",
    "SessionState",
    "StreamState",
    "EventType",
    "MessageType",
    "StopReason",

    # Core functions
    "join",
    "leave",
    "initialize",
    "uninitialize",
    "uuid",
    "stream_id",
    "generate_signature",
    "setAudioParams",
    "setVideoParams",
    "setDeskshareParams",

    # Logging functions
    "log_debug",
    "log_info",
    "log_warn",
    "log_error",
    "configure_logger",

    # Callback decorators
    "on_webhook_event",
    "onJoinConfirm",
    "onSessionUpdate",
    "onUserUpdate",
    "onAudioData",
    "onVideoData",
    "onDeskshareData",
    "onTranscriptData",
    "onLeave",
    "onEventEx"
]