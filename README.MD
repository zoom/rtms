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

## Node.js Version Support

| Node.js Version | N-API Version | Status | Notes |
|-----------------|---------------|--------|-------|
| 22.x (LTS)      | 10            | ✅ Fully Supported | **Recommended** |
| 20.3.0+         | 9-10          | ✅ Fully Supported | **Minimum version**, LTS until April 2026 |
| 20.0.0 - 20.2.x  | 8            | ❌ Not Supported | N-API v9 unavailable |
| 18.x and older   | 8 or older   | ❌ Not Supported | **EOL April 2025** - No security updates |
| 16.x and older   | < 9          | ❌ Not Supported | N-API version too old |

**Why the strict requirements?**
This package requires N-API version 9 or 10. Using older Node.js versions will result in segmentation faults or module loading errors.

**Recommended versions:**
- **Node.js 22 LTS** (latest, best performance)
- **Node.js 20 LTS** (stable, long-term support, minimum supported)

## Overview

The RTMS SDK allows developers to:

- Connect to live Zoom meetings
- Process real-time media streams (audio, video, transcripts)
- Receive events about session and participant updates
- Build applications that interact with Zoom meetings in real-time
- Handle webhook events with full control over validation and responses

## Installation

### Node.js

**⚠️ Requirements: Node.js >= 20.3.0 (Node.js 20 LTS or 22 LTS)**

The RTMS SDK uses N-API versions 9 and 10, which require Node.js 20.3.0 or higher.

```bash
# Check your Node.js version
node --version

# Install the package
npm install @zoom/rtms
```

**If you're using an older version of Node.js:**
```bash
# Using nvm (recommended)
nvm install 22       # Install Node.js 22 LTS (recommended)
nvm use 22

# Or install Node.js 20 LTS (minimum)
nvm install 20
nvm use 20

# Reinstall the package
npm install @zoom/rtms
```

**Download Node.js:** https://nodejs.org/

The Node.js package provides both class-based and singleton APIs for connecting to RTMS streams.

### Python

**⚠️ Requirements: Python >= 3.10 (Python 3.10, 3.11, 3.12, or 3.13)**

The RTMS SDK requires Python 3.10 or higher.

```bash
# Check your Python version
python3 --version

# Install from TestPyPI (Current)
pip install -i https://test.pypi.org/simple/ rtms

# Install from PyPI (Coming Soon)
pip install rtms
```

**If you're using an older version of Python:**
```bash
# Using pyenv (recommended)
pyenv install 3.12
pyenv local 3.12

# Or using your system's package manager
# Ubuntu/Debian: sudo apt install python3.12
# macOS: brew install python@3.12
```

**Download Python:** https://www.python.org/downloads/

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

# Install RTMS SDK
pip install -i https://test.pypi.org/simple/ rtms
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
docker compose up js  # Create Node.js prebuild for linux-x64
# or
docker compose up py  # Create Python wheel for linux-x64
```

Docker Compose creates **distributable packages** for linux-x64 (prebuilds for Node.js, wheels for Python). Use this when developing on macOS to build Linux packages for distribution.

### Building Locally

#### Prerequisites
- Node.js (>= 20.3.0, LTS recommended)
- Python 3.10+ with pip (for Python build)
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

# Build for specific language and platform
npm run build:js          # Build Node.js for current platform
npm run build:js:linux    # Build Node.js for Linux
npm run build:js:darwin   # Build Node.js for macOS

npm run build:py          # Build Python for current platform
npm run build:py:linux    # Build Python wheel for Linux
npm run build:py:darwin   # Build Python wheel for macOS
```

### Development Commands

The project uses a unified command structure across all languages: `<action>:<lang>:<platform>`

```bash
# Building modules
npm run build:js              # Build Node.js for current platform
npm run build:js:linux        # Build Node.js for Linux
npm run build:js:darwin       # Build Node.js for macOS

npm run build:py              # Build Python for current platform
npm run build:py:linux        # Build Python wheel for Linux
npm run build:py:darwin       # Build Python wheel for macOS

npm run build:go              # Build Go for current platform (coming soon)
npm run build:go:linux        # Build Go for Linux (coming soon)
npm run build:go:darwin       # Build Go for macOS (coming soon)

# Creating prebuilds for distribution
npm run prebuild:js           # Create Node.js prebuilds for all platforms
npm run prebuild:js:linux     # Create Node.js prebuild for Linux
npm run prebuild:js:darwin    # Create Node.js prebuild for macOS

npm run prebuild:py           # Create Python wheels (same as build)
npm run prebuild:py:linux     # Create Python wheel for Linux
npm run prebuild:py:darwin    # Create Python wheel for macOS

# Uploading to package registries
npm run upload:js             # Upload Node.js prebuilds to GitHub releases
npm run upload:js:linux       # Upload Node.js Linux prebuild
npm run upload:js:darwin      # Upload Node.js macOS prebuild

npm run upload:py             # Upload Python wheels to TestPyPI
npm run upload:py:linux       # Upload Python Linux wheel to TestPyPI
npm run upload:py:darwin      # Upload Python macOS wheel to TestPyPI

npm run upload:py:prod        # Upload Python wheels to production PyPI
npm run upload:py:prod:linux  # Upload Python Linux wheel to PyPI
npm run upload:py:prod:darwin # Upload Python macOS wheel to PyPI

# Testing
npm run test                  # Run all tests
npm run test:js               # Run Node.js tests
npm run test:py               # Run Python tests

# Build modes
npm run debug                 # Switch to debug mode
npm run release               # Switch to release mode (default)
npm run rtms mode             # Check current build mode
```

These commands help you manage different aspects of the build process and testing workflow. The unified structure makes it easy to build, package, and publish for multiple languages and platforms.

## Troubleshooting

If you encounter issues:

### 1. Segmentation Fault / Crash on Startup

**Symptoms:**
- Immediate crash when requiring/importing the module
- Error message: `Segmentation fault (core dumped)`
- Stack trace shows `napi_module_register_by_symbol`

**Root Cause:** Using Node.js version < 20.3.0

**Solution:**
```bash
# 1. Check your Node.js version
node --version

# 2. If < 20.3.0, upgrade to a supported version

# Using nvm (recommended):
nvm install 22           # Install Node.js 22 LTS (recommended)
nvm use 22

# Or install minimum version:
nvm install 20
nvm use 20

# Or download from: https://nodejs.org/

# 3. Clear npm cache and reinstall
npm cache clean --force
rm -rf node_modules package-lock.json
npm install
```

**Prevention:**
- Always use Node.js 20.3.0 or higher
- Use recommended version with `.nvmrc`: `nvm use` (Node.js 22)
- Check version before installing: `node --version`

### 2. Platform Support
Verify you're using a supported platform (darwin-arm64 or linux-x64)

### 3. SDK Files
Ensure RTMS C SDK files are correctly placed in the appropriate lib directory

### 4. Build Mode
Try both debug and release modes (`npm run debug` or `npm run release`)

### 5. Dependencies
Verify all prerequisites are installed

## For Maintainers

If you're a maintainer looking to build, test, or publish new releases of the RTMS SDK, please refer to [PUBLISHING.md](PUBLISHING.md) for comprehensive documentation on:

- Building platform-specific wheels and prebuilds
- Publishing to npm and PyPI
- GitHub Actions CI/CD workflow
- Testing procedures
- Troubleshooting common issues
- Release workflows for Node.js and Python

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
