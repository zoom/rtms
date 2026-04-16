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
- **[Zoom Contact Center](examples/zcc.md)** - Real-time streams from Zoom Contact Center voice engagements

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

**⚠️ Requirements: Node.js >= 22.0.0 (Node.js 24 LTS recommended)**

The RTMS SDK uses N-API versions 9 and 10, which require Node.js 22.0.0 or higher.

```bash
# Check your Node.js version
node --version

# Install the package
npm install @zoom/rtms
```

#### Using NVM
**If you're using an older version of Node.js:**
```bash
# Using nvm (recommended)
nvm install 24       # Install Node.js 24 LTS (recommended)
nvm use 24

# Or install Node.js 22 LTS (minimum supported)
nvm install 22
nvm use 22

# Reinstall the package
npm install @zoom/rtms
```

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

## Usage

Regardless of the language that you use the RTMS SDK follows the same basic steps: 

 - Receive and validate the webhook event
 - Create an RTMS SDK Client 
 - Assign data callbacks to that client
 - Join the meeting with the event payload

### Configure

All SDK languages read from the environment or a `.env` file:

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

### Node.js

For more details, see our [Quickstart App](https://github.com/zoom/rtms-quickstart-js)

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

> **Production note:** The example above does not validate the incoming webhook signature. Zoom cryptographically signs every webhook — production apps must verify signatures before processing. See [Webhook Validation](examples/node.md#webhook-validation)

### Python 

For more details, see our [Quickstart App](https://github.com/zoom/rtms-quickstart-py)

#### Environment Setup

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

#### Quickstart


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

> **Production note:** The example above does not validate the incoming webhook signature. Zoom cryptographically signs every webhook — production apps must verify signatures before processing. See [Webhook Validation](examples/python.md#webhook-validation) 

## Troubleshooting

If you encounter issues some of these steps may help.

### 1. Segmentation Fault / Crash on Startup

**Symptoms:**
- Immediate crash when requiring/importing the module
- Error message: `Segmentation fault (core dumped)`
- Stack trace shows `napi_module_register_by_symbol`

**Root Cause:** Using Node.js version < 22.0.0

**Solution:** See [Using NVM](#using-nvm)


**Prevention:**
- Always use Node.js 22.0.0 or higher
- Use recommended version with `.nvmrc`: `nvm use` (Node.js 24 LTS)
- Check version before installing: `node --version`

### 2. Platform Support
Verify you're using a supported platform (darwin-arm64 or linux-x64)

### 3. SDK Files
Ensure RTMS C++ SDK files are correctly placed in the appropriate lib directory

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

## Building from Source

The RTMS SDK can be built from source using either Docker (recommended) or local build tools.

### Using Docker (Recommended)

#### Prerequisites
- Docker and Docker Compose
- Zoom RTMS C++ SDK files (contact Zoom for access)
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
- Node.js (>= 22.0.0, LTS recommended)
- Python 3.10+ with pip (for Python build)
- CMake 3.25+
- C/C++ build tools
- Task (go-task) - https://taskfile.dev/installation/
- Zoom RTMS C++ SDK files (contact Zoom for access)

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

# Debug logging for C++ SDK callbacks
RTMS_DEBUG=ON task build:js       # Enable verbose callback logging
```

## For Contributors

For detailed contribution guidelines, build instructions, and troubleshooting, see [CONTRIBUTING.md](CONTRIBUTING.md).

## For Maintainers

If you're a maintainer looking to build, test, or publish new releases of the RTMS SDK, please refer to [PUBLISHING.md](PUBLISHING.md)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
