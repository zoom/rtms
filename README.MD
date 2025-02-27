# Zoom Realtime Media Streaming (RTMS) SDK

A cross-platform wrapper for the Zoom RTMS C SDK, providing bindings for Node.js, Python, and Go to easily consume real-time audio, video, and transcript streams from Zoom meetings.

## Overview

The RTMS SDK allows developers to:

- Connect to live Zoom meetings
- Process real-time media streams (audio, video, transcripts)
- Receive events about session and participant updates
- Build applications that interact with Zoom meetings in real-time

## Quick Start Guide

### 1. Create a Zoom App

Obtain Client ID and Client Secret credentials from your Zoom Marketplace App that has RTMS enabled.

### 2. Obtain the RTMS C SDK

The RTMS C SDK is available from Zoom. Contact your Zoom representative to obtain:
- `rtms_csdk.h`
- `rtms_common.h`
- `librtmsdk.so` (Linux)

### 3. Set Up the Project

Clone this repository:
```bash
git clone https://github.com/zoom/rtms.git
cd rtms
```

Place the RTMS C SDK files in the `lib/librtmsdk` directory:
```bash
cp /path/to/rtms_csdk.h /path/to/rtms_common.h /path/to/librtmsdk.so lib/librtmsdk/
```

#### Option A: Using Docker (Recommended)

Build and run using Docker Compose:
```bash
docker compose up js  # For Node.js
# or
docker compose up py  # For Python
```

#### Option B: Manual Setup (Linux)

Install dependencies:
```bash
# Node.js dependencies
apt update
apt install -y cmake python3-full python3-pip pipx
npm install -g prebuild
pip install "pybind11[global]" python-dotenv pdoc3
```

Install and build:
```bash
npm install
npm run build:js  # For Node.js
# or
npm run build:py  # For Python
```

## Build Modes

The SDK supports both debug and release build modes:

```bash
# Set mode for a single command
npm run rtms build js -- --debug     # Build JS in debug mode
npm run rtms build all -- --release  # Build all in release mode

# Set persistent mode for future commands
npm run debug                        # Switch to debug mode
npm run release                      # Switch to release mode

# Check current build mode
npm run rtms mode
```

## Development Tools

### Using npm Scripts (Recommended for Local Development)

The project includes several npm scripts for common development tasks:

```bash
# Building modules
npm run build         # Build all modules
npm run build:js      # Build only Node.js module
npm run build:py      # Build only Python module
npm run build:debug   # Build all modules in debug mode

# Testing
npm run test          # Run all tests
npm run test:js       # Run Node.js tests
npm run test:py       # Run Python tests
npm run test:manual   # Run manual Node.js test script

# Documentation
npm run docs          # Generate all documentation
npm run docs:js       # Generate Node.js documentation
npm run docs:py       # Generate Python documentation

# Utility
npm run clean         # Clean build artifacts
```

### Using the CLI Tool

For more granular control during local development, the project provides a CLI tool:

```bash
# Get help
npm run rtms help

# Building specific targets with options
npm run rtms build js -- --debug     # Build Node.js module in debug mode
npm run rtms build python -- --release  # Build Python module in release mode
npm run rtms build all                # Build all modules

# Running tests
npm run rtms test js                  # Test Node.js module
npm run rtms test python              # Test Python module
npm run rtms test js:manual           # Run manual JS test

# Managing build mode
npm run rtms mode debug               # Set debug mode for future builds
npm run rtms mode release             # Set release mode for future builds
npm run rtms mode                     # Show current build mode

# Cleaning artifacts
npm run rtms clean                    # Clean all build artifacts
```

### Docker Development

When developing with Docker, use Docker Compose commands instead of the CLI tool:

```bash
# Build and test JavaScript module
docker compose up js

# Build and test Python module
docker compose up py

# Build and test Go module
docker compose up go

# Development mode with automatic rebuilds
docker compose up --build js

# Run specific services in the background
docker compose up -d js
```

The Docker Compose configuration includes file watching for development, automatically rebuilding when files change.

## Authentication and Signing

Create a `.env` file with your Zoom credentials:
```bash
ZM_RTMS_CLIENT=your_client_id
ZM_RTMS_SECRET=your_client_secret
```

The SDK follows this authentication logic priority:

1. Looks for environment variables (`ZM_RTMS_CLIENT` and `ZM_RTMS_SECRET`) first
2. If not found, uses client and secret parameters provided in the join method
3. If a signature is provided directly as an argument, that overrides both options above

### Automatic Signature Generation

```javascript
// Node.js - signature generated automatically if client and secret provided
rtms.join({
  meeting_uuid: 'your_meeting_uuid',
  rtms_stream_id: 'your_stream_id',
  server_urls: 'wss://signaling-fake.zoom.us:8000',
  client: 'your_client_id',        // Will use this to generate signature
  secret: 'your_client_secret'     // Will use this to generate signature
});
```

```python
# Python - signature generated automatically if client and secret provided
rtms.join(
    uuid='your_meeting_uuid',
    stream_id='your_stream_id',
    server_urls='wss://signaling-fake.zoom.us:8000',
    client='your_client_id',       # Will use this to generate signature
    secret='your_client_secret'    # Will use this to generate signature
)
```

### Manual Signature Generation

```javascript
// Node.js manual signature generation
const signature = rtms.generateSignature({
  client: 'your_client_id',
  secret: 'your_client_secret',
  uuid: 'meeting_uuid',
  streamId: 'stream_id'
});

// Then pass it to join
rtms.join({
  meeting_uuid: 'meeting_uuid',
  rtms_stream_id: 'stream_id',
  server_urls: 'wss://signaling-fake.zoom.us:8000',
  signature: signature  // Pass pre-generated signature
});
```

```python
# Python manual signature generation
signature = rtms.generate_signature(
    client='your_client_id',
    secret='your_client_secret',
    uuid='meeting_uuid',
    stream_id='stream_id'
)

# Then pass it to join
rtms.join(
    uuid='meeting_uuid',
    stream_id='stream_id',
    server_urls='wss://signaling-fake.zoom.us:8000',
    signature=signature  # Pass pre-generated signature
)
```

## Basic Usage

### Node.js

There are two ways to use the SDK in Node.js:

#### 1. Global Singleton (Single-Instance)

```javascript
import rtms from 'rtms';

rtms.onAudioData((buffer, size, timestamp, metadata) => {
  console.log(`Received audio: ${size} bytes from ${metadata.userName}`);
  // Process audio data...
});

rtms.join({
  meeting_uuid: 'your_meeting_uuid',
  rtms_stream_id: 'your_stream_id',
  server_urls: 'wss://signaling-fake.zoom.us:8000',               // Polling interval in ms
});

// Later, leave the meeting
rtms.leave();
```

#### 2. Client Class (Multi-Instance)

```javascript
import rtms from 'rtms';

const client = new rtms.Client();

client.onAudioData((data timestamp, metadata) => {
  console.log(`Client received audio: ${data.length} bytes from ${metadata.userName}`);
  // Process audio data...
});

// Join a meeting using environment variables
client.join({
  meeting_uuid: 'your_meeting_uuid',
  rtms_stream_id: 'your_stream_id',
  server_urls: 'wss://signaling-fake.zoom.us:8000',
});


// Later, leave the meeting
client.leave();
```

### Python

Python provides a similar API with decorator-based event handlers:

```python
import rtms
import os

# Define event handlers using decorators
@rtms.on_audio_data
def handle_audio(buffer, size, timestamp, metadata):
    print(f"Received audio: {size} bytes from {metadata.user_name}")
    # Process audio data...
    
# Join a meeting using environment variables
rtms.join(
    uuid='your_meeting_uuid',
    stream_id='your_stream_id',
    server_urls='wss://signaling-fake.zoom.us:8000',
)

# Later, leave the meeting
rtms.leave()
```

## Documentation

Generate documentation for different language bindings:

```bash
# Generate Node.js docs
npm run docs:js

# Generate Python docs
npm run docs:py
```

## Working with Media

### Audio

#### JavaScript
```javascript
// Set up audio processing
rtms.onAudioData((buffer, size, timestamp, metadata) => {
  // The buffer contains raw audio frames
  // Common format is 16-bit PCM at 16kHz mono
  console.log(`Received ${size} bytes of audio from ${metadata.userName}`);
  
  // Process audio data as needed:
  // - Save to file
  // - Forward to a real-time processing service
  // - Analyze locally
});
```

#### Python
```python
@rtms.on_audio_data
def handle_audio(buffer, size, timestamp, metadata):
    # The buffer contains raw audio frames
    # Common format is 16-bit PCM at 16kHz mono
    print(f"Received {size} bytes of audio from {metadata.user_name}")
    
    # Process audio data as needed:
    # - Save to file
    # - Forward to a real-time processing service
    # - Analyze locally
```

### Video

#### JavaScript
```javascript
// Set up video processing
rtms.onVideoData((buffer, size, timestamp, trackId, metadata) => {
  // The buffer contains encoded video frames (typically H.264)
  console.log(`Received ${size} bytes of video from ${metadata.userName}`);
  
  // Process video data as needed:
  // - Save to file
  // - Decode frames
  // - Forward to a real-time processing service
});
```

#### Python
```python
@rtms.on_video_data
def handle_video(buffer, size, timestamp, track_id, metadata):
    # The buffer contains encoded video frames (typically H.264)
    print(f"Received {size} bytes of video from {metadata.user_name}")
    
    # Process video data as needed:
    # - Save to file
    # - Decode frames
    # - Forward to a real-time processing service
```

### Transcripts

#### JavaScript
```javascript
// Set up transcript processing
rtms.onTranscriptData((buffer, size, timestamp, metadata) => {
  // The buffer contains transcript data in JSON format
  const transcriptText = buffer.toString('utf8');
  const transcript = JSON.parse(transcriptText);
  
  console.log(`Transcript from ${metadata.userName}: ${transcript.text}`);
});
```

#### Python
```python
@rtms.on_transcript_data
def handle_transcript(buffer, size, timestamp, metadata):
    # The buffer contains transcript data in JSON format
    transcript_text = buffer.decode('utf-8')
    transcript = json.loads(transcript_text)
    
    print(f"Transcript from {metadata.user_name}: {transcript['text']}")
```

## Troubleshooting

### Common Issues

1. **Library Not Found**: Ensure `librtmsdk.so` is in `lib/librtmsdk/` or a system library path.

2. **Authentication Errors**: Verify your Client ID and Secret are correct.

3. **Connection Issues**: Check that your server URL is correct and accessible.

4. **Build Failures**: Make sure all dependencies are installed.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Need Help?

If you encounter issues or have questions:

1. Check the documentation in the `/docs` directory
2. Run tests to verify your setup: `npm run test:js` or `npm run test:py`
3. Reach out to the Zoom Developer Forum community at https://devforum.zoom.us/
4. Check out Zoom's official documentation at https://developers.zoom.us/docs/
5. Open an issue on GitHub