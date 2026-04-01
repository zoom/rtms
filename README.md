# Zoom Realtime Media Streams (RTMS) SDK

Bindings for real-time audio, video, and transcript streams from Zoom Meetings

[![npm](https://img.shields.io/npm/v/@zoom/rtms)](https://www.npmjs.com/package/@zoom/rtms)
[![PyPI](https://img.shields.io/pypi/v/rtms)](https://pypi.org/project/rtms/)
[![docs](https://img.shields.io/badge/docs-online-blue)](https://zoom.github.io/rtms/)

## Supported Products

The RTMS SDK works with multiple Zoom products:

- **[Zoom Meetings](examples/meetings.md)** - Real-time streams from Zoom Meetings
- **[Zoom Webinars](examples/webinars.md)** - Broadcast-quality streams from Zoom Webinars
- **[Zoom Video SDK](examples/videosdk.md)** - Custom video experiences with RTMS access

See [examples/](examples/) for complete guides and code samples.

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

**⚠️ Requirements: Node.js >= 20.3.0 (Node.js 24 LTS recommended)**

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
nvm install 24       # Install Node.js 24 LTS (recommended)
nvm use 24

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

# Install from PyPI
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

## For Contributors

**This project uses [Task](https://taskfile.dev) (go-task) for development builds and testing.**

If you're an **end user** installing via npm or pip, you don't need Task - the installation will work automatically using prebuilt binaries.

If you're a **contributor** building from source, you'll need to install Task:

**macOS:**
```bash
brew install go-task
```

**Linux:**
```bash
sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d -b ~/.local/bin
```

**Quick Start for Contributors:**
```bash
# Verify your environment meets requirements
task doctor

# Setup the project (fetch SDK, install dependencies)
task setup

# Build for your platform
task build:js          # Build Node.js bindings
task build:py          # Build Python bindings

# Run tests
task test:js           # Test Node.js
task test:py           # Test Python

# See all available commands
task --list
```

For detailed contribution guidelines, build instructions, and troubleshooting, see [CONTRIBUTING.md](CONTRIBUTING.md).

## Usage

> **Speaker Identification with Mixed Audio**
>
> When using `AUDIO_MIXED_STREAM` (the default), all participants are mixed into a single audio stream and the audio callback's metadata will **not** identify the current speaker. To identify who is speaking, register an `onActiveSpeakerEvent` callback — it fires whenever the active speaker changes. See [Troubleshooting #7](#7-identifying-speakers-with-mixed-audio-streams) and [#80](https://github.com/zoom/rtms/issues/80) for details.

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

### Node.js - Sharing Ports with Existing Servers

If you need to integrate webhook handling with your existing Express, Fastify, or other HTTP server (useful for Cloud Run, Kubernetes, or any deployment requiring a single port), use `createWebhookHandler`:

```javascript
import express from 'express';
import rtms from '@zoom/rtms';

const app = express();
app.use(express.json());

// Your existing application routes
app.get('/health', (req, res) => {
    res.json({ status: 'healthy' });
});

app.get('/admin', (req, res) => {
    res.json({ admin: 'panel' });
});

// Create a webhook handler that can be mounted on your existing server
const webhookHandler = rtms.createWebhookHandler(
    (payload) => {
        console.log(`Received webhook: ${payload.event}`);

        if (payload.event === "meeting.rtms_started") {
            const client = new rtms.Client();
            client.onAudioData((data, timestamp, metadata) => {
                console.log(`Audio from ${metadata.userName}`);
            });
            client.join(payload.payload);
        }
    },
    '/zoom/webhook'  // Path to handle
);

// Mount the webhook handler on your Express app
app.post('/zoom/webhook', webhookHandler);

// Single port for all routes
const PORT = process.env.PORT || 8080;
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
    console.log(`Webhook endpoint: http://localhost:${PORT}/zoom/webhook`);
    console.log(`Health check: http://localhost:${PORT}/health`);
});
```

You can also use `RawWebhookCallback` with `createWebhookHandler` for custom validation:

```javascript
const webhookHandler = rtms.createWebhookHandler(
    (payload, req, res) => {
        // Custom validation with raw HTTP access
        const signature = req.headers['x-zoom-signature'];

        if (!validateSignature(payload, signature)) {
            res.writeHead(401);
            res.end('Unauthorized');
            return;
        }

        // Process webhook...

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'ok' }));
    },
    '/zoom/webhook'
);

app.post('/zoom/webhook', webhookHandler);
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
from dotenv import load_dotenv

load_dotenv()

@rtms.on_webhook_event
def handle_webhook(payload):
    if payload.get('event') != 'meeting.rtms_started':
        return

    client = rtms.Client()

    @client.on_join_confirm
    def on_join(reason):
        print(f'Joined with reason: {reason}')

    @client.on_transcript_data
    def on_transcript(data, size, timestamp, metadata):
        print(f'[{metadata.userName}]: {data.decode()}')

    @client.on_leave
    def on_leave(reason):
        print(f'Left meeting: {reason}')

    client.join(payload['payload'])

if __name__ == '__main__':
    print('Listening for webhooks on http://localhost:8080')
    rtms.run()  # Blocks; Ctrl-C to stop
```

### Python - Context Manager

Use `with rtms.Client() as client:` to ensure `leave()` is always called — even if an exception occurs:

```python
import rtms

with rtms.Client() as client:
    client.on_audio_data(lambda data, size, ts, meta: process(data))
    client.join(meeting_uuid=..., rtms_stream_id=..., server_urls=...)
    rtms.run(stop_on_empty=True)
# client.leave() called automatically on exit
```

### Python - asyncio Integration

`run_async()` is a drop-in replacement for `run()` that uses `asyncio.sleep()` instead of `time.sleep()`, so it composes naturally with aiohttp, FastAPI, asyncpg, and any other async framework on a shared event loop:

```python
import asyncio
import rtms
from aiohttp import web

routes = web.RouteTableDef()

@routes.post('/webhook')
async def webhook(request):
    payload = await request.json()
    if payload.get('event') == 'meeting.rtms_started':
        client = rtms.Client()
        client.on_transcript_data(lambda d, s, t, m: print(m.userName, d.decode()))
        client.join(payload['payload'])
    return web.Response(text='ok')

async def main():
    app = web.Application()
    app.add_routes(routes)
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, port=8080)
    await site.start()
    await rtms.run_async()   # yields control between polls — no blocking

asyncio.run(main())
```

Async callbacks are detected automatically and scheduled on the running event loop:

```python
client = rtms.Client()

async def save_audio(data, size, timestamp, metadata):
    await db.insert('audio', data)   # fully non-blocking

client.on_audio_data(save_audio)     # coroutine detected, dispatched via loop
```

### Python - Executor-Based Callback Dispatch

For CPU-bound or I/O-heavy callbacks that should not block the poll loop, pass a `concurrent.futures.Executor`:

```python
from concurrent.futures import ThreadPoolExecutor
import rtms

# Per-client executor
client = rtms.Client(executor=ThreadPoolExecutor(max_workers=8))
client.on_audio_data(heavy_audio_processor)   # dispatched to thread pool
client.on_transcript_data(store_transcript)   # dispatched to thread pool

# Global default for all clients in the event loop
rtms.run(executor=ThreadPoolExecutor(max_workers=32))
```

Callbacks without an executor continue to run inline — identical to v1.0 behavior.

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
- Task installed (or use Docker's Task installation)

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

# Build using Docker Compose with Task
docker compose run --rm build task build:js    # Build Node.js for linux-x64
docker compose run --rm build task build:py    # Build Python wheel for linux-x64

# Or use convenience services
docker compose run --rm test-js                # Build and test Node.js
docker compose run --rm test-py                # Build and test Python
```

Docker Compose creates **distributable packages** for linux-x64 (prebuilds for Node.js, wheels for Python). Use this when developing on macOS to build Linux packages for distribution.

### Building Locally

#### Prerequisites
- Node.js (>= 20.3.0, LTS recommended)
- Python 3.10+ with pip (for Python build)
- CMake 3.25+
- C/C++ build tools
- Task (go-task) - https://taskfile.dev/installation/
- Zoom RTMS C SDK files (contact Zoom for access)

#### Steps
```bash
# Install system dependencies
## macOS
brew install cmake go-task python@3.13 node@24

## Linux
sudo apt update
sudo apt install -y cmake python3-full python3-pip npm
sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d -b ~/.local/bin

# Clone and set up the repository
git clone https://github.com/zoom/rtms.git
cd rtms

# Place SDK files in the appropriate lib directory
# lib/linux-x64/ or lib/darwin-arm64/

# Verify your environment meets requirements
task doctor

# Setup project (fetches SDK if not present, installs dependencies)
task setup

# Build for specific language and platform
task build:js             # Build Node.js for current platform
task build:js:linux       # Build Node.js for Linux (via Docker)
task build:js:darwin      # Build Node.js for macOS

task build:py             # Build Python for current platform
task build:py:linux       # Build Python wheel for Linux (via Docker)
task build:py:darwin      # Build Python wheel for macOS
```

### Development Commands

The project uses Task (go-task) for build orchestration. Commands follow the pattern: `task <action>:<lang>:<platform>`

```bash
# See all available commands
task --list

# Verify environment
task doctor                   # Check Node, Python, CMake, Docker versions

# Setup project
task setup                    # Fetch SDK and install dependencies

# Build modes
BUILD_TYPE=Debug task build:js    # Build in debug mode
BUILD_TYPE=Release task build:js  # Build in release mode (default)

# Debug logging for C SDK callbacks
RTMS_DEBUG=ON task build:js       # Enable verbose callback logging
```

## Python Scaling Strategy

The v1.1 Python SDK provides a layered set of primitives for scaling from a single meeting to hundreds of concurrent streams. Use the layer that fits your workload:

### Layer 1 — Single process, inline callbacks (default, zero config)

Appropriate for < ~20 concurrent meetings with lightweight callbacks (logging, forwarding, simple storage).

```python
import rtms

@rtms.on_webhook_event
def handle(payload):
    if payload.get('event') != 'meeting.rtms_started':
        return
    client = rtms.Client()
    client.on_transcript_data(lambda d, s, t, m: print(m.userName, d.decode()))
    client.join(payload['payload'])

rtms.run()
```

- **Event loop**: `rtms.run()` — synchronous, single-threaded
- **Callbacks**: inline, no dispatch overhead
- **GIL**: released during `poll()`, so other Python threads (e.g. the webhook HTTP server) stay responsive

### Layer 2 — Single process, executor dispatch

Appropriate for CPU-bound callbacks (ML inference, audio processing) or I/O callbacks (database writes, S3 uploads) that would otherwise block the poll loop.

```python
from concurrent.futures import ThreadPoolExecutor
import rtms

executor = ThreadPoolExecutor(max_workers=32)

@rtms.on_webhook_event
def handle(payload):
    if payload.get('event') != 'meeting.rtms_started':
        return
    client = rtms.Client(executor=executor)
    client.on_audio_data(run_asr_model)          # dispatched to thread pool
    client.on_transcript_data(write_to_database) # dispatched to thread pool
    client.join(payload['payload'])

rtms.run()
```

- **Event loop**: `rtms.run()` — poll loop stays fast; callbacks run in the pool
- **Thread pool size**: tune `max_workers` to match your callback latency profile
- **Back-pressure**: executor queue depth limits unbounded growth

### Layer 3 — asyncio native

Appropriate when the rest of your stack is already async (aiohttp, FastAPI, asyncpg). Callbacks declared as `async def` are auto-scheduled on the running event loop.

```python
import asyncio
import rtms

async def on_transcript(data, size, timestamp, metadata):
    await db.execute('INSERT INTO transcripts VALUES ($1, $2)', metadata.userId, data)

@rtms.on_webhook_event
def handle(payload):
    if payload.get('event') != 'meeting.rtms_started':
        return
    client = rtms.Client()
    client.on_transcript_data(on_transcript)   # coroutine auto-detected
    client.join(payload['payload'])

async def main():
    await asyncio.gather(
        rtms.run_async(),
        start_web_server(),
    )

asyncio.run(main())
```

- **Event loop**: `rtms.run_async()` — `await asyncio.sleep()` between polls, never blocks
- **Coroutine dispatch**: `asyncio.run_coroutine_threadsafe` bridges SDK callbacks to the loop
- **Composable**: runs alongside any other async service on the same loop

### Layer 4 — Multi-process

Appropriate for 50+ concurrent meetings. Each process runs its own `rtms.run()` loop; a load balancer routes webhook events to available workers.

```
Zoom → Load Balancer (nginx/HAProxy)
         ├── Worker 0 (rtms.run(), meetings 0..N)
         ├── Worker 1 (rtms.run(), meetings N..2N)
         └── Worker 2 (rtms.run(), meetings 2N..3N)
```

Use a message queue (Redis pub/sub, RabbitMQ, Kafka) to distribute join requests:

```python
# worker.py — one process per worker
import rtms, redis

r = redis.Redis()
sub = r.pubsub()
sub.subscribe('rtms:join')

@rtms.on_webhook_event
def handle(payload):
    # Publish to queue; a free worker picks it up
    r.publish('rtms:join', json.dumps(payload))

rtms.run()
```

### Choosing the Right Layer

| Workload | Recommended Layer |
|---|---|
| Prototyping / light callbacks | Layer 1 — inline |
| CPU-bound (ASR, video) | Layer 2 — executor |
| Async framework (FastAPI, aiohttp) | Layer 3 — run_async |
| 50+ concurrent meetings | Layer 4 — multi-process |

All layers use the same `Client` API. You can mix layers — e.g. Layer 3 for the web server plus Layer 2 executors for heavy callbacks — without any API changes.

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
nvm install 24           # Install Node.js 24 LTS (recommended)
nvm use 24

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
- Use recommended version with `.nvmrc`: `nvm use` (Node.js 24 LTS)
- Check version before installing: `node --version`

### 2. Platform Support
Verify you're using a supported platform (darwin-arm64 or linux-x64)

### 3. SDK Files
Ensure RTMS C SDK files are correctly placed in the appropriate lib directory

### 4. Build Mode
Try both debug and release modes (`npm run debug` or `npm run release`)

### 5. Dependencies
Verify all prerequisites are installed

### 6. Audio Defaults Mismatch
This SDK uses different default audio parameters than the raw RTMS WebSocket protocol for better out-of-the-box quality. If you need to match the WebSocket protocol defaults, see [#92](https://github.com/zoom/rtms/issues/92) for details.

### 7. Identifying Speakers with Mixed Audio Streams
When using `AUDIO_MIXED_STREAM`, the audio callback's metadata does not identify the current speaker since all participants are mixed into a single stream. To identify who is speaking, use the `onActiveSpeakerEvent` callback:

**Node.js:**
```javascript
client.onActiveSpeakerEvent((timestamp, userId, userName) => {
    console.log(`Active speaker: ${userName} (${userId})`);
});
```

**Python:**
```python
@client.onActiveSpeakerEvent
def on_active_speaker(timestamp, user_id, user_name):
    print(f"Active speaker: {user_name} ({user_id})")
```

This callback notifies your application whenever the active speaker changes in the meeting. You can also use the lower-level `onEventEx` function with the active speaker event type directly. See [#80](https://github.com/zoom/rtms/issues/80) for more details.

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
