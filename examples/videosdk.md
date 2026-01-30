# Zoom Video SDK Examples

Real-time media streaming from custom Zoom Video SDK applications.

## Overview

The RTMS SDK provides the same API for accessing real-time media streams from applications built with Zoom Video SDK. This enables you to build custom video experiences and still access the underlying media streams for processing, recording, or integration.

> **Audio Configuration:** This SDK uses different default audio parameters than the raw RTMS WebSocket protocol. See [Audio Configuration](meetings.md#audio-configuration) in the Meetings documentation for details on SDK defaults vs WebSocket defaults.

## Quick Start

### Node.js

The API is identical to Meetings. See the [Meetings Node.js Quick Start](meetings.md) for full examples.

Key differences:
- Webhook event: `session.rtms_started` (Video SDK uses session events)
- Webhook payload contains `session_id` instead of `meeting_uuid`
- Join parameters come from Video SDK webhook payload

```javascript
import rtms from "@zoom/rtms";

rtms.onWebhookEvent(({event, payload}) => {
    if (event !== "session.rtms_started") return;

    const client = new rtms.Client();

    client.onAudioData((data, timestamp, metadata) => {
        console.log(`Video SDK audio from ${metadata.userName}`);
    });

    client.onVideoData((data, timestamp, metadata) => {
        console.log(`Video SDK video: ${metadata.width}x${metadata.height}`);
    });

    // Video SDK payload contains session_id (not meeting_uuid)
    // The SDK accepts both - just pass the payload directly
    client.join(payload);
});
```

### Python

The API is identical to Meetings. See the [Meetings Python Quick Start](meetings.md) for full examples.

Key differences:
- Webhook event: `session.rtms_started` (Video SDK uses session events)
- Webhook payload contains `session_id` instead of `meeting_uuid`

```python
import rtms

client = rtms.Client()

@client.on_webhook_event()
def handle_webhook(payload):
    if payload.get('event') == 'session.rtms_started':
        rtms_payload = payload.get('payload', {})
        client.join(
            # Video SDK uses session_id instead of meeting_uuid
            session_id=rtms_payload.get('session_id'),
            rtms_stream_id=rtms_payload.get('rtms_stream_id'),
            server_urls=rtms_payload.get('server_urls')
        )

@client.onAudioData
def on_audio(data, size, timestamp, metadata):
    print(f'Video SDK audio from {metadata.userName}: {size} bytes')

@client.onVideoData
def on_video(data, size, timestamp, metadata):
    print(f'Video SDK video: {metadata.width}x{metadata.height}')
```

## Use Cases

Common use cases for Video SDK RTMS:

- **Custom recording** - Build your own recording solution with custom branding
- **AI/ML integration** - Feed video/audio to ML models for real-time analysis
- **Multi-platform distribution** - Stream to multiple platforms simultaneously
- **Custom effects** - Apply real-time audio/video effects
- **Analytics** - Track custom engagement metrics
- **Compliance** - Implement industry-specific recording and archival requirements

## Video SDK Integration

When building custom video applications with Zoom Video SDK, you can enable RTMS to access the underlying media streams. This allows you to:

1. Build custom UI with Video SDK
2. Access raw media streams via RTMS
3. Process, record, or integrate streams as needed

This combination provides maximum flexibility for custom video experiences while maintaining access to raw media data.

## Related Documentation

- [Meetings Examples](meetings.md) - Full API examples (same API)
- [Webinars Examples](webinars.md) - Webinar streaming
- [Main README](../README.md) - Installation and setup

## Resources

- [Zoom Video SDK Documentation](https://developers.zoom.us/docs/video-sdk/)
- [Zoom RTMS Documentation](https://developers.zoom.us/docs/api/rest/reference/zoom-api/methods/#tag/RTMS)
- [Video SDK GitHub](https://github.com/zoom/videosdk-sample-signature-node.js)
