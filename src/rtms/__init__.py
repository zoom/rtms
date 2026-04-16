import os
import json
import hmac
import hashlib
import threading
import time
import sys
import traceback
import asyncio
import inspect
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Callable, Dict, Any, Optional, Union, List, Tuple
from enum import IntEnum, Enum
from pathlib import Path

from ._rtms import (
    # Classes
    Client as _ClientBase, Session, Participant, Metadata,
    AiTargetLanguage, AiInterpreter,
    AudioParams, VideoParams, DeskshareParams, TranscriptParams,

    # Media type constants
    MEDIA_TYPE_AUDIO, MEDIA_TYPE_VIDEO, MEDIA_TYPE_DESKSHARE,
    MEDIA_TYPE_TRANSCRIPT, MEDIA_TYPE_CHAT, MEDIA_TYPE_ALL,

    # Session event constants
    SESSION_EVENT_ADD, SESSION_EVENT_STOP, SESSION_EVENT_PAUSE, SESSION_EVENT_RESUME,

    # User event constants
    USER_JOIN, USER_LEAVE,

    # Event types for subscribeEvent/unsubscribeEvent (used with onEventEx callback)
    # These match EVENT_TYPE from Zoom's C SDK
    EVENT_UNDEFINED, EVENT_FIRST_PACKET_TIMESTAMP,
    EVENT_ACTIVE_SPEAKER_CHANGE, EVENT_PARTICIPANT_JOIN, EVENT_PARTICIPANT_LEAVE,
    EVENT_SHARING_START, EVENT_SHARING_STOP,
    EVENT_MEDIA_CONNECTION_INTERRUPTED,
    EVENT_PARTICIPANT_VIDEO_ON, EVENT_PARTICIPANT_VIDEO_OFF,
    EVENT_CONSUMER_ANSWERED, EVENT_CONSUMER_END,
    EVENT_USER_ANSWERED, EVENT_USER_END, EVENT_USER_HOLD, EVENT_USER_UNHOLD,

    # Status code constants
    RTMS_SDK_FAILURE, RTMS_SDK_OK, RTMS_SDK_TIMEOUT,
    RTMS_SDK_NOT_EXIST, RTMS_SDK_WRONG_TYPE,
    RTMS_SDK_INVALID_STATUS, RTMS_SDK_INVALID_ARGS,
    SESS_STATUS_ACTIVE, SESS_STATUS_PAUSED,

    # Parameter dictionaries - imported as private names, converted to IntEnum below
    AudioContentType as _AudioContentType,
    AudioCodec as _AudioCodec,
    AudioSampleRate as _AudioSampleRate,
    AudioChannel as _AudioChannel,
    DataOption as _DataOption,
    VideoContentType as _VideoContentType,
    VideoCodec as _VideoCodec,
    VideoResolution as _VideoResolution,
    MediaDataType as _MediaDataType,
    SessionState as _SessionState,
    StreamState as _StreamState,
    EventType as _EventType,
    MessageType as _MessageType,
    StopReason as _StopReason,
    TranscriptLanguage as _TranscriptLanguage,
)

# Convert raw C++ dicts to IntEnum for Pythonic dot-notation access
AudioContentType  = IntEnum("AudioContentType",  _AudioContentType)
AudioCodec        = IntEnum("AudioCodec",         _AudioCodec)
AudioSampleRate   = IntEnum("AudioSampleRate",    _AudioSampleRate)
AudioChannel      = IntEnum("AudioChannel",       _AudioChannel)
DataOption        = IntEnum("DataOption",         _DataOption)
AudioDataOption   = DataOption  # legacy alias
VideoContentType  = IntEnum("VideoContentType",   _VideoContentType)
VideoCodec        = IntEnum("VideoCodec",         _VideoCodec)
VideoResolution   = IntEnum("VideoResolution",    _VideoResolution)
VideoDataOption   = DataOption  # legacy alias
MediaDataType     = IntEnum("MediaDataType",      _MediaDataType)
SessionState      = IntEnum("SessionState",       _SessionState)
StreamState       = IntEnum("StreamState",        _StreamState)
EventType         = IntEnum("EventType",          _EventType)
MessageType       = IntEnum("MessageType",        _MessageType)
StopReason        = IntEnum("StopReason",         _StopReason)
TranscriptLanguage = IntEnum("TranscriptLanguage", _TranscriptLanguage)


# Set up logging
_log_level = os.getenv('ZM_RTMS_LOG_LEVEL', 'info').lower()
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

# Global client registry (for status/monitoring — len(rtms._clients))
_clients: Dict[int, 'Client'] = {}
_clients_lock = threading.Lock()
_sdk_init_lock = threading.Lock()
_running = False
_stop_event = threading.Event()
_run_executor: Optional['Executor'] = None

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

            # Validate required webhook fields
            if not isinstance(payload.get('event'), str):
                log_warn("webhook", "Received webhook payload missing required 'event' field")
                self.send_response(400)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(b'{"error": "Invalid webhook payload: missing required event field"}')
                return

            event_type = payload.get('event', 'unknown')
            log_info("webhook", f"Received event: {event_type}")
            log_debug("webhook", f"Received webhook payload: {payload}")

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

            log_info("webhook", f"Listening for webhook events at http://localhost:{self.port}{self.path}")
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
    """
    Validate audio parameters and provide helpful error messages

    Note: AudioParams now has sensible defaults, so users can omit setAudioParams()
    or change individual values without setting all fields.
    """
    if not hasattr(params, '__dict__'):
        return  # Not a parameter object

    errors = []

    # Check required fields are set (non-zero)
    if hasattr(params, 'contentType') and params.contentType == 0:
        errors.append(
            "AudioParams: contentType must be set (e.g., rtms.AudioContentType.RAW_AUDIO)"
        )

    if hasattr(params, 'codec') and params.codec == 0:
        errors.append(
            "AudioParams: codec must be set (e.g., rtms.AudioCodec.OPUS)"
        )

    if hasattr(params, 'channel') and params.channel == 0:
        errors.append(
            "AudioParams: channel must be set (e.g., rtms.AudioChannel.STEREO)"
        )

    if hasattr(params, 'dataOpt') and params.dataOpt == 0:
        errors.append(
            "AudioParams: dataOpt must be set (e.g., rtms.AudioDataOption.AUDIO_MULTI_STREAMS)"
        )

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
        valid_opts = [v for k, v in DataOption.__dict__.items() if not k.startswith('_')]
        if params.dataOpt not in valid_opts:
            errors.append(
                f"Invalid audio dataOpt: {params.dataOpt}. "
                f"Use rtms.DataOption constants (e.g., rtms.DataOption.AUDIO_MULTI_STREAMS)"
            )

    # Validate codec-specific requirements
    if hasattr(params, 'codec') and params.codec == AudioCodec.OPUS:
        if hasattr(params, 'sampleRate') and params.sampleRate != AudioSampleRate.SR_48K:
            errors.append(
                f"AudioParams: OPUS codec requires 48kHz sample rate (rtms.AudioSampleRate.SR_48K). "
                f"Current value: {params.sampleRate}"
            )

    # Validate frame size matches sample rate and duration
    if hasattr(params, 'duration') and hasattr(params, 'frameSize'):
        if params.duration > 0 and params.frameSize > 0:
            # Calculate expected frame size based on sample rate
            sample_rate_map = {
                AudioSampleRate.SR_8K: 8000,
                AudioSampleRate.SR_16K: 16000,
                AudioSampleRate.SR_32K: 32000,
                AudioSampleRate.SR_48K: 48000,
            }

            if hasattr(params, 'sampleRate') and params.sampleRate in sample_rate_map:
                sample_rate_hz = sample_rate_map[params.sampleRate]
                expected_frame_size = sample_rate_hz * params.duration // 1000

                if params.frameSize != expected_frame_size:
                    errors.append(
                        f"AudioParams: frameSize ({params.frameSize}) does not match sampleRate and duration "
                        f"(expected {expected_frame_size} for {params.duration}ms)"
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
        valid_opts = [v for k, v in DataOption.__dict__.items() if not k.startswith('_')]
        if params.dataOpt not in valid_opts:
            errors.append(
                f"Invalid video dataOpt: {params.dataOpt}. "
                f"Use rtms.DataOption constants (e.g., rtms.DataOption.VIDEO_SINGLE_ACTIVE_STREAM)"
            )

    if errors:
        raise ValueError("\n".join(errors))


def _validate_deskshare_params(params):
    """Validate deskshare parameters and provide helpful error messages"""
    # Deskshare uses same parameter types as video
    _validate_video_params(params)


# ============================================================================
# EventLoop
# ============================================================================

class EventLoop:
    """
    An SDK I/O thread that owns one or more Client lifecycles.

    The Zoom C SDK requires that alloc(), join(), poll(), and release() all run
    on the same OS thread. EventLoop is that thread. Clients assigned to a loop
    via add() will have their entire lifecycle managed on the loop's thread.

    Usage::

        loop = rtms.EventLoop()

        @rtms.on_webhook_event
        def handle(payload):
            client = rtms.Client(executor=EXECUTOR)
            client.on_audio_data(on_audio)
            loop.add(client)
            client.join(payload['payload'])

        await loop.run_async()   # or loop.run() to block

    Callbacks are dispatched according to the executor set on each Client:
    - No executor: callback runs inline on the loop's thread (simple, low latency)
    - executor=ThreadPoolExecutor(...): heavy work offloaded to worker pool
    - async def callback: bridged to the asyncio event loop via run_coroutine_threadsafe
    """

    def __init__(self, poll_interval: float = 0.01, name: str = None):
        """
        Args:
            poll_interval: Seconds between poll cycles (default: 0.01 = 10ms)
            name: Optional thread name for debugging
        """
        self._poll_interval = poll_interval
        self._name = name
        self._clients: List['Client'] = []
        self._clients_lock = threading.Lock()
        self._pending: List['Client'] = []   # clients waiting for alloc+join on this thread
        self._pending_lock = threading.Lock()
        self._running = False
        self._stop_event = threading.Event()
        self._thread: Optional[threading.Thread] = None

    @property
    def client_count(self) -> int:
        """Number of clients currently owned by this loop."""
        with self._clients_lock:
            return len(self._clients)

    def add(self, client: 'Client') -> None:
        """
        Assign a client to this loop's thread.

        Must be called before client.join(). The loop's thread will call
        alloc() and join() on behalf of the client.

        Args:
            client: A Client whose join() will be deferred to this loop's thread
        """
        client._assigned_loop = self
        with self._pending_lock:
            self._pending.append(client)

    def _drain_pending(self) -> None:
        """Called from the loop's thread — alloc and join all waiting clients."""
        with self._pending_lock:
            pending = self._pending[:]
            self._pending.clear()

        for client in pending:
            try:
                client._do_alloc_and_join()
                with self._clients_lock:
                    self._clients.append(client)
            except Exception as e:
                log_error('eventloop', f'Failed to alloc/join client: {e}')
                traceback.print_exc()

    def _poll_all(self) -> None:
        """Poll all active clients. Removes clients that have left."""
        with self._clients_lock:
            active = self._clients[:]

        to_remove = []
        for client in active:
            if client._running:
                try:
                    client.poll()
                except Exception as e:
                    log_error('eventloop', f'Error polling client: {e}')
                    to_remove.append(client)
            else:
                to_remove.append(client)

        if to_remove:
            with self._clients_lock:
                for c in to_remove:
                    self._clients.discard(c) if hasattr(self._clients, 'discard') else None
            with self._clients_lock:
                self._clients = [c for c in self._clients if c not in to_remove]

    def run(self, stop_on_empty: bool = False) -> None:
        """
        Run the event loop on the current thread (blocking).

        The current thread becomes the SDK I/O thread. Use this when you want
        explicit control of which thread drives the loop.

        Args:
            stop_on_empty: Stop automatically when all clients have left
        """
        self._running = True
        self._stop_event.clear()
        log_info('eventloop', f'Starting event loop{" (" + self._name + ")" if self._name else ""} '
                              f'(poll_interval={self._poll_interval}s)')
        try:
            while self._running and not self._stop_event.is_set():
                self._drain_pending()
                self._poll_all()
                if stop_on_empty:
                    with self._clients_lock:
                        if not self._clients and not self._pending:
                            break
                time.sleep(self._poll_interval)
        except KeyboardInterrupt:
            pass
        finally:
            self._running = False
            log_debug('eventloop', 'Event loop stopped')

    async def run_async(self, stop_on_empty: bool = False) -> None:
        """
        Run the event loop as an asyncio coroutine.

        Yields to the asyncio event loop between poll cycles so other coroutines
        (aiohttp, FastAPI, asyncpg, etc.) run freely. Async callbacks registered
        on clients are automatically bridged to this event loop.

        Args:
            stop_on_empty: Stop automatically when all clients have left

        Example::

            async def main():
                loop = rtms.EventLoop()
                await asyncio.gather(loop.run_async(), aiohttp_app.start())

            asyncio.run(main())
        """
        self._running = True
        self._stop_event.clear()
        log_info('eventloop', f'Starting async event loop{" (" + self._name + ")" if self._name else ""} '
                              f'(poll_interval={self._poll_interval}s)')
        try:
            while self._running and not self._stop_event.is_set():
                self._drain_pending()
                self._poll_all()
                if stop_on_empty:
                    with self._clients_lock:
                        if not self._clients and not self._pending:
                            break
                await asyncio.sleep(self._poll_interval)
        except asyncio.CancelledError:
            log_info('eventloop', 'Async event loop cancelled')
        finally:
            self._running = False
            log_debug('eventloop', 'Async event loop stopped')

    def start(self) -> 'EventLoop':
        """
        Start the event loop in a background daemon thread.

        Returns self for chaining::

            loop = rtms.EventLoop().start()

        The thread runs until stop() is called or the process exits.
        """
        self._thread = threading.Thread(
            target=self.run,
            name=self._name or 'rtms-eventloop',
            daemon=True,
        )
        self._thread.start()
        log_debug('eventloop', f'Background thread started: {self._thread.name}')
        return self

    def stop(self) -> None:
        """Signal the event loop to stop after the current poll cycle."""
        self._running = False
        self._stop_event.set()

    def join(self, timeout: float = None) -> None:
        """Wait for the background thread to finish (only valid after start())."""
        if self._thread:
            self._thread.join(timeout=timeout)


# ============================================================================
# EventLoopPool
# ============================================================================

class EventLoopPool:
    """
    A pool of EventLoop threads that distributes clients across N SDK I/O threads.

    Use this for high-concurrency deployments where many clients share a fixed
    number of threads. Each client is permanently assigned to one loop for its
    entire lifetime.

    Usage::

        pool = rtms.EventLoopPool(threads=4)

        @rtms.on_webhook_event
        def handle(payload):
            client = rtms.Client(executor=EXECUTOR)
            client.on_audio_data(on_audio)
            pool.add(client)          # routed to least-loaded loop
            client.join(payload['payload'])

        await pool.run_async()        # or pool.run()

    Scaling guidance:
    - 1 thread per ~25 clients is a reasonable starting point
    - Use executor= on Client for CPU/IO-heavy callbacks
    - Monitor loop.client_count to tune thread count
    """

    def __init__(
        self,
        threads: int = 4,
        poll_interval: float = 0.01,
        strategy: str = 'least_loaded',
    ):
        """
        Args:
            threads: Number of SDK I/O threads (default: 4)
            poll_interval: Seconds between poll cycles per loop (default: 0.01)
            strategy: Client routing strategy — 'least_loaded' or 'round_robin'
        """
        if threads < 1:
            raise ValueError("threads must be >= 1")
        if strategy not in ('least_loaded', 'round_robin'):
            raise ValueError("strategy must be 'least_loaded' or 'round_robin'")
        self._loops = [
            EventLoop(poll_interval=poll_interval, name=f'rtms-pool-{i}')
            for i in range(threads)
        ]
        self._strategy = strategy
        self._rr_index = 0
        self._rr_lock = threading.Lock()

    @property
    def loops(self) -> List[EventLoop]:
        """The underlying EventLoop list."""
        return self._loops

    @property
    def client_count(self) -> int:
        """Total clients across all loops."""
        return sum(l.client_count for l in self._loops)

    def add(self, client: 'Client') -> EventLoop:
        """
        Assign a client to a loop according to the routing strategy.

        Returns the EventLoop the client was assigned to.
        """
        if self._strategy == 'least_loaded':
            loop = min(self._loops, key=lambda l: l.client_count)
        else:  # round_robin
            with self._rr_lock:
                loop = self._loops[self._rr_index % len(self._loops)]
                self._rr_index += 1
        loop.add(client)
        return loop

    def run(self, stop_on_empty: bool = False) -> None:
        """
        Run all loops. Starts N-1 loops as background daemon threads and runs
        the last one on the current thread (blocking).
        """
        for loop in self._loops[:-1]:
            loop.start()
        self._loops[-1].run(stop_on_empty=stop_on_empty)

    async def run_async(self, stop_on_empty: bool = False) -> None:
        """
        Run all loops as asyncio coroutines concurrently.
        """
        await asyncio.gather(*[l.run_async(stop_on_empty=stop_on_empty) for l in self._loops])

    def stop(self) -> None:
        """Stop all loops."""
        for loop in self._loops:
            loop.stop()


# Module-level default EventLoop used by rtms.run() / rtms.run_async()
_default_loop: Optional[EventLoop] = None


class Client(_ClientBase):
    """
    RTMS Client - provides real-time media streaming capabilities

    This client allows you to join Zoom meetings and process audio, video,
    and transcript data in real-time.
    """

    _sdk_initialized = False

    def __init__(self, executor=None):
        """Initialize a new RTMS client.

        Args:
            executor: Optional concurrent.futures.Executor for dispatching data callbacks
                (audio, video, transcript, deskshare) to a thread pool. When set, callbacks
                are submitted via executor.submit() instead of running inline. Pass
                concurrent.futures.ThreadPoolExecutor(n) for CPU-bound or I/O-heavy callbacks.
        """
        # super().__init__() is PyClient() — intentionally a no-op at construction
        # time. The C SDK handle is allocated lazily in _do_alloc_and_join(), which
        # runs on the owning EventLoop's thread. This satisfies the C SDK's thread
        # affinity requirement: alloc/join/poll/release must share one OS thread.
        super().__init__()
        self._polling_interval = 10  # milliseconds
        self._running = False
        self._webhook_server = None
        self._executor = executor  # concurrent.futures.Executor or None
        self._loop = None           # asyncio event loop captured at callback-registration time

        # EventLoop that owns this client's lifecycle (set by loop.add() or auto-created)
        self._assigned_loop: Optional['EventLoop'] = None
        # Pending join params — stored until the loop's thread calls _do_alloc_and_join()
        self._pending_join_params: Optional[dict] = None

        # Individual video subscription callbacks
        self._participant_video_callback = None
        self._video_subscribed_callback = None

        # Shared event dispatcher state (matches Node.js setupEventHandler pattern)
        self._event_handler_registered = False
        self._participant_event_callback = None
        self._active_speaker_callback = None
        self._sharing_callback = None
        self._media_interrupted_callback = None
        self._raw_event_callback = None

        # Register with global client registry
        with _clients_lock:
            _clients[id(self)] = self

    def join(self,
             meeting_uuid: str = None,
             webinar_uuid: str = None,
             session_id: str = None,
             engagement_id: str = None,
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
            meeting_uuid (str): Meeting UUID (for Meeting SDK events)
            webinar_uuid (str): Webinar UUID (for Webinar events)
            session_id (str): Session ID (for Video SDK events) - used when meeting_uuid is not provided
            engagement_id (str): Engagement ID (for ZCC events) - used when meeting_uuid is not provided
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

        # Normalise all call forms into a single params dict
        params = {
            'meeting_uuid': meeting_uuid,
            'webinar_uuid': webinar_uuid,
            'session_id': session_id,
            'engagement_id': engagement_id,
            'rtms_stream_id': rtms_stream_id,
            'server_urls': server_urls,
            'signature': signature,
            'timeout': timeout,
            'ca': ca,
            'client': client,
            'secret': secret,
            'poll_interval': poll_interval,
        }
        if kwargs:
            params.update(kwargs)
        if isinstance(meeting_uuid, dict):
            params = dict(meeting_uuid)

        # Store params for the loop thread to consume
        self._pending_join_params = params

        # If the client has been assigned to an explicit EventLoop, it will be
        # picked up by that loop's _drain_pending() call — nothing else to do.
        if self._assigned_loop is not None:
            log_debug("client", "join() deferred to assigned EventLoop thread")
            return True

        # If rtms.run() / rtms.run_async() is active, route to the default loop.
        if _default_loop is not None:
            log_debug("client", "join() routed to default EventLoop")
            _default_loop.add(self)
            return True

        # Zero-config (Tier 0): no loop assigned and no run() active —
        # create an implicit single-client EventLoop as a background daemon thread.
        log_debug("client", "No EventLoop assigned — creating implicit single-client loop")
        implicit_loop = EventLoop(
            poll_interval=params.get('poll_interval', 10) / 1000.0,
            name='rtms-implicit',
        )
        implicit_loop.add(self)
        implicit_loop.start()
        return True

    def _do_alloc_and_join(self) -> None:
        """
        Called by EventLoop._drain_pending() on the loop's own thread.

        Performs the two operations that must share an OS thread:
          1. alloc()  — creates the C SDK handle (rtms_alloc)
          2. join()   — registers callbacks and connects (rtms_set_callbacks + rtms_join)
        """
        params = self._pending_join_params
        if params is None:
            raise RuntimeError("_do_alloc_and_join called with no pending join params")

        try:
            meeting_uuid  = params.get('meeting_uuid')
            webinar_uuid  = params.get('webinar_uuid')
            session_id    = params.get('session_id')
            engagement_id = params.get('engagement_id')
            rtms_stream_id = params.get('rtms_stream_id')
            server_urls   = params.get('server_urls')
            signature     = params.get('signature')
            timeout       = params.get('timeout', -1)
            client_id     = params.get('client', os.getenv('ZM_RTMS_CLIENT'))
            secret        = params.get('secret', os.getenv('ZM_RTMS_SECRET'))
            poll_interval = params.get('poll_interval', 10)

            instance_id = meeting_uuid or webinar_uuid or session_id or engagement_id
            if not instance_id:
                raise ValueError("meeting_uuid, webinar_uuid, session_id, or engagement_id is required")
            if not rtms_stream_id:
                raise ValueError("rtms_stream_id is required")
            if not server_urls:
                raise ValueError("server_urls is required")

            if not signature:
                signature = generate_signature(client_id, secret, instance_id, rtms_stream_id)

            self._polling_interval = poll_interval

            # Phase 1: ensure SDK is initialized on THIS thread (same thread as alloc/join).
            # The lock ensures init() runs exactly once even if multiple EventLoops start
            # simultaneously, but the actual C call only happens on the first client's thread.
            with _sdk_init_lock:
                if not Client._sdk_initialized:
                    ca_path = find_ca_certificate()
                    log_debug("client", f"Initializing SDK with CA: {ca_path}")
                    try:
                        _ClientBase.initialize(ca_path, 1, "python-rtms")
                    except Exception:
                        log_debug("client", "Trying SDK initialization with empty CA path")
                        _ClientBase.initialize("", 1, "python-rtms")
                    Client._sdk_initialized = True
                    log_debug("client", "SDK initialized successfully")

            # Phase 2: allocate C SDK handle on this thread
            super().alloc()

            session_type = 'meeting' if meeting_uuid else 'webinar' if webinar_uuid else 'engagement' if engagement_id else 'session'
            log_info("client", f"Joining {session_type}: {instance_id}")

            # join() on the same thread as alloc() — C SDK constraint satisfied
            super().join(instance_id, rtms_stream_id, signature, server_urls, timeout)

            self._running = True
            log_info("client", "Successfully joined")

        except Exception as e:
            log_error("client", f"Error in _do_alloc_and_join: {e}")
            traceback.print_exc()
            raise

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

    def poll(self):
        """Poll the C SDK for pending events. Called by the owning EventLoop's thread."""
        if self._running:
            try:
                super().poll()
            except Exception as e:
                log_error("client", f"Error during polling: {e}")

    # ========================================================================
    # Callback Dispatch
    # ========================================================================

    def _wrap_callback(self, callback):
        """Wrap a callback for executor or asyncio dispatch.

        - sync + no executor  → returned unchanged (v1.0 inline behavior)
        - sync + executor     → submitted to executor.submit() on each call
        - async coroutine     → scheduled on the captured asyncio event loop
        """
        if callback is None:
            return None
        if inspect.iscoroutinefunction(callback):
            # Capture the running loop now (at registration time) if one exists.
            try:
                loop = asyncio.get_running_loop()
            except RuntimeError:
                loop = None
            self._loop = loop
            def async_wrapper(*args):
                _loop = self._loop
                if _loop and _loop.is_running():
                    asyncio.run_coroutine_threadsafe(callback(*args), _loop)
                else:
                    try:
                        asyncio.run(callback(*args))
                    except RuntimeError:
                        pass
            return async_wrapper
        executor = self._executor or _run_executor
        if executor is not None:
            def executor_wrapper(*args):
                executor.submit(callback, *args)
            return executor_wrapper
        return callback

    # ========================================================================
    # Data Callbacks (Python-level so _wrap_callback applies and aliases work)
    # ========================================================================

    def on_audio_data(self, callback) -> None:
        """Register audio data callback. Supports executor and async coroutines."""
        super().on_audio_data(self._wrap_callback(callback))

    onAudioData = on_audio_data

    def on_video_data(self, callback) -> None:
        """Register video data callback. Supports executor and async coroutines."""
        super().on_video_data(self._wrap_callback(callback))

    onVideoData = on_video_data

    def on_deskshare_data(self, callback) -> None:
        """Register deskshare data callback. Supports executor and async coroutines."""
        super().on_deskshare_data(self._wrap_callback(callback))

    onDeskshareData = on_deskshare_data

    def on_transcript_data(self, callback) -> None:
        """Register transcript data callback. Supports executor and async coroutines."""
        super().on_transcript_data(self._wrap_callback(callback))

    onTranscriptData = on_transcript_data

    # ========================================================================
    # Context Manager
    # ========================================================================

    def __enter__(self):
        """Support `with rtms.Client() as client:` usage."""
        return self

    def __exit__(self, *_):
        """Call leave() on context exit. Exceptions are not suppressed."""
        self.leave()
        return False

    def stop(self):
        """
        Stop RTMS client and release resources.

        This is equivalent to calling leave(), but with a more intuitive name.
        """
        return self.leave()

    def set_audio_params(self, params):
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
        return super().set_audio_params(params)

    # camelCase legacy alias
    setAudioParams = set_audio_params

    def set_video_params(self, params):
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
        return super().set_video_params(params)

    # camelCase legacy alias
    setVideoParams = set_video_params

    def set_deskshare_params(self, params):
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
        return super().set_deskshare_params(params)

    # camelCase legacy alias
    setDeskshareParams = set_deskshare_params

    def set_transcript_params(self, params):
        """
        Set transcript parameters.

        Args:
            params (TranscriptParams): Transcript parameters object

        Returns:
            bool: True if parameters were set successfully
        """
        return super().set_transcript_params(params)

    # camelCase legacy alias
    setTranscriptParams = set_transcript_params

    def set_proxy(self, proxy_type: str, proxy_url: str) -> None:
        """Configure a proxy for SDK connections.

        Args:
            proxy_type (str): Proxy protocol type (e.g. 'http', 'https').
            proxy_url (str): Full proxy URL including host and port.
        """
        return super().set_proxy(proxy_type, proxy_url)

    # camelCase legacy alias
    setProxy = set_proxy

    def subscribe_video(self, user_id: int, subscribe: bool) -> None:
        """Subscribe or unsubscribe from an individual participant's video stream.

        Args:
            user_id (int): The participant's user ID.
            subscribe (bool): True to subscribe, False to unsubscribe.
        """
        return super().subscribe_video(user_id, subscribe)

    subscribeVideo = subscribe_video

    def on_participant_video(self, callback) -> None:
        """Register a callback for participant video state changes.

        The callback receives (users: list[int], is_on: bool).
        """
        self._participant_video_callback = callback
        super().on_participant_video(callback)

    onParticipantVideo = on_participant_video

    def on_video_subscribed(self, callback) -> None:
        """Register a callback for video subscription responses.

        The callback receives (user_id: int, status: int, error: str).
        """
        self._video_subscribed_callback = callback
        super().on_video_subscribed(callback)

    onVideoSubscribed = on_video_subscribed

    def subscribe_event(self, events):
        """
        Subscribe to receive specific event types.

        Note: Calling on_participant_event() automatically subscribes to
        EVENT_PARTICIPANT_JOIN and EVENT_PARTICIPANT_LEAVE events.

        Args:
            events: List of event type constants (e.g., [EVENT_PARTICIPANT_JOIN, EVENT_PARTICIPANT_LEAVE])

        Returns:
            bool: True if subscription was successful
        """
        return super().subscribe_event(events)

    # camelCase legacy alias
    subscribeEvent = subscribe_event

    def unsubscribe_event(self, events):
        """
        Unsubscribe from specific event types.

        Args:
            events: List of event type constants

        Returns:
            bool: True if unsubscription was successful
        """
        return super().unsubscribe_event(events)

    # camelCase legacy alias
    unsubscribeEvent = unsubscribe_event

    def _setup_event_handler(self):
        """
        Internal shared event dispatcher that routes events to typed callbacks.
        Matches the Node.js setupEventHandler() pattern. Only registers once.
        """
        if self._event_handler_registered:
            return
        self._event_handler_registered = True

        def event_dispatcher(event_data: str):
            if self._raw_event_callback:
                self._raw_event_callback(event_data)
            try:
                data = json.loads(event_data)
                event_type = data.get('event_type')

                if event_type == EVENT_PARTICIPANT_JOIN:
                    if self._participant_event_callback:
                        participants = [
                            {'user_id': p.get('user_id'), 'user_name': p.get('user_name')}
                            for p in data.get('participants', [])
                        ]
                        self._participant_event_callback('join', data.get('timestamp', 0), participants)

                elif event_type == EVENT_PARTICIPANT_LEAVE:
                    if self._participant_event_callback:
                        participants = [
                            {'user_id': p.get('user_id'), 'user_name': p.get('user_name')}
                            for p in data.get('participants', [])
                        ]
                        self._participant_event_callback('leave', data.get('timestamp', 0), participants)

                elif event_type == EVENT_ACTIVE_SPEAKER_CHANGE:
                    if self._active_speaker_callback:
                        self._active_speaker_callback(
                            data.get('timestamp', 0),
                            data.get('user_id', 0),
                            data.get('user_name', '')
                        )

                elif event_type == EVENT_SHARING_START:
                    if self._sharing_callback:
                        self._sharing_callback(
                            'start',
                            data.get('timestamp', 0),
                            data.get('user_id'),
                            data.get('user_name')
                        )

                elif event_type == EVENT_SHARING_STOP:
                    if self._sharing_callback:
                        self._sharing_callback(
                            'stop',
                            data.get('timestamp', 0),
                            None,
                            None
                        )

                elif event_type == EVENT_MEDIA_CONNECTION_INTERRUPTED:
                    if self._media_interrupted_callback:
                        self._media_interrupted_callback(data.get('timestamp', 0))

            except Exception as e:
                log_error('client', f'Failed to parse event: {e}')

        super().on_event_ex(event_dispatcher)

    def on_participant_event(self, callback: Callable[[str, int, list], None]) -> bool:
        """
        Register a callback for participant join/leave events.

        This automatically subscribes to EVENT_PARTICIPANT_JOIN and EVENT_PARTICIPANT_LEAVE.
        Events are delivered as parsed objects, not raw JSON.

        Args:
            callback: Function called with (event, timestamp, participants) where:
                - event: 'join' or 'leave'
                - timestamp: Unix timestamp in milliseconds
                - participants: List of dicts with 'user_id' and optional 'user_name'

        Returns:
            bool: True if registration succeeds

        Example:
            >>> def on_participant(event, timestamp, participants):
            ...     print(f"Participant {event}: {participants}")
            >>> client.on_participant_event(on_participant)
        """
        self._participant_event_callback = callback
        self._setup_event_handler()
        try:
            self.subscribe_event([EVENT_PARTICIPANT_JOIN, EVENT_PARTICIPANT_LEAVE])
        except Exception as e:
            log_warn('client', f'Failed to auto-subscribe to participant events: {e}')
        return True

    # camelCase legacy alias
    onParticipantEvent = on_participant_event

    def on_active_speaker_event(self, callback: Callable[[int, int, str], None]) -> bool:
        """
        Register a callback for active speaker change events.

        This automatically subscribes to EVENT_ACTIVE_SPEAKER_CHANGE.

        Args:
            callback: Function called with (timestamp, user_id, user_name)

        Returns:
            bool: True if registration succeeds

        Example:
            >>> def on_speaker(timestamp, user_id, user_name):
            ...     print(f"Active speaker: {user_name} ({user_id})")
            >>> client.on_active_speaker_event(on_speaker)
        """
        self._active_speaker_callback = callback
        self._setup_event_handler()
        try:
            self.subscribe_event([EVENT_ACTIVE_SPEAKER_CHANGE])
        except Exception as e:
            log_warn('client', f'Failed to auto-subscribe to active speaker events: {e}')
        return True

    # camelCase legacy alias
    onActiveSpeakerEvent = on_active_speaker_event

    def on_sharing_event(self, callback: Callable[[str, int, Optional[int], Optional[str]], None]) -> bool:
        """
        Register a callback for sharing start/stop events.

        This automatically subscribes to EVENT_SHARING_START and EVENT_SHARING_STOP.
        Note: These events only work when the RTMS app has DESKSHARE scope permission.

        Args:
            callback: Function called with (event, timestamp, user_id, user_name) where:
                - event: 'start' or 'stop'
                - timestamp: Unix timestamp in milliseconds
                - user_id: Optional user ID (only for 'start')
                - user_name: Optional user name (only for 'start')

        Returns:
            bool: True if registration succeeds

        Example:
            >>> def on_sharing(event, timestamp, user_id, user_name):
            ...     print(f"Sharing {event} by {user_name}")
            >>> client.on_sharing_event(on_sharing)
        """
        self._sharing_callback = callback
        self._setup_event_handler()
        try:
            self.subscribe_event([EVENT_SHARING_START, EVENT_SHARING_STOP])
        except Exception as e:
            log_warn('client', f'Failed to auto-subscribe to sharing events: {e}')
        return True

    # camelCase legacy alias
    onSharingEvent = on_sharing_event

    def on_media_connection_interrupted(self, callback: Callable[[int], None]) -> bool:
        """
        Register a callback for media connection interrupted events.

        This automatically subscribes to EVENT_MEDIA_CONNECTION_INTERRUPTED.

        Args:
            callback: Function called with (timestamp,) when the media connection is interrupted

        Returns:
            bool: True if registration succeeds

        Example:
            >>> def on_interrupted(timestamp):
            ...     print(f"Media connection interrupted at {timestamp}")
            >>> client.on_media_connection_interrupted(on_interrupted)
        """
        self._media_interrupted_callback = callback
        self._setup_event_handler()
        try:
            self.subscribe_event([EVENT_MEDIA_CONNECTION_INTERRUPTED])
        except Exception as e:
            log_warn('client', f'Failed to auto-subscribe to media connection interrupted events: {e}')
        return True

    # camelCase legacy alias
    onMediaConnectionInterrupted = on_media_connection_interrupted

    def on_event_ex(self, callback: Callable[[str], None]) -> bool:
        """
        Register a callback for raw event data.

        This provides access to the raw JSON event data from the SDK.
        Use this when you need custom event handling or access to all event types.
        This callback is called IN ADDITION to typed callbacks, not instead of.

        Args:
            callback: Function called with raw JSON event data string

        Returns:
            bool: True if registration succeeds
        """
        self._raw_event_callback = callback
        self._setup_event_handler()
        return True

    # camelCase legacy alias
    onEventEx = on_event_ex

    def leave(self):
        """
        Leave the RTMS session.

        Signals the owning EventLoop to stop polling this client and releases
        the C SDK handle. Safe to call from any thread.

        Returns:
            bool: True if left successfully
        """
        log_info("client", "Leaving RTMS session")

        # Signal the EventLoop to stop polling this client
        self._running = False

        # Unregister from global client registry
        with _clients_lock:
            _clients.pop(id(self), None)

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

def uninitialize():
    """
    Uninitialize the RTMS SDK.

    Call this when you're done using the SDK to release resources.
    """
    try:
        _ClientBase.uninitialize()
        Client._sdk_initialized = False
        return True
    except Exception as e:
        log_error("rtms", f"Error uninitializing RTMS SDK: {e}")
        traceback.print_exc()
        return False

def initialize(ca_path=None):
    """
    Initialize the RTMS SDK.

    Note: The SDK is automatically initialized when you create a Client instance.
    You only need to call this if you want to initialize with a custom CA path.

    Args:
        ca_path (str, optional): Path to the CA certificate file.

    Returns:
        bool: True if initialization was successful
    """
    ca_path = find_ca_certificate(ca_path)
    try:
        _ClientBase.initialize(ca_path)
        Client._sdk_initialized = True
        return True
    except Exception as e:
        log_error("rtms", f"Error initializing RTMS SDK: {e}")
        return False


def onWebhookEvent(callback=None, port=None, path=None):
    """
    Start a webhook server to receive events from Zoom.

    This function creates an HTTP server that listens for webhook events from Zoom.
    When a webhook event is received, it parses the JSON payload and passes it to
    the provided callback function.

    Can be used as a decorator or a direct function call:

    @rtms.onWebhookEvent(port=8080, path='/webhook')
    def handle_webhook(payload):
        if payload.get('event') == 'meeting.rtms.started':
            # Create a client and join
            client = rtms.Client()
            client.join(...)

    Args:
        callback (callable, optional): Function to call when a webhook is received
        port (int, optional): Port to listen on. Defaults to ZM_RTMS_PORT env var or 8080
        path (str, optional): URL path to listen on. Defaults to ZM_RTMS_PATH env var or '/'

    Returns:
        callable: Decorator function if used as a decorator
    """
    global _webhook_server

    # Determine port and path
    webhook_port = port or int(os.getenv('ZM_RTMS_PORT', '8080'))
    webhook_path = path or os.getenv('ZM_RTMS_PATH', '/')

    # If used as a decorator without arguments
    if callback is not None and callable(callback):
        if _webhook_server is None:
            _webhook_server = WebhookServer(webhook_port, webhook_path)
        _webhook_server.start(callback)
        return callback

    # If used as a decorator with arguments or as a method call
    def decorator(func):
        global _webhook_server
        if _webhook_server is None:
            _webhook_server = WebhookServer(webhook_port, webhook_path)
        _webhook_server.start(func)
        return func

    return decorator


# Alias for backwards compatibility
on_webhook_event = onWebhookEvent


def run(poll_interval: float = 0.01, stop_on_empty: bool = False, executor=None):
    """
    Start the default RTMS event loop (blocking).

    Clients that call join() without an explicit EventLoop are automatically
    routed to this default loop. Blocks until interrupted or stop() is called.

    Args:
        poll_interval: Seconds between poll cycles (default: 0.01 = 10ms)
        stop_on_empty: Stop automatically when all clients have left
        executor: Optional concurrent.futures.Executor for dispatching data
            callbacks on all clients that don't have their own executor set.

    Example::

        @rtms.on_webhook_event
        def handle(payload):
            client = rtms.Client()
            client.on_transcript_data(lambda d,s,t,m: print(m.userName, d))
            client.join(payload['payload'])

        rtms.run()   # blocks until Ctrl-C
    """
    global _default_loop, _running, _run_executor
    _run_executor = executor
    _default_loop = EventLoop(poll_interval=poll_interval, name='rtms-default')
    _running = True
    _stop_event.clear()
    try:
        _default_loop.run(stop_on_empty=stop_on_empty)
    finally:
        _running = False
        _default_loop = None
        _run_executor = None


async def run_async(poll_interval: float = 0.01, stop_on_empty: bool = False, executor=None):
    """
    Start the default RTMS event loop as an asyncio coroutine.

    Drop-in async replacement for rtms.run(). Yields to the asyncio event loop
    between poll cycles so other coroutines (aiohttp, FastAPI, asyncpg) run freely.

    Args:
        poll_interval: Seconds between poll cycles (default: 0.01 = 10ms)
        stop_on_empty: Stop automatically when all clients have left
        executor: Optional concurrent.futures.Executor for dispatching data
            callbacks on all clients that don't have their own executor set.

    Example::

        async def main():
            await asyncio.gather(rtms.run_async(), aiohttp_app.start())

        asyncio.run(main())
    """
    global _default_loop, _running, _run_executor
    _run_executor = executor
    _default_loop = EventLoop(poll_interval=poll_interval, name='rtms-default')
    _running = True
    _stop_event.clear()
    try:
        await _default_loop.run_async(stop_on_empty=stop_on_empty)
    finally:
        _running = False
        _default_loop = None
        _run_executor = None


def stop():
    """
    Stop the default event loop (rtms.run() or rtms.run_async()).

    For EventLoop or EventLoopPool instances, call their .stop() method directly.
    """
    global _running
    _running = False
    _stop_event.set()
    if _default_loop:
        _default_loop.stop()
    log_info('rtms', 'Stop signal received')


__all__ = [
    # Classes
    "Client",
    "EventLoop",
    "EventLoopPool",
    "Session",
    "Participant",
    "AiTargetLanguage",
    "AiInterpreter",
    "Metadata",
    "AudioParams",
    "VideoParams",
    "DeskshareParams",
    "TranscriptParams",
    "LogLevel",
    "LogFormat",

    # Constants - Media Types
    "MEDIA_TYPE_AUDIO",
    "MEDIA_TYPE_VIDEO",
    "MEDIA_TYPE_DESKSHARE",
    "MEDIA_TYPE_TRANSCRIPT",
    "MEDIA_TYPE_CHAT",
    "MEDIA_TYPE_ALL",

    # Constants - Session Events
    "SESSION_EVENT_ADD",
    "SESSION_EVENT_STOP",
    "SESSION_EVENT_PAUSE",
    "SESSION_EVENT_RESUME",

    # Constants - User Events
    "USER_JOIN",
    "USER_LEAVE",

    # Constants - Event Types (for subscribeEvent/onEventEx)
    # These match EVENT_TYPE from Zoom's C SDK
    "EVENT_UNDEFINED",
    "EVENT_FIRST_PACKET_TIMESTAMP",
    "EVENT_ACTIVE_SPEAKER_CHANGE",
    "EVENT_PARTICIPANT_JOIN",
    "EVENT_PARTICIPANT_LEAVE",
    "EVENT_SHARING_START",
    "EVENT_SHARING_STOP",
    "EVENT_MEDIA_CONNECTION_INTERRUPTED",
    "EVENT_PARTICIPANT_VIDEO_ON",
    "EVENT_PARTICIPANT_VIDEO_OFF",
    "EVENT_CONSUMER_ANSWERED",
    "EVENT_CONSUMER_END",
    "EVENT_USER_ANSWERED",
    "EVENT_USER_END",
    "EVENT_USER_HOLD",
    "EVENT_USER_UNHOLD",

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

    # Transcript language constants dict (mirrors AudioCodec/VideoCodec pattern)
    "TranscriptLanguage",

    # Parameter dictionaries
    "AudioContentType",
    "AudioCodec",
    "AudioSampleRate",
    "AudioChannel",
    "DataOption",
    "AudioDataOption",  # legacy alias for DataOption
    "VideoContentType",
    "VideoCodec",
    "VideoResolution",
    "VideoDataOption",  # legacy alias for DataOption
    "MediaDataType",
    "SessionState",
    "StreamState",
    "EventType",
    "MessageType",
    "StopReason",

    # SDK initialization functions
    "initialize",
    "uninitialize",

    # Utility functions
    "generate_signature",

    # Webhook functions
    "onWebhookEvent",
    "on_webhook_event",

    # Event loop functions
    "run",
    "run_async",
    "stop",

    # Logging functions
    "log_debug",
    "log_info",
    "log_warn",
    "log_error",
    "configure_logger",
]