import os
import json
import hmac
import hashlib
import threading
import time
import sys
import traceback
from http.server import BaseHTTPRequestHandler, HTTPServer
from functools import wraps

_webhook_server = None
_polling_thread = None
_polling_interval = 0
_running = False
_initialized = False
_debug = os.getenv('RTMS_DEBUG', '0') == '1'

def _log_debug(msg):
    """Print debug message if debug mode is enabled"""
    if _debug:
        print(f"[RTMS-PY] {msg}", file=sys.stderr)

try:
    _log_debug("Importing _rtms module")
    from ._rtms import (_initialize, _uninitialize, _join, _poll, _release, _uuid, _stream_id,
                      on_join_confirm, on_session_update, on_user_update,
                      on_audio_data, on_video_data, on_transcript_data, on_leave,
                      Session, Participant, Metadata)
    _log_debug("Successfully imported _rtms module")
except ImportError as e:
    print(f"Error importing RTMS module: {e}", file=sys.stderr)
    traceback.print_exc()
    # Provide mock objects to avoid errors
    def _initialize(*args, **kwargs): pass
    def _uninitialize(*args, **kwargs): pass
    def _join(*args, **kwargs): pass
    def _poll(*args, **kwargs): pass
    def _release(*args, **kwargs): pass
    def _uuid(*args, **kwargs): return ""
    def _stream_id(*args, **kwargs): return ""
    def on_join_confirm(func): return func
    def on_session_update(func): return func
    def on_user_update(func): return func
    def on_audio_data(func): return func
    def on_video_data(func): return func
    def on_transcript_data(func): return func
    def on_leave(func): return func


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


def on_webhook_event(callback):
    """Decorator for webhook event handlers
    
    Example:
        @rtms.on_webhook_event
        def handle_webhook(payload):
            print(f"Received webhook: {payload}")
    """
    global _webhook_server
    
    @wraps(callback)
    def wrapper(*args, **kwargs):
        return callback(*args, **kwargs)
    
    # Start webhook server if not already running
    if not _webhook_server:
        try:
            port = int(os.getenv('ZM_RTMS_PORT', '8080')) 
            path = os.getenv('ZM_RTMS_PATH', '/')
            
            class WebhookServer(HTTPServer):
                def __init__(self, *args, **kwargs):
                    super().__init__(*args, **kwargs)
                    self.webhook_callback = callback
                    self.webhook_path = path
            
            _webhook_server = WebhookServer(('0.0.0.0', port), WebhookHandler)
            
            print(f"🚀 Listening for webhook events at http://localhost:{port}{path}")
            
            server_thread = threading.Thread(target=_webhook_server.serve_forever, daemon=True)
            server_thread.start()
        except Exception as e:
            _log_debug(f"Error starting webhook server: {e}")
            traceback.print_exc()
    else:
        # Update the callback if server already exists
        _webhook_server.webhook_callback = callback
    
    return wrapper


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


def _polling_worker():
    """Background thread for polling RTMS client"""
    global _running
    _log_debug("Starting polling worker thread")
    
    # We need to initialize the Python thread properly for GIL management
    import threading
    
    # Important: Make sure thread is properly initialized for GIL management
    threading.current_thread().name = "RTMS_Polling_Thread"
    
    while _running:
        try:
            # Use a shorter timeout to ensure responsive shutdown
            for _ in range(10):  # Split sleep into smaller chunks for more responsive shutdown
                if not _running:
                    break
                time.sleep(_polling_interval / 10000)  # Convert to seconds and divide by 10
            
            if not _running:
                break
                
            # Call poll if we're still running
            _poll()
        except Exception as e:
            _log_debug(f"Error during polling: {e}")
            traceback.print_exc()
            # Don't break the loop on errors, just log and continue
            time.sleep(0.5)  # Add small delay to avoid tight loop if there are persistent errors
            
    _log_debug("Polling worker thread stopped")


def join(uuid, stream_id, server_urls, timeout=-1, ca="ca.pem", client="", secret="", poll_interval=10):
    """
    Join a RTMS session.
    
    Args:
        uuid (str): Meeting UUID
        stream_id (str): RTMS stream ID
        server_urls (str): Server URLs (comma-separated)
        timeout (int, optional): Timeout in milliseconds. Defaults to -1 (no timeout).
        ca (str, optional): CA certificate path. Defaults to "ca.pem".
        client (str, optional): Client ID. If empty, uses ZM_RTMS_CLIENT env var.
        secret (str, optional): Client secret. If empty, uses ZM_RTMS_SECRET env var.
        poll_interval (int, optional): Polling interval in milliseconds. Defaults to 10.
    
    Returns:
        int: 0 for success, non-zero for failure
    """
    global _polling_thread, _polling_interval, _running, _initialized
    
    _log_debug(f"Joining session: uuid={uuid}, stream_id={stream_id}, server_urls={server_urls}")
    
    # Find CA file
    ca_path = os.getenv("ZM_RTMS_CA", ca)
    if not os.path.exists(ca_path):
        _log_debug(f"CA file not found at {ca_path}, looking for system CA files")
        # Try to find system CA files
        system_ca_paths = [
            '/etc/ssl/certs/ca-certificates.crt',  # Debian/Ubuntu
            '/etc/pki/tls/certs/ca-bundle.crt',    # Fedora/RHEL
            '/etc/ssl/ca-bundle.pem',              # OpenSUSE
            '/etc/ssl/cert.pem',                   # Alpine
            '/etc/ssl/certs'                       # Directory with certs
        ]
        for path in system_ca_paths:
            if os.path.exists(path):
                ca_path = path
                _log_debug(f"Found system CA file at {ca_path}")
                break
    
    if not _initialized:
        try:
            _log_debug(f"Initializing RTMS with CA path: {ca_path}")
            _initialize(ca_path)
            _initialized = True
            _log_debug("RTMS initialized successfully")
        except Exception as e:
            _log_debug(f"Failed to initialize RTMS: {e}")
            traceback.print_exc()
            return 1  # Error code
    
    _polling_interval = poll_interval
    
    try:
        signature = generate_signature(client, secret, uuid, stream_id)
        _log_debug(f"Generated signature: {signature}")
        
        _join(uuid, stream_id, signature, server_urls, timeout)

        _log_debug("after join")
        
        # Start polling thread
        _running = True
        _polling_worker()
    
        _log_debug("after thread")

        return 0  # Success
    except Exception as e:
        _log_debug(f"Error joining session: {e}")
        traceback.print_exc()
        return 1  # Error code


def leave():
    """
    Leave the RTMS session.
    
    Returns:
        bool: True if left successfully
    """
    global _running, _polling_thread, _webhook_server
    
    _log_debug("Leaving RTMS session")
    
    _running = False
    if _polling_thread and _polling_thread.is_alive():
        try:
            _log_debug("Stopping polling thread")
            _polling_thread.join(timeout=1)
            _polling_thread = None
        except Exception as e:
            _log_debug(f"Error stopping polling thread: {e}")
    
    if _webhook_server:
        try:
            _log_debug("Shutting down webhook server")
            _webhook_server.shutdown()
            _webhook_server.server_close()
            _webhook_server = None
        except Exception as e:
            _log_debug(f"Error shutting down webhook server: {e}")
    
    try:
        _log_debug("Releasing RTMS client")
        _release()
        return True
    except Exception as e:
        _log_debug(f"Error releasing RTMS client: {e}")
        traceback.print_exc()
        return False


def uninitialize():
    """Uninitialize the RTMS SDK"""
    global _initialized
    
    if _initialized:
        try:
            _log_debug("Uninitializing RTMS SDK")
            _uninitialize()
            _initialized = False
            return True
        except Exception as e:
            _log_debug(f"Error uninitializing RTMS SDK: {e}")
            traceback.print_exc()
            return False
    return True


def uuid():
    """Get the UUID of the current meeting"""
    try:
        return _uuid()
    except Exception as e:
        _log_debug(f"Error getting UUID: {e}")
        return ""


def stream_id():
    """Get the stream ID of the current meeting"""
    try:
        return _stream_id()
    except Exception as e:
        _log_debug(f"Error getting stream ID: {e}")
        return ""


def set_debug(enable=True):
    """Enable or disable debug logging"""
    global _debug
    _debug = enable
    os.environ['RTMS_DEBUG'] = '1' if enable else '0'
    print(f"RTMS debug logging {'enabled' if enable else 'disabled'}")


__all__ = [
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
    "on_leave",
    
    # Classes
    "Session",
    "Participant",
    "Metadata"
]