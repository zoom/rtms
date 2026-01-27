# RTMS SDK Examples

Product-specific examples for using the Zoom Real-Time Media Streaming (RTMS) SDK.

## Supported Products

The RTMS SDK works with multiple Zoom products. Choose the product you're working with:

### Zoom Meetings
Real-time streams from Zoom Meetings
- [Quick Start Guide](meetings.md) - Node.js and Python examples

### Zoom Webinars
Broadcast-quality streams from Zoom Webinars
- [Webinars Overview](webinars.md)

### Zoom Video SDK
Custom video experiences with RTMS access
- [Video SDK Overview](videosdk.md)

## Common Features

All products support the same RTMS SDK API:

- **Audio Streaming** - Real-time audio data from participants
- **Video Streaming** - Video frames with metadata
- **Transcripts** - Live transcription data
- **Session Events** - Start, stop, pause, resume notifications
- **Participant Events** - Join, leave, and status updates
- **Webhook Integration** - Easy webhook handling with built-in server

## Installation

### Node.js
```bash
npm install @zoom/rtms
```

Requirements: Node.js >= 20.3.0 (Node.js 24 LTS recommended)

### Python
```bash
pip install rtms
```

Requirements: Python >= 3.10 (Python 3.12 or 3.13 recommended)

## Quick Start

Choose your product and language:

| Product | Node.js | Python |
|---------|---------|--------|
| **Meetings** | [Quick Start](meetings.md) | [Quick Start](meetings.md) |
| **Webinars** | [Overview](webinars.md) | [Overview](webinars.md) |
| **Video SDK** | [Overview](videosdk.md) | [Overview](videosdk.md) |

## Example Files

- **meetings.md** - Comprehensive guide with Node.js and Python examples
- **webinars.md** - Webinars overview (uses same API as Meetings)
- **videosdk.md** - Video SDK overview (uses same API with session events)

## Documentation

- [Main README](../README.md) - Installation, setup, troubleshooting
- [API Documentation](https://zoom.github.io/rtms/js/) - Full Node.js API reference
- [PUBLISHING.md](../PUBLISHING.md) - For maintainers
- [CONTRIBUTING.md](../CONTRIBUTING.md) - For contributors

## Support

- **Issues**: [GitHub Issues](https://github.com/zoom/rtms/issues)
- **Discussions**: [GitHub Discussions](https://github.com/zoom/rtms/discussions)
- **Zoom Developers**: [developers.zoom.us](https://developers.zoom.us/)

## Contributing

Have a great example to share? Contributions are welcome! See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.
