# Zoom Contact Center (ZCC)

Real-time media streaming from Zoom Contact Center voice engagements.

## Overview

The API is identical to Meetings. See the [Meetings Quick Start](meetings.md) for full examples.

Key differences:
- Webhook event: `contact_center.voice_rtms_started` instead of `meeting.rtms_started`
- Stop event: `contact_center.voice_rtms_stopped` instead of `meeting.rtms_stopped`

> **Audio Configuration:** This SDK uses different default audio parameters than the raw RTMS WebSocket protocol. See [Audio Configuration](meetings.md#audio-configuration) in the Meetings documentation for details.

## Quick Start

### Node.js

The API is identical to Meetings. See the [Meetings Example](meetings.md) for full examples.

```javascript
import rtms from "@zoom/rtms";

const clients = new Map();

rtms.onWebhookEvent(({ event, payload }) => {
    const streamId = payload?.rtms_stream_id;

    if (event === "contact_center.voice_rtms_stopped") {
        clients.get(streamId)?.leave();
        clients.delete(streamId);
        return;
    }

    if (event !== "contact_center.voice_rtms_started") return;

    const client = new rtms.Client();
    clients.set(streamId, client);

    client.onAudioData((data, size, timestamp, metadata) => {
        console.log(`Contact center audio from ${metadata.userName}`);
    });

    client.onTranscriptData((data, size, timestamp, metadata) => {
        console.log(`[${timestamp}] ${metadata.userName}: ${data}`);
    });

    client.join(payload);
});
```

### Python

The API is identical to Meetings. See the [Meetings Python Quick Start](meetings.md) for full examples.

```python
import rtms

clients = {}

@rtms.on_webhook_event
def handle_webhook(payload):
    event = payload.get('event', '')
    rtms_payload = payload.get('payload', {})
    stream_id = rtms_payload.get('rtms_stream_id')

    if event == 'contact_center.voice_rtms_stopped':
        client = clients.pop(stream_id, None)
        if client:
            client.leave()
        return

    if event != 'contact_center.voice_rtms_started':
        return

    client = rtms.Client()
    clients[stream_id] = client

    @client.on_audio_data
    def on_audio(data, size, timestamp, metadata):
        print(f'Contact center audio from {metadata.userName}: {size} bytes')

    @client.on_transcript_data
    def on_transcript(data, size, timestamp, metadata):
        print(f'[{timestamp}] {metadata.userName}: {data.decode()}')

    client.join(rtms_payload)

rtms.run()
```

## Use Cases

- **Compliance recording** — Archive voice engagement audio and transcripts for regulatory requirements
- **Real-time transcription** — Provide live captions or searchable transcripts for agents and supervisors
- **Sentiment analysis** — Detect customer sentiment during calls to surface coaching opportunities
- **Agent assist** — Feed transcripts to an LLM to suggest responses or retrieve knowledge base articles in real-time
- **Quality assurance** — Automatically score call quality against defined criteria

## Related Documentation

- [Meetings Examples](meetings.md) — Full API examples (same API)
- [Node.js API Reference](node.md) — Webhook validation and advanced patterns
- [Python API Reference](python.md) — asyncio, executor dispatch, and more
- [Main README](../README.md)
