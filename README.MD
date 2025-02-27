# Zoom Realtime Media Streaming (RTMS) SDK

A cross-platform wrapper for the Zoom RTMS C SDK, providing bindings for Node.js, Python, and Go to easily consume real-time audio, video, and transcript streams from Zoom meetings.

## Features

- Process real-time media streams (audio, video, transcripts) from Zoom meetings
- Cross-platform support with bindings for:
  - Node.js
  - Python
  - Go (in development)
- Simple API for joining meetings and processing media data
- Built-in webhook event handling
- Authentication and signature generation
- Docker-based build environment for consistent cross-platform builds

## Prerequisites

- Docker (for building and testing)
- Zoom RTMS C SDK files (should be obtained from Zoom)
- Zoom Developer Credentials (Client ID and Secret)

## Getting Started


### Setup Environment

1. Place the RTMS C SDK files in the `lib/librtmsdk` directory:

```bash
cp rtms_csdk.h rtms_common.h librtmsdk.so ./lib/librtmsdk
```

2. Create a `.env` file with your Zoom credentials:

```bash
ZM_RTMS__CLIENT=        # Your Zoom Client ID
ZM_RTMS_SECRET=    # Your Zoom Client Secret
ZM_RTMS_CA=       # CA Path (you can use system ca)
```

3. Build and run the project using Docker:

```bash
docker compose up
```


## Installation

### Installing the RTMS C SDK

You have several options for installing the RTMS C SDK library (`librtmsdk.so.0`):

#### Option 1: Place in the default location (recommended)

Place the library and headers in the project's `lib/librtmsdk` directory:

```bash
mkdir -p lib/librtmsdk
cp /path/to/librtmsdk.so.0 lib/librtmsdk/
cp /path/to/rtms_csdk.h /path/to/rtms_common.h lib/librtmsdk/h
```

#### Option 2: Install to a system location

```bash
# Copy to system library directory (may require sudo)
sudo cp /path/to/librtmsdk.so.0 /usr/local/lib/
sudo cp /path/to/rtms_csdk.h /path/to/rtms_common.h /usr/local/include/
sudo ldconfig  # Update the library cache
```

#### Option 3: Specify custom location during build

```bash
# When building with CMake directly
cmake -DRTMS_SDK_PATH=/path/to/rtms/sdk ...

# Or when using npm scripts
RTMS_SDK_PATH=/path/to/rtms/sdk npm run build-js
```

#### Option 4: Set LD_LIBRARY_PATH (for runtime only)

```bash
export LD_LIBRARY_PATH=/path/to/rtms/sdk:$LD_LIBRARY_PATH
```

Note: This option only affects runtime location, not build-time. You'll still need to specify the location during build using one of the options above.

## Usage

### Node.js

```javascript
import rtms from 'rtms';

// Join a meeting using environment variables
rtms.join({
  meeting_uuid: process.env.MEETING_UUID,
  rtms_stream_id: process.env.SESSION_ID,
  server_urls: 'wss://rtmsbrokersus.zoom.us:8000',
});

// Process audio data
rtms.onAudioData((buffer, size, timestamp, metadata) => {
  console.log(`Received audio: ${size} bytes from ${metadata.userName}`);
  // Process audio data...
});
```

### Python

```python
import rtms
import os

# Join a meeting using environment variables
rtms.join(
    uuid=os.getenv('MEETING_UUID'),
    stream_id=os.getenv('SESSION_ID'),
    server_urls='wss://rtmsbrokersus.zoom.us:8000'
)

# Process audio data
@rtms.on_audio_data
def handle_audio(buffer, size, timestamp, metadata):
    print(f"Received audio: {size} bytes from {metadata.user_name}")
    # Process audio data...
```

## Development Setup

## Build Modes

The RTMS SDK supports both debug and release build modes:

### Switching Build Modes

You can set the build mode in several ways:

```bash
# Set mode for a single command
npm run rtms build js -- --debug     # Build JS in debug mode
npm run rtms build all -- --release  # Build all in release mode

# Set persistent mode for future commands
npm run debug                        # Switch to debug mode
npm run release                      # Switch to release mode

# Set mode via environment variable
RTMS_BUILD_MODE=debug npm run build  # Build in debug mode
NODE_ENV=development npm run build   # Also triggers debug mode
```

### Current Configuration

To check your current build mode:

```bash
npm run rtms mode
# Current build mode: debug
```

### Build Mode Details

| Mode    | Description                                        | Use Case                   |
|---------|----------------------------------------------------|----------------------------|
| release | Optimized builds with no debug symbols (default)   | Production, benchmarking   |
| debug   | Includes debug symbols, minimal optimization       | Development, troubleshooting |

### Technical Implementation

The build mode affects different build systems as follows:

- **Node.js/C++**: Adds `--debug` flag to cmake-js commands
- **Python**: Sets `-Ccmake.build-type='Debug'` for scikit-build
- **Go**: Passes appropriate debug flags to Go compiler

Build mode is stored persistently in a `.rtms-config.json` file in your project directory and is also passed via the `RTMS_BUILD_MODE` environment variable to all build processes.

### Building from Source

```bash
# Build for Node.js
docker compose up js

# Build for Python
docker compose up py

# Build for Go (in development)
docker compose up go
```

### Testing

```bash
# Run Node.js tests
npm test

# Run Python tests
npm run test-py
```


### Project Structure

```
├── bin/               # Scripts for build and test
├── lib/librtmsdk/     # Zoom RTMS C SDK files
├── src/               # Source code
│   ├── rtms.h         # Core C++ implementation
│   ├── rtms.cpp       # Core C++ implementation
│   ├── node.cpp       # Node.js bindings
│   ├── python.cpp     # Python bindings
│   └── rtms/          # Python package
├── index.ts           # Node.js module entry point
├── CMakeLists.txt     # CMake build configuration
├── Dockerfile         # Docker build environment
├── compose.yaml       # Docker Compose configuration
├── package.json       # Node.js package configuration
└── pyproject.toml     # Python package configuration
```

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request