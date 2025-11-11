# Zoom Realtime Media Streams (RTMS) SDK

Bindings for real-time audio, video, and transcript streams from Zoom Meetings

[![npm](https://img.shields.io/npm/v/@zoom/rtms)](https://www.npmjs.com/package/@zoom/rtms)
[![docs](https://img.shields.io/badge/docs-online-blue)](https://zoom.github.io/rtms/js/)

## Platform Support Status

| Language | Status | Supported Platforms |
|----------|--------|---------------------|
| Node.js | ✅ Supported | darwin-arm64, linux-x64 |
| Python | ✅ Supported | darwin-arm64, linux-x64 |
| Go | 📅 Planned | - |

> We are actively working to expand both language and platform support in future releases.

## Overview

The RTMS SDK allows developers to:

- Connect to live Zoom meetings
- Process real-time media streams (audio, video, transcripts)
- Receive events about session and participant updates
- Build applications that interact with Zoom meetings in real-time
- Handle webhook events with full control over validation and responses

## Installation

### Node.js
```bash
npm install @zoom/rtms
```

The Node.js package provides both class-based and singleton APIs for connecting to RTMS streams.

### Python
```bash
pip install rtms
```

The Python package provides a Pythonic decorator-based API with full feature parity to Node.js.

## Usage

### Node.js - Webhook Integration

Easily respond to Zoom webhooks and connect to RTMS streams:

```javascript
import rtms from "@zoom/rtms";

// CommonJS
// const rtms = require('@zoom/rtms').default;

rtms.onWebhookEvent(({event, payload}) => {
    if (event !== "meeting.rtms_started") return;

    const client = new rtms.Client();
    
    client.onAudioData((data, timestamp, metadata) => {
        console.log(`Received audio: ${data.length} bytes from ${metadata.userName}`);
    });

    client.join(payload);
});
```

### Node.js - Advanced Webhook Handling

For advanced use cases requiring custom webhook validation or response handling (e.g., Zoom's webhook validation challenge), you can use the enhanced callback with raw HTTP access:

```javascript
import rtms from "@zoom/rtms";

rtms.onWebhookEvent((payload, req, res) => {
    // Access request headers for webhook validation
    const signature = req.headers['x-zoom-signature'];
    
    // Handle Zoom's webhook validation challenge
    if (req.headers['x-zoom-webhook-validator']) {
        const validationToken = req.headers['x-zoom-webhook-validator'];
        
        // Echo back the validation token
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ plainToken: validationToken }));
        return;
    }
    
    // Custom validation logic
    if (!validateWebhookSignature(payload, signature)) {
        res.writeHead(401, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Invalid signature' }));
        return;
    }
    
    // Process the webhook payload
    if (payload.event === "meeting.rtms_started") {
        const client = new rtms.Client();
        
        client.onAudioData((data, timestamp, metadata) => {
            console.log(`Received audio from ${metadata.userName}`);
        });
        
        client.join(payload.payload);
    }
    
    // Send custom response
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ status: 'ok' }));
});
```

### Node.js - Class-Based Approach

For greater control or connecting to multiple streams simultaneously:

```javascript
import rtms from "@zoom/rtms";

const client = new rtms.Client();

client.onAudioData((data, timestamp, metadata) => {
    console.log(`Received audio: ${data.length} bytes`);
});

client.join({
    meeting_uuid: "your_meeting_uuid",
    rtms_stream_id: "your_stream_id",
    server_urls: "wss://example.zoom.us",
});
```

### Node.js - Global Singleton

When you only need to connect to a single RTMS stream:

```javascript
import rtms from "@zoom/rtms";

rtms.onAudioData((data, timestamp, metadata) => {
    console.log(`Received audio from ${metadata.userName}`);
});

rtms.join({
    meeting_uuid: "your_meeting_uuid",
    rtms_stream_id: "your_stream_id",
    server_urls: "wss://rtms.zoom.us"
});
```

### Python - Basic Usage

```python
#!/usr/bin/env python3
import rtms
import signal
import sys
from dotenv import load_dotenv

load_dotenv()

client = rtms.Client()

# Graceful shutdown handler
def signal_handler(sig, frame):
    print('\nShutting down gracefully...')
    client.leave()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

# Webhook event handler
@client.on_webhook_event()
def handle_webhook(payload):
    if payload.get('event') == 'meeting.rtms_started':
        rtms_payload = payload.get('payload', {})
        client.join(
            meeting_uuid=rtms_payload.get('meeting_uuid'),
            rtms_stream_id=rtms_payload.get('rtms_stream_id'),
            server_urls=rtms_payload.get('server_urls'),
            signature=rtms_payload.get('signature')
        )

# Callback handlers
@client.onJoinConfirm
def on_join(reason):
    print(f'Joined meeting: {reason}')

@client.onTranscriptData
def on_transcript(data, size, timestamp, metadata):
    text = data.decode('utf-8')
    print(f'[{metadata.userName}]: {text}')

@client.onLeave
def on_leave(reason):
    print(f'Left meeting: {reason}')

if __name__ == '__main__':
    print('Webhook server running on http://localhost:8080')
    import time
    while True:
        # Process queued join requests from webhook thread
        client._process_join_queue()
        # Poll for SDK events
        client._poll_if_needed()
        time.sleep(0.01)
```

### Python - Advanced Webhook Validation

For production use cases requiring custom webhook validation:

```python
import rtms
import hmac
import hashlib

client = rtms.Client()

@client.on_webhook_event()
def handle_webhook(payload, request, response):
    # Access request headers for validation
    signature = request.headers.get('x-zoom-signature')

    # Handle Zoom's webhook validation challenge
    if request.headers.get('x-zoom-webhook-validator'):
        validator = request.headers['x-zoom-webhook-validator']
        response.set_status(200)
        response.send({'plainToken': validator})
        return

    # Custom signature validation
    if not validate_signature(payload, signature):
        response.set_status(401)
        response.send({'error': 'Invalid signature'})
        return

    # Process valid webhook
    if payload.get('event') == 'meeting.rtms_started':
        client.join(payload.get('payload'))

    response.send({'status': 'ok'})
```

### Python - Environment Setup

Create a virtual environment and install dependencies:

```bash
# Create virtual environment
python3 -m venv .venv

# Activate virtual environment
source .venv/bin/activate  # On Windows: .venv\Scripts\activate

# Install dependencies
pip install python-dotenv

# Install RTMS SDK (when published)
pip install rtms
```

Create a `.env` file:

```bash
# Required - Your Zoom OAuth credentials
ZM_RTMS_CLIENT=your_client_id
ZM_RTMS_SECRET=your_client_secret

# Optional - Webhook server configuration
ZM_RTMS_PORT=8080
ZM_RTMS_PATH=/webhook

# Optional - Logging configuration
ZM_RTMS_LOG_LEVEL=debug          # error, warn, info, debug, trace
ZM_RTMS_LOG_FORMAT=progressive    # progressive or json
ZM_RTMS_LOG_ENABLED=true          # true or false
```

## Building from Source

The RTMS SDK can be built from source using either Docker (recommended) or local build tools.

### Using Docker (Recommended)

#### Prerequisites
- Docker and Docker Compose
- Zoom RTMS C SDK files (contact Zoom for access)

#### Steps
```bash
# Clone the repository
git clone https://github.com/zoom/rtms.git
cd rtms

# Place your SDK library files in the lib/{arch} folder
# For linux-x64:
cp ../librtmsdk.0.2025xxxx/librtmsdk.so.0 lib/linux-x64

# For darwin-arm64 (Apple Silicon):
cp ../librtmsdk.0.2025xxxx/librtmsdk.dylib lib/darwin-arm64

# Place the include files in the proper directory
cp ../librtmsdk.0.2025xxxx/h/* lib/include

# Build and run using Docker Compose
docker compose up js  # For Node.js
# or
docker compose up py  # For Python (experimental)
```

Docker Compose provides an isolated build environment with all necessary dependencies preconfigured, making it the simplest way to build the SDK.

### Building Locally

#### Prerequisites
- Node.js (>= 22.14.0)
- Python 3.8+ with pip (for Python build)
- CMake 3.25+
- C/C++ build tools
- Zoom RTMS C SDK files (contact Zoom for access)

#### Steps
```bash
# Install system dependencies
sudo apt update
sudo apt install -y cmake python3-full python3-pip pipx npm
npm install -g prebuild
pip install "pybind11[global]" python-dotenv pdoc3

# Clone and set up the repository
git clone https://github.com/zoom/rtms.git
cd rtms

# Place SDK files in the appropriate lib directory
# lib/linux-x64/ or lib/darwin-arm64/

# Install project dependencies and build
npm install
npm run build:js  # For Node.js (fully supported)
# or
npm run build:py  # For Python (experimental)
```

### Development Commands

The project includes several npm scripts for common development tasks:

```bash
# Building modules
npm run build         # Build all modules
npm run build:js      # Build only Node.js module
npm run build:py      # Build only Python module (experimental)

# Testing
npm run test          # Run all tests
npm run test:js       # Run Node.js tests
npm run test:py       # Run Python tests (experimental)

# Build modes
npm run debug         # Switch to debug mode
npm run release       # Switch to release mode (default)
npm run rtms mode     # Check current build mode
```

These commands help you manage different aspects of the build process and testing workflow.

## Troubleshooting

If you encounter issues:

1. **Platform Support**: Verify you're using a supported platform (darwin-arm64 or linux-x64)
2. **SDK Files**: Ensure RTMS C SDK files are correctly placed in the appropriate lib directory
3. **Build Mode**: Try both debug and release modes (`npm run debug` or `npm run release`)
4. **Dependencies**: Verify all prerequisites are installed

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
