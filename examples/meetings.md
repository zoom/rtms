# Zoom Meetings - RTMS Quick Start

Get started with processing real-time media streams from Zoom Meetings.

## Prerequisites

### Node.js
- Node.js >= 20.3.0 (Node.js 24 LTS recommended)
- Zoom OAuth App with RTMS permissions
- npm or yarn

### Python
- Python >= 3.10 (Python 3.12 or 3.13 recommended)
- Zoom OAuth App with RTMS permissions
- pip

## Installation

### Node.js
```bash
npm install @zoom/rtms
```

### Python
```bash
pip install rtms
```

## Basic Usage

### Node.js - Webhook Integration

Easily respond to Zoom webhooks and connect to RTMS streams:

```javascript
import rtms from "@zoom/rtms";

rtms.onWebhookEvent(({event, payload}) => {
    if (event !== "meeting.rtms_started") return;

    const client = new rtms.Client();

    client.onAudioData((data, timestamp, metadata) => {
        console.log(`Received audio: ${data.length} bytes from ${metadata.userName}`);
    });

    client.join(payload);
});
```

### Python - Webhook Integration

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

@client.onAudioData
def on_audio(data, size, timestamp, metadata):
    print(f'Audio from {metadata.userName}: {size} bytes')

if __name__ == '__main__':
    print('Webhook server running on http://localhost:8080')
    import time
    while True:
        client._process_join_queue()
        client._poll_if_needed()
        time.sleep(0.01)
```

## Advanced Webhook Handling

### Node.js - Custom Validation

For production use cases requiring custom webhook validation:

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

If you need to integrate webhook handling with your existing Express, Fastify, or other HTTP server:

```javascript
import express from 'express';
import rtms from '@zoom/rtms';

const app = express();
app.use(express.json());

// Your existing application routes
app.get('/health', (req, res) => {
    res.json({ status: 'healthy' });
});

// Create webhook handler
const webhookHandler = rtms.createWebhookHandler(
    (payload) => {
        if (payload.event === "meeting.rtms_started") {
            const client = new rtms.Client();
            client.onAudioData((data, timestamp, metadata) => {
                console.log(`Audio from ${metadata.userName}`);
            });
            client.join(payload.payload);
        }
    },
    '/zoom/webhook'
);

// Mount the webhook handler
app.post('/zoom/webhook', webhookHandler);

// Single port for all routes
const PORT = process.env.PORT || 8080;
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
```

### Python - Custom Validation

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

## Processing Media Streams

### Node.js - Audio Processing

```javascript
import rtms from "@zoom/rtms";
import fs from "fs";

const client = new rtms.Client();
const audioStream = fs.createWriteStream("meeting-audio.raw");

client.onAudioData((data, timestamp, metadata) => {
    console.log(`Audio from ${metadata.userName}: ${data.length} bytes`);
    audioStream.write(data);
});

client.join({
    meeting_uuid: "your_meeting_uuid",
    rtms_stream_id: "your_stream_id",
    server_urls: "wss://rtms.zoom.us"
});
```

### Python - Audio Processing

```python
import rtms

client = rtms.Client()
audio_file = open('meeting-audio.raw', 'wb')

@client.onAudioData
def on_audio(data, size, timestamp, metadata):
    print(f'Audio from {metadata.userName}: {size} bytes')
    audio_file.write(data)

client.join(
    meeting_uuid='your_meeting_uuid',
    rtms_stream_id='your_stream_id',
    server_urls='wss://rtms.zoom.us'
)

import time
while True:
    client._poll_if_needed()
    time.sleep(0.01)
```

### Node.js - Video Processing

```javascript
import rtms from "@zoom/rtms";

const client = new rtms.Client();

client.onVideoData((data, timestamp, metadata) => {
    console.log(`Video frame from ${metadata.userName}:`);
    console.log(`  Size: ${data.length} bytes`);
    console.log(`  Resolution: ${metadata.width}x${metadata.height}`);
    console.log(`  Format: ${metadata.format}`);
});

client.join({
    meeting_uuid: "your_meeting_uuid",
    rtms_stream_id: "your_stream_id",
    server_urls: "wss://rtms.zoom.us"
});
```

### Python - Video Processing

```python
import rtms

client = rtms.Client()

@client.onVideoData
def on_video(data, size, timestamp, metadata):
    print(f'Video frame from {metadata.userName}:')
    print(f'  Size: {size} bytes')
    print(f'  Resolution: {metadata.width}x{metadata.height}')
    print(f'  Format: {metadata.format}')

client.join(
    meeting_uuid='your_meeting_uuid',
    rtms_stream_id='your_stream_id',
    server_urls='wss://rtms.zoom.us'
)

import time
while True:
    client._poll_if_needed()
    time.sleep(0.01)
```

### Node.js - Transcript Processing

```javascript
import rtms from "@zoom/rtms";

const client = new rtms.Client();

client.onTranscriptData((data, timestamp, metadata) => {
    const text = data.toString('utf-8');
    console.log(`[${metadata.userName}]: ${text}`);
});

client.join({
    meeting_uuid: "your_meeting_uuid",
    rtms_stream_id: "your_stream_id",
    server_urls: "wss://rtms.zoom.us"
});
```

### Python - Transcript Processing

```python
import rtms

client = rtms.Client()

@client.onTranscriptData
def on_transcript(data, size, timestamp, metadata):
    text = data.decode('utf-8')
    print(f'[{metadata.userName}]: {text}')

client.join(
    meeting_uuid='your_meeting_uuid',
    rtms_stream_id='your_stream_id',
    server_urls='wss://rtms.zoom.us'
)

import time
while True:
    client._poll_if_needed()
    time.sleep(0.01)
```

## Session and Participant Events

### Node.js

```javascript
import rtms from "@zoom/rtms";

const client = new rtms.Client();

// Session lifecycle
client.onJoinConfirm((reason) => {
    console.log("Joined meeting:", reason);
});

client.onSessionUpdate((event, session) => {
    console.log(`Session event: ${event}`);
    console.log(`Session ID: ${session.sessionId}`);
    console.log(`Meeting ID: ${session.meetingId}`);
});

// Participant events
client.onParticipantEvent((event, timestamp, participants) => {
    participants.forEach(p => {
        console.log(`User ${event}: ${p.userName} (${p.userId})`);
    });
});

client.onLeave((reason) => {
    console.log("Left meeting:", reason);
});

client.join({
    meeting_uuid: "your_meeting_uuid",
    rtms_stream_id: "your_stream_id",
    server_urls: "wss://rtms.zoom.us"
});
```

### Python

```python
import rtms

client = rtms.Client()

@client.onJoinConfirm
def on_join(reason):
    print(f'Joined meeting: {reason}')

@client.onSessionUpdate
def on_session(event, session):
    print(f'Session event: {event}')
    print(f'Session ID: {session.sessionId}')
    print(f'Meeting ID: {session.meetingId}')

@client.onParticipantEvent
def on_participant(event, timestamp, participants):
    for p in participants:
        print(f'User {event}: {p.get("user_name")} ({p.get("user_id")})')

@client.onLeave
def on_leave(reason):
    print(f'Left meeting: {reason}')

client.join(
    meeting_uuid='your_meeting_uuid',
    rtms_stream_id='your_stream_id',
    server_urls='wss://rtms.zoom.us'
)

import time
while True:
    client._poll_if_needed()
    time.sleep(0.01)
```

## Environment Setup

### Node.js

Create a `.env` file:

```bash
# Required
ZM_RTMS_CLIENT=your_client_id
ZM_RTMS_SECRET=your_client_secret

# Optional
ZM_RTMS_PORT=8080
ZM_RTMS_PATH=/webhook
ZM_RTMS_LOG_LEVEL=debug
```

### Python

```bash
# Create virtual environment
python3 -m venv .venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate

# Install dependencies
pip install python-dotenv rtms
```

Create a `.env` file:

```bash
# Required
ZM_RTMS_CLIENT=your_client_id
ZM_RTMS_SECRET=your_client_secret

# Optional
ZM_RTMS_PORT=8080
ZM_RTMS_PATH=/webhook
ZM_RTMS_LOG_LEVEL=debug
ZM_RTMS_LOG_FORMAT=progressive
ZM_RTMS_LOG_ENABLED=true
```

## Next Steps

- See the [full Node.js API documentation](https://zoom.github.io/rtms/js/)
- Check out [Webinars examples](webinars.md)
- Learn about [Video SDK integration](videosdk.md)
- Return to [main README](../README.md)
