# Zoom Webinars Examples

Real-time media streaming from Zoom Webinars.

## Overview

The RTMS SDK provides the same API for accessing real-time media streams from Zoom Webinars. The key difference is in how you obtain the webhook payload and join parameters from Zoom's Webinar RTMS webhooks.

> **Audio Configuration:** This SDK uses different default audio parameters than the raw RTMS WebSocket protocol. See [Audio Configuration](meetings.md#audio-configuration) in the Meetings documentation for details on SDK defaults vs WebSocket defaults.

## Quick Start

### Node.js

The API is identical to Meetings. See the [Meetings Node.js Quick Start](meetings.md) for full examples.

Key differences:
- Webhook event: `webinar.rtms_started` instead of `meeting.rtms_started`
- Join parameters come from Webinar webhook payload

```javascript
import rtms from "@zoom/rtms";

rtms.onWebhookEvent(({event, payload}) => {
    if (event !== "webinar.rtms_started") return;

    const client = new rtms.Client();

    client.onAudioData((data, timestamp, metadata) => {
        console.log(`Webinar audio from ${metadata.userName}`);
    });

    client.join(payload);
});
```

### Python

The API is identical to Meetings. See the [Meetings Python Quick Start](meetings.md) for full examples.

Key differences:
- Webhook event: `webinar.rtms_started` instead of `meeting.rtms_started`

```python
import rtms

client = rtms.Client()

@client.on_webhook_event()
def handle_webhook(payload):
    if payload.get('event') == 'webinar.rtms_started':
        rtms_payload = payload.get('payload', {})
        client.join(
            meeting_uuid=rtms_payload.get('meeting_uuid'),
            rtms_stream_id=rtms_payload.get('rtms_stream_id'),
            server_urls=rtms_payload.get('server_urls')
        )

@client.onAudioData
def on_audio(data, size, timestamp, metadata):
    print(f'Webinar audio from {metadata.userName}: {size} bytes')
```

## Use Cases

Common use cases for Webinar RTMS:

- **Live transcription** - Provide real-time captions for large audiences
- **Content moderation** - Monitor webinar content in real-time
- **Analytics** - Track engagement and participation patterns
- **Archival** - Record and process webinar content
- **Integration** - Connect webinar streams to other systems

## Related Documentation

- [Meetings Examples](meetings.md) - Full API examples (same API)
- [Video SDK Examples](videosdk.md) - Custom video app streaming
- [Main README](../README.md) - Installation and setup

## Resources

- [Zoom Webinars Documentation](https://developers.zoom.us/docs/api/rest/reference/zoom-api/methods/#tag/Webinars)
- [Zoom RTMS Documentation](https://developers.zoom.us/docs/api/rest/reference/zoom-api/methods/#tag/RTMS)
