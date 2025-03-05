# Zoom Realtime Media Streaming (RTMS) SDK

A cross-platform wrapper for the Zoom RTMS C SDK, providing bindings for Node.js, Python, and Go to easily consume real-time audio, video, and transcript streams from Zoom meetings.

## Overview

The RTMS SDK allows developers to:

- Connect to live Zoom meetings
- Process real-time media streams (audio, video, transcripts)
- Receive events about session and participant updates
- Build applications that interact with Zoom meetings in real-time


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

The Python package offers a decorator-based API for handling RTMS events and data streams.

## Usage

### Node.js - Webhook Integration

Easily respond to Zoom webhooks and connect to RTMS streams:

```javascript
import rtms from "@zoom/rtms";

rtms.onWebhookEvent(({event, payload}) => {
    if (event !== "meeting.rtms.started") return;

    const client = new rtms.Client();
    
    client.onAudioData((data, timestamp, metadata) => {
        console.log(`Received audio: ${data.length} bytes from ${metadata.userName}`);
    });

    client.join(payload);
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

### Python Integration

Python offers a similar experience with decorator-based event handling:

```python
import rtms

@rtms.on_webhook_event()
def handle_webhook(payload):
    if payload.get('event') != 'meeting.rtms.started':
        return
    
    client = rtms.Client()
    
    @client.on_audio_data()
    def handle_audio(buffer, size, timestamp, metadata):
        print(f"Received audio: {size} bytes")
    
    client.join(payload)
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

# Place SDK files in lib/librtmsdk/

# Build and run using Docker Compose
docker compose up js  # For Node.js
# or
docker compose up py  # For Python
```

Docker Compose provides an isolated build environment with all necessary dependencies preconfigured, making it the simplest way to build the SDK.

### Building Locally

#### Prerequisites
- Node.js (>= 22.14.0)
- Python 3.8+ with pip
- CMake 3.25+
- C/C++ build tools
- Zoom RTMS C SDK files (contact Zoom for access)

#### Steps
```bash
# Install system dependencies
apt update
apt install -y cmake python3-full python3-pip pipx
npm install -g prebuild
pip install "pybind11[global]" python-dotenv pdoc3

# Clone and set up the repository
git clone https://github.com/zoom/rtms.git
cd rtms

# Place SDK files in lib/librtmsdk/

# Install project dependencies and build
npm install
npm run build:js  # For Node.js
# or
npm run build:py  # For Python
```

### Development Commands

The project includes several npm scripts for common development tasks:

```bash
# Building modules
npm run build         # Build all modules
npm run build:js      # Build only Node.js module
npm run build:py      # Build only Python module

# Testing
npm run test          # Run all tests
npm run test:js       # Run Node.js tests
npm run test:py       # Run Python tests

# Build modes
npm run debug         # Switch to debug mode
npm run release       # Switch to release mode (default)
npm run rtms mode     # Check current build mode
```

These commands help you manage different aspects of the build process and testing workflow.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.