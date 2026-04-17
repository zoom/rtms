# Node.js API Reference

Complete API reference and examples for the `@zoom/rtms` Node.js package.

**Requirements:** Node.js >= 22.0.0 (Node.js 24 LTS recommended)

For product-specific webhook events and payload fields, see the [product guides](../README.md#supported-products).

## Installation

```bash
npm install @zoom/rtms
```

## Webhook Integration

`onWebhookEvent` sets up an HTTP server that receives Zoom webhook deliveries and passes the parsed payload to your callback. The SDK starts polling for RTMS events automatically after `join()` succeeds.

```javascript
import rtms from "@zoom/rtms";

const clients = new Map();

rtms.onWebhookEvent(({ event, payload }) => {
    const streamId = payload?.rtms_stream_id;

    if (event.includes("rtms_stopped")) {
        clients.get(streamId)?.leave();
        clients.delete(streamId);
        return;
    }

    if (!event.includes("rtms_started")) return;

    const client = new rtms.Client();
    clients.set(streamId, client);

    client.onTranscriptData((data, size, timestamp, metadata) => {
        console.log(`[${timestamp}] ${metadata.userName}: ${data}`);
    });

    client.join(payload);
});
```

For the specific event names for your product, see the [product guides](../README.md#supported-products).

## Webhook Validation

> **⚠️ Required for production.** The example above processes all incoming requests without verification. In production, Zoom cryptographically signs every webhook — you must validate the signature to reject forged requests.

Use the raw callback form `(payload, req, res)` to access headers directly:

```javascript
import rtms from "@zoom/rtms";
import { createHmac, timingSafeEqual } from "crypto";

const WEBHOOK_SECRET = process.env.ZM_RTMS_WEBHOOK_SECRET;
if (!WEBHOOK_SECRET) {
    throw new Error("ZM_RTMS_WEBHOOK_SECRET is not set");
}

function verifySignature(body, timestamp, signature) {
    if (!signature || !timestamp) return false;
    const expected = "v0=" + createHmac("sha256", WEBHOOK_SECRET)
        .update(`v0:${timestamp}:${body}`)
        .digest("hex");
    if (signature.length !== expected.length) return false;
    return timingSafeEqual(Buffer.from(signature), Buffer.from(expected));
}

rtms.onWebhookEvent((payload, req, res) => {
    // Endpoint validation: Zoom POSTs this once when the webhook URL is set
    // or changed. Respond with plainToken + HMAC-SHA256(plainToken, secret)
    // to prove the endpoint holds the secret.
    if (payload.event === "endpoint.url_validation") {
        const plainToken = payload.payload?.plainToken;
        const encryptedToken = createHmac("sha256", WEBHOOK_SECRET)
            .update(plainToken)
            .digest("hex");
        res.writeHead(200, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ plainToken, encryptedToken }));
        return;
    }

    const signature = req.headers["x-zm-signature"];
    const timestamp = req.headers["x-zm-request-timestamp"];

    if (!verifySignature(JSON.stringify(payload), timestamp, signature)) {
        res.writeHead(401);
        res.end(JSON.stringify({ error: "Unauthorized" }));
        return;
    }

    res.writeHead(200, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ status: "ok" }));

    if (payload.event?.includes("rtms_started")) {
        const client = new rtms.Client();
        client.onTranscriptData((data, size, timestamp, metadata) => {
            console.log(`[${timestamp}] ${metadata.userName}: ${data}`);
        });
        client.join(payload.payload);
    }
});
```

## Sharing a Port with an Existing Server

If your app already runs an HTTP server (Express, Fastify, Cloud Run, etc.), mount the webhook handler on your existing port with `createWebhookHandler`:

```javascript
import express from "express";
import rtms from "@zoom/rtms";

const app = express();
app.use(express.json());

app.get("/health", (req, res) => res.json({ status: "ok" }));

const webhookHandler = rtms.createWebhookHandler(
    ({ event, payload }) => {
        if (!event.includes("rtms_started")) return;
        const client = new rtms.Client();
        client.onAudioData((data, size, timestamp, metadata) => {
            console.log(`Audio from ${metadata.userName}: ${data.length}B`);
        });
        client.join(payload);
    },
    "/zoom/webhook"
);

app.post("/zoom/webhook", webhookHandler);
app.listen(8080);
```

`createWebhookHandler` also accepts the raw `(payload, req, res)` form for custom validation.

## Class-Based API

Use `new rtms.Client()` directly for more control or to connect to multiple streams simultaneously:

```javascript
import rtms from "@zoom/rtms";

const client = new rtms.Client();

client.onAudioData((data, size, timestamp, metadata) => {
    console.log(`Received ${data.length} bytes from ${metadata.userName}`);
});

client.join({
    meeting_uuid: "your_meeting_uuid",
    rtms_stream_id: "your_stream_id",
    server_urls:   "wss://example.zoom.us",
});
```

## Media Callbacks

All callbacks receive a `metadata` object with `userId` and `userName`:

```javascript
// Transcript — text data with speaker info
client.onTranscriptData((data, size, timestamp, metadata) => {
    console.log(`[${timestamp}] ${metadata.userName}: ${data}`);
});

// Audio — raw PCM / Opus frames
client.onAudioData((data, size, timestamp, metadata) => {
    console.log(`Audio: ${data.length}B from ${metadata.userName}`);
});

// Video — H.264 / raw frames
client.onVideoData((data, size, timestamp, metadata) => {
    console.log(`Video: ${size}B from ${metadata.userName}`);
});

// Desktop share
client.onDeskshareData((data, size, timestamp, metadata) => {
    console.log(`Deskshare: ${size}B from ${metadata.userName}`);
});
```

> **Speaker identification with mixed audio:** When using the default `AUDIO_MIXED_STREAM`, audio metadata does not identify the current speaker. Use `onActiveSpeakerEvent` to track who is speaking:
>
> ```javascript
> client.onActiveSpeakerEvent((timestamp, userId, userName) => {
>     console.log(`Active speaker: ${userName} (${userId})`);
> });
> ```

## Transcript Language Configuration

By default the SDK auto-detects the spoken language before enabling transcription (~30 seconds). Providing a language hint with `setTranscriptParams` lets transcription begin immediately:

```javascript
// Hint the source language — skips auto-detect, transcription starts immediately
client.setTranscriptParams({ srcLanguage: rtms.TranscriptLanguage.ENGLISH });
```

`TranscriptLanguage` constants: `ENGLISH`, `SPANISH`, `JAPANESE`, `CHINESE_SIMPLIFIED`, and many more. To use auto-detection, omit `setTranscriptParams` or pass `srcLanguage: rtms.TranscriptLanguage.NONE`.

## Media Configuration

By default each stream type uses sensible settings (OPUS audio at 48 kHz, H.264 video at HD/30 fps). Call the relevant `set*Params` method before `join()` to override any field — unspecified fields keep their defaults.

### Video

```javascript
// Switch from the default composite active-speaker stream to per-participant streams
client.setVideoParams({
    dataOpt: rtms.VideoDataOption.VIDEO_SINGLE_INDIVIDUAL_STREAM,
});

// Full control — all fields optional
client.setVideoParams({
    codec:      rtms.VideoCodec.H264,
    resolution: rtms.VideoResolution.HD,
    fps:        30,
    dataOpt:    rtms.VideoDataOption.VIDEO_SINGLE_ACTIVE_STREAM,
});
```

`VideoCodec` constants: `H264`, `JPG`, `PNG`. `VideoResolution` constants: `SD`, `HD`, `FHD`, `QHD`. `VideoDataOption` constants: `VIDEO_SINGLE_ACTIVE_STREAM` (default composite), `VIDEO_SINGLE_INDIVIDUAL_STREAM` (per-participant), `VIDEO_MIXED_GALLERY_VIEW`.

### Audio

```javascript
// Receive a single mixed stream instead of the default per-participant streams
client.setAudioParams({
    dataOpt: rtms.AudioDataOption.AUDIO_MIXED_STREAM,
});
```

`AudioSampleRate` constants: `SR_8K`, `SR_16K`, `SR_32K`, `SR_48K` (default). `AudioChannel` constants: `MONO`, `STEREO` (default). `AudioDataOption` constants: `AUDIO_MULTI_STREAMS` (default, per-participant), `AUDIO_MIXED_STREAM`.

### Desktop Share

```javascript
client.setDeskshareParams({
    codec:      rtms.VideoCodec.H264,
    resolution: rtms.VideoResolution.FHD,
    fps:        5,
});
```

Uses the same `codec`, `resolution`, `fps`, and `dataOpt` fields as video.

## HTTP Proxy

Route RTMS WebSocket traffic through an HTTP proxy. Call `setProxy` before `join()` — it returns `true` on success:

```javascript
import rtms from "@zoom/rtms";

const clients = new Map();

rtms.onWebhookEvent(({ event, payload }) => {
    const streamId = payload?.rtms_stream_id;

    if (event.includes("rtms_stopped")) {
        clients.get(streamId)?.leave();
        clients.delete(streamId);
        return;
    }

    if (!event.includes("rtms_started")) return;

    const client = new rtms.Client();
    clients.set(streamId, client);

    // Route WebSocket traffic through an HTTP proxy.
    // Must be called before join(). Returns true on success.
    const ok = client.setProxy("http", process.env.RTMS_PROXY_URL);
    if (!ok) console.warn("setProxy failed — joining without proxy");

    client.onTranscriptData((data, size, timestamp, metadata) => {
        console.log(`[${timestamp}] ${metadata.userName}: ${data}`);
    });

    client.join(payload);
});
```

The first argument is the proxy type (`"http"`). The second argument is the full proxy URL including host and port.

## Individual Video Streams

By default you receive a single composite stream of the active speaker. To receive per-participant video, first configure `VIDEO_SINGLE_INDIVIDUAL_STREAM`, then subscribe per participant as they join:

```javascript
// Must be called before join() — switches from composite to per-participant streams
client.setVideoParams({
    dataOpt: rtms.VideoDataOption.VIDEO_SINGLE_INDIVIDUAL_STREAM,
});

// Subscribe when a participant joins, unsubscribe when they leave
client.onUserUpdate((op, participant) => {
    if (op === rtms.USER_JOIN && participant?.id) {
        client.subscribeVideo(participant.id, true);
    }
    if (op === rtms.USER_LEAVE && participant?.id) {
        client.subscribeVideo(participant.id, false);
    }
});

// Fires when a participant's video turns on or off
client.onParticipantVideo((userIds, isOn) => {
    console.log(`Video ${isOn ? "on" : "off"} for users: ${userIds}`);
});

// Fires with the subscription result for each subscribeVideo() call
client.onVideoSubscribed((userId, status, error) => {
    console.log(`subscribeVideo(${userId}): status=${status}${error ? " error=" + error : ""}`);
});
```

## Participant Events

```javascript
client.onParticipantEvent((event, timestamp, participants) => {
    for (const p of participants) {
        console.log(`${event} ts=${timestamp} userId=${p.userId} name="${p.userName}"`);
    }
});
```

## Environment Variables

| Variable | Required | Default | Description |
|---|---|---|---|
| `ZM_RTMS_CLIENT` | Yes | — | Your Zoom OAuth Client ID |
| `ZM_RTMS_SECRET` | Yes | — | Your Zoom OAuth Client Secret |
| `ZM_RTMS_PORT` | No | `8080` | Webhook server port |
| `ZM_RTMS_PATH` | No | `/` | Webhook endpoint path |
| `ZM_RTMS_CA` | No | system CA | Path to CA certificate file |
| `ZM_RTMS_LOG_LEVEL` | No | `info` | Log level: `error`, `warn`, `info`, `debug`, `trace` |
| `ZM_RTMS_LOG_FORMAT` | No | `progressive` | Log format: `progressive` or `json` |
| `ZM_RTMS_LOG_ENABLED` | No | `true` | Enable/disable SDK logging |

## Related

- [Python API Reference](python.md)
- [Full API Reference](https://zoom.github.io/rtms/js/)
