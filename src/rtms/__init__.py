import os
import json
import hmac
import hashlib
import threading
import time
import sys
import traceback
from http.server import BaseHTTPRequestHandler, HTTPServer

_webhook_server = None
_debug = os.getenv('RTMS_DEBUG', '0') == '1'

def _log_debug(msg):
    """Print debug message if debug mode is enabled"""
    if _debug:
        print(f"[RTMS-PY] {msg}", file=sys.stderr)

try:
    _log_debug("Importing _rtms module")
    from ._rtms import (Client as _ClientBase, _initialize,
                      _uninitialize, Session, Participant, Metadata,
                      SDK_AUDIO, SDK_VIDEO, SDK_TRANSCRIPT, SDK_ALL,
                      SESSION_ADD, SESSION_STOP, SESSION_PAUSE, SESSION_RESUME,
                      USER_JOIN, USER_LEAVE,
                      RTMS_SDK_FAILURE, RTMS_SDK_OK, RTMS_SDK_TIMEOUT,
                      RTMS_SDK_NOT_EXIST, RTMS_SDK_WRONG_TYPE,
                      RTMS_SDK_INVALID_STATUS, RTMS_SDK_INVALID_ARGS,
                      SESS_STATUS_ACTIVE, SESS_STATUS_PAUSED)
    _log_debug("Successfully imported _rtms module")
except ImportError as e:
    print(f"Error importing RTMS module: {e}", file=sys.stderr)
    traceback.print_exc()
    # Provide mock objects to avoid errors
    class _ClientBase:
        @staticmethod
        def initialize(*args, **kwargs): pass
        @staticmethod
        def uninitialize(*args, **kwargs): pass
        def join(self, *args, **kwargs): pass
        def poll(self, *args, **kwargs): pass
        def release(self, *args, **kwargs): pass
        def uuid(self, *args, **kwargs): return ""
        def stream_id(self, *args, **kwargs): return ""
        def on_join_confirm(self): return lambda func: func
        def on_session_update(self): return lambda func: func
        def on_user_update(self): return lambda func: func
        def on_audio_data(self): return lambda func: func
        def on_video_data(self): return lambda func: func
        def on_transcript_data(self): return lambda func: func
        def on_leave(self): return lambda func: func

    def _initialize(*args, **kwargs): pass
    def _uninitialize(*args, **kwargs): pass
    class Session: pass
    class Participant: pass
    class Metadata: pass
    SDK_AUDIO = 1
    SDK_VIDEO = 2
    SDK_TRANSCRIPT = 4
    SDK_ALL = 7
    SESSION_ADD = 1
    SESSION_STOP = 2
    SESSION_PAUSE = 3
    SESSION_RESUME = 4
    USER_JOIN = 1
    USER_LEAVE = 2
    RTMS_SDK_FAILURE = 1
    RTMS_SDK_OK = 0
    RTMS_SDK_TIMEOUT = 2
    RTMS_SDK_NOT_EXIST = 3
    RTMS_SDK_WRONG_TYPE = 4
    RTMS_SDK_INVALID_STATUS = 5
    RTMS_SDK_INVALID_ARGS = 6
    SESS_STATUS_ACTIVE = 1
    SESS_STATUS_PAUSED = 2

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
            _log_debug(f"Received webhook payload: {payload}")
            self.server.webhook_callback(payload) 

            self.send_response(200)
            self.end_headers()
            self.wfile.write(b'Webhook received successfully')
        except json.JSONDecodeError as e:
            _log_debug(f"Error parsing webhook JSON: {e}")
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Invalid JSON received")
        except Exception as e:
            _log_debug(f"Error processing webhook: {e}")
            self.send_response(500)
            self.end_headers()
            self.wfile.write(f"Internal error: {str(e)}".encode('utf-8'))
    
    def log_message(self, format, *args):
        # Suppress default logging unless in debug mode
        if _debug:
            super().log_message(format, *args)

def generate_signature(client, secret, uuid, stream_id):
    """Generate a signature for RTMS authentication"""
    client_id = os.getenv("ZM_RTMS_CLIENT", client)
    client_secret = os.getenv("ZM_RTMS_SECRET", secret)

    if not client_id:
        raise EnvironmentError("Client ID cannot be blank")
    elif not client_secret:
        raise EnvironmentError("Client Secret cannot be blank")

    message = f"{client_id},{uuid},{stream_id}"

    signature = hmac.new(
        client_secret.encode('utf-8'),
        message.encode('utf-8'),
        hashlib.sha256
    ).hexdigest()

    return signature

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
            
            _log_debug(f"Starting webhook server on port {self.port} path {self.path}")
            self.server_thread = threading.Thread(target=self.server.serve_forever, daemon=True)
            self.server_thread.start()
            
            return True
        except Exception as e:
            _log_debug(f"Error starting webhook server: {e}")
            traceback.print_exc()
            return False
    
    def stop(self):
        if not self.server:
            return
        
        try:
            _log_debug("Shutting down webhook server")
            self.server.shutdown()
            self.server.server_close()
            self.server = None
            self.server_thread = None
        except Exception as e:
            _log_debug(f"Error shutting down webhook server: {e}")

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
             uuid: str = None, 
             stream_id: str = None, 
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
            uuid (str): Meeting UUID
            stream_id (str): RTMS stream ID
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
        # Support for both dictionary-style and parameter-style calls
        if len(kwargs) > 0:
            # If additional kwargs are provided, merge them with the named parameters
            params = {
                'uuid': uuid,
                'stream_id': stream_id,
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
        if isinstance(uuid, dict):
            return self._join_with_params(**uuid)
            
        # Otherwise, use the parameters directly
        return self._join_with_params(
            uuid=uuid,
            stream_id=stream_id,
            server_urls=server_urls,
            signature=signature,
            timeout=timeout,
            ca=ca,
            client=client,
            secret=secret,
            poll_interval=poll_interval
        )
    
    def _join_with_params(self, **params):
        """
        Internal method to join with parameter dictionary
        """
        # Extract parameters with defaults
        uuid = params.get('uuid') or params.get('meeting_uuid')
        stream_id = params.get('stream_id') or params.get('rtms_stream_id')
        server_urls = params.get('server_urls')
        signature = params.get('signature')
        timeout = params.get('timeout', -1)
        ca = params.get('ca')
        client = params.get('client', '')
        secret = params.get('secret', '')
        poll_interval = params.get('poll_interval', 10)
        
        if not uuid:
            raise ValueError("Meeting UUID is required")
        if not stream_id:
            raise ValueError("Stream ID is required")
        if not server_urls:
            raise ValueError("Server URLs is required")
            
        # Initialize RTMS if not already initialized
        if ca:
            try:
                _initialize(ca)
            except Exception as e:
                _log_debug(f"Error initializing RTMS with CA {ca}: {e}")
                # Try to find a system CA file
                self._initialize_with_system_ca()
        else:
            # Try to find a system CA file
            self._initialize_with_system_ca()
        
        # Generate signature if not provided
        if not signature:
            try:
                signature = generate_signature(client, secret, uuid, stream_id)
            except Exception as e:
                _log_debug(f"Error generating signature: {e}")
                raise
        
        # Store polling interval
        self._polling_interval = poll_interval
        
        # Join the meeting
        try:
            super().join(uuid, stream_id, signature, server_urls, timeout)
            
            # Start polling thread
            self._start_polling()
            
            return True
        except Exception as e:
            _log_debug(f"Error joining meeting: {e}")
            traceback.print_exc()
            return False
    
    def _initialize_with_system_ca(self):
        """Try to initialize RTMS with a system CA file"""
        system_ca_paths = [
            '/etc/ssl/certs/ca-certificates.crt',  # Debian/Ubuntu
            '/etc/pki/tls/certs/ca-bundle.crt',    # Fedora/RHEL
            '/etc/ssl/ca-bundle.pem',              # OpenSUSE
            '/etc/ssl/cert.pem',                   # Alpine
            '/etc/ssl/certs'                       # Directory with certs
        ]
        
        ca_path = os.getenv("ZM_RTMS_CA", "")
        if not ca_path or not os.path.exists(ca_path):
            for path in system_ca_paths:
                if os.path.exists(path):
                    ca_path = path
                    _log_debug(f"Found system CA file at {ca_path}")
                    break
        
        if ca_path:
            try:
                _initialize(ca_path)
                return True
            except Exception as e:
                _log_debug(f"Error initializing RTMS with CA {ca_path}: {e}")
                raise
        else:
            # Try initializing with empty CA path
            try:
                _initialize("")
                return True
            except Exception as e:
                _log_debug(f"Error initializing RTMS without CA: {e}")
                raise
    
    def _polling_worker(self):
        """Background thread for polling RTMS client"""
        _log_debug("Starting polling worker thread")
        
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
                _log_debug(f"Error during polling: {e}")
                # Don't break the loop on errors, just log and continue
                time.sleep(0.5)  # Add small delay to avoid tight loop if there are persistent errors
                
        _log_debug("Polling worker thread stopped")
    
    def _start_polling(self):
        """Start the polling thread"""
        if self._polling_thread and self._polling_thread.is_alive():
            return
        
        self._running = True
        self._polling_thread = threading.Thread(target=self._polling_worker)
        self._polling_thread.start()
    
    def _stop_polling(self):
        """Stop the polling thread"""
        self._running = False
        if self._polling_thread and self._polling_thread.is_alive():
            try:
                self._polling_thread.join(timeout=1)
            except Exception as e:
                _log_debug(f"Error stopping polling thread: {e}")
    
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
        _log_debug("Leaving RTMS session")
        
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
            _log_debug(f"Error releasing RTMS resources: {e}")
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

def uninitialize():
    """
    Uninitialize the RTMS SDK.
    """
    try:
        _uninitialize()
        return True
    except Exception as e:
        _log_debug(f"Error uninitializing RTMS SDK: {e}")
        traceback.print_exc()
        return False

def uuid():
    """
    Get the UUID of the current meeting using the global client.
    """
    try:
        return _global_client.uuid()
    except Exception as e:
        _log_debug(f"Error getting UUID: {e}")
        return ""

def stream_id():
    """
    Get the stream ID of the current meeting using the global client.
    """
    try:
        return _global_client.stream_id()
    except Exception as e:
        _log_debug(f"Error getting stream ID: {e}")
        return ""

def set_debug(enable=True):
    """
    Enable or disable debug logging.
    
    Args:
        enable (bool, optional): Whether to enable debug logging. Defaults to True.
    """
    global _debug
    _debug = enable
    os.environ['RTMS_DEBUG'] = '1' if enable else '0'
    print(f"RTMS debug logging {'enabled' if enable else 'disabled'}")

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

__all__ = [
    # Classes
    "Client",
    "Session",
    "Participant",
    "Metadata",
    
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
    "uninitialize",
    "uuid", 
    "stream_id",
    "generate_signature",
    "set_debug",
    
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