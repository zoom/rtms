import os
import json
import hmac
import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer

from ._rtms import _init, _join, _is_init, on_join_confirm, on_session_update, \
    on_audio_data, on_video_data, on_transcript_data, on_leave

server = port = path = None  # Global server reference

class WebhookHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # Validate request path
        if self.path != path:
            self.send_response(404)
            self.wfile.write(b"Not Found")
            return

        # Read and parse request body
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length)

        try:
            payload = json.loads(body.decode('utf-8'))
            self.server.callback(payload) 

            self.send_response(200)
            self.wfile.write(b'Webhook received successfully')
        except json.JSONDecodeError as e:
            print("Error parsing webhook JSON:", e)
            self.send_response(400)
            self.wfile.write(b"Invalid JSON received")


def on_webhook_event(callback):
    global server
    if server:
        print("Server is already running.")
        return

    port = int(os.getenv('ZM_RTMS_PORT', 8080)) 
    path = os.getenv('ZM_RTMS_PATH', '/')

    server = HTTPServer(('0.0.0.0', port), WebhookHandler)
    server.callback = callback

    print(f"🚀 Listening for events at http://localhost:{port}{path}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down the server.")
        server.server_close()


def generate_signature(client, secret, uuid, stream_id):
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


def join(uuid, stream_id, server_urls, timeout = -1, ca="ca.pem", client="", secret=""):
    ca_path = os.getenv("ZM_RTMS_CA", ca)
    if not _is_init():
        _init(ca_path)
    
    signature = generate_signature(client, secret, uuid, stream_id)
    return _join(uuid, stream_id, signature, server_urls, timeout)


__all__ = [
    "join", 
    "on_webhook_event"
    "on_join_confirm",
    "on_session_update",
    "on_audio_data",
    "on_video_data",
    "on_transcript_data",
    "on_leave"
]