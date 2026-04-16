# Python API Reference

Complete API reference and examples for the `rtms` Python package.

**Requirements:** Python >= 3.10

For product-specific webhook events and payload fields, see the [product guides](../README.md#supported-products).

## Installation

```bash
pip install rtms
```

## Webhook Integration

The `@rtms.on_webhook_event` decorator sets up an HTTP server that receives Zoom webhook deliveries. The SDK starts polling for RTMS events automatically after `join()` succeeds.

```python
import rtms

@rtms.on_webhook_event
def handle_webhook(payload):
    if 'rtms_started' not in payload.get('event', ''):
        return

    client = rtms.Client()

    @client.on_transcript_data
    def on_transcript(data, size, timestamp, metadata):
        print(f'[{timestamp}] {metadata.userName}: {data.decode()}')

    client.join(payload['payload'])

rtms.run()  # blocks; Ctrl-C to stop
```

For the specific event names for your product, see the [product guides](../README.md#supported-products).

## Webhook Validation

> **⚠️ Required for production.** The example above processes all incoming requests without verification. In production, Zoom cryptographically signs every webhook — you must validate the signature to reject forged requests.

```python
import rtms
import hmac
import hashlib
import os

def verify_signature(body: str, timestamp: str, signature: str) -> bool:
    message = f"v0:{timestamp}:{body}"
    expected = "v0=" + hmac.new(
        os.environ["ZM_RTMS_WEBHOOK_SECRET"].encode(),
        message.encode(),
        hashlib.sha256
    ).hexdigest()
    return hmac.compare_digest(expected, signature)

@rtms.on_webhook_event
def handle_webhook(payload, request, response):
    signature = request.headers.get('x-zm-signature', '')
    timestamp = request.headers.get('x-zm-request-timestamp', '')

    # Zoom endpoint validation challenge
    validator = request.headers.get('x-zm-webhook-validator')
    if validator:
        response.set_status(200)
        response.send({'plainToken': validator})
        return

    if not verify_signature(str(payload), timestamp, signature):
        response.set_status(401)
        response.send({'error': 'Unauthorized'})
        return

    response.send({'status': 'ok'})

    if 'rtms_started' in payload.get('event', ''):
        client = rtms.Client()
        client.on_transcript_data(
            lambda data, size, ts, meta: print(f'{meta.userName}: {data.decode()}')
        )
        client.join(payload['payload'])

rtms.run()
```

## Context Manager

Use `with rtms.Client() as client:` to ensure `leave()` is always called — even if an exception occurs:

```python
import rtms

with rtms.Client() as client:
    client.on_audio_data(lambda data, size, ts, meta: process(data))
    client.join(
        meeting_uuid=...,
        rtms_stream_id=...,
        server_urls=...
    )
    rtms.run(stop_on_empty=True)
# client.leave() called automatically on exit
```

## Media Callbacks

All callbacks receive a `metadata` object with `userId` and `userName`:

```python
# Transcript — text data with speaker info
@client.on_transcript_data
def on_transcript(data, size, timestamp, metadata):
    print(f'[{timestamp}] {metadata.userName}: {data.decode()}')

# Audio — raw PCM / Opus frames
@client.on_audio_data
def on_audio(data, size, timestamp, metadata):
    print(f'Audio: {len(data)}B from {metadata.userName}')

# Video — H.264 / raw frames
@client.on_video_data
def on_video(data, size, timestamp, metadata):
    print(f'Video: {size}B from {metadata.userName}')

# Desktop share
@client.on_deskshare_data
def on_deskshare(data, size, timestamp, metadata):
    print(f'Deskshare: {size}B from {metadata.userName}')
```

> **Speaker identification with mixed audio:** When using the default `AUDIO_MIXED_STREAM`, audio metadata does not identify the current speaker. Use `on_active_speaker_event` to track who is speaking:
>
> ```python
> @client.on_active_speaker_event
> def on_speaker(timestamp, user_id, user_name):
>     print(f'Active speaker: {user_name} ({user_id})')
> ```

## Media Configuration

By default each stream type uses sensible settings (OPUS audio at 48 kHz, H.264 video at HD/30 fps). Call the relevant `set_*_params` method before `join()` to override any field — unspecified fields keep their defaults.

### Video

```python
# Switch from the default composite active-speaker stream to per-participant streams
params = rtms.VideoParams()
params.data_opt = rtms.DataOption.VIDEO_SINGLE_INDIVIDUAL_STREAM
client.set_video_params(params)

# Full control — set only the fields you want to change
params = rtms.VideoParams()
params.codec      = rtms.VideoCodec.H264
params.resolution = rtms.VideoResolution.HD
params.fps        = 30
params.data_opt   = rtms.DataOption.VIDEO_SINGLE_ACTIVE_STREAM
client.set_video_params(params)
```

`VideoCodec` constants: `H264`, `JPG`, `PNG`. `VideoResolution` constants: `SD`, `HD`, `FHD`, `QHD`. `DataOption` video constants: `VIDEO_SINGLE_ACTIVE_STREAM` (default composite), `VIDEO_SINGLE_INDIVIDUAL_STREAM` (per-participant), `VIDEO_MIXED_GALLERY_VIEW`.

### Audio

```python
# Receive a single mixed stream instead of the default per-participant streams
params = rtms.AudioParams()
params.data_opt = rtms.DataOption.AUDIO_MIXED_STREAM
client.set_audio_params(params)
```

`AudioSampleRate` constants: `SR_8K`, `SR_16K`, `SR_32K`, `SR_48K` (default). `AudioChannel` constants: `MONO`, `STEREO` (default). `DataOption` audio constants: `AUDIO_MULTI_STREAMS` (default, per-participant), `AUDIO_MIXED_STREAM`.

### Desktop Share

```python
params = rtms.DeskshareParams()
params.codec      = rtms.VideoCodec.H264
params.resolution = rtms.VideoResolution.FHD
params.fps        = 5
client.set_deskshare_params(params)
```

Uses the same `codec`, `resolution`, `fps`, and `data_opt` fields as video.

### Transcript Language

By default the SDK auto-detects the spoken language before enabling transcription (~30 seconds). Providing a language hint lets transcription begin immediately:

```python
# Hint the source language — skips auto-detect, transcription starts immediately
params = rtms.TranscriptParams()
params.src_language = rtms.TranscriptLanguage.ENGLISH
client.set_transcript_params(params)
```

`TranscriptLanguage` constants: `ENGLISH`, `SPANISH`, `JAPANESE`, `CHINESE_SIMPLIFIED`, and many more. To use auto-detection, omit `set_transcript_params` or set `src_language = rtms.TranscriptLanguage.NONE`.

## Individual Video Streams

By default you receive a single composite stream of the active speaker. To receive per-participant video, first configure `VIDEO_SINGLE_INDIVIDUAL_STREAM`, then subscribe per participant as they join:

```python
# Must be called before join() — switches from composite to per-participant streams
params = rtms.VideoParams()
params.data_opt = rtms.DataOption.VIDEO_SINGLE_INDIVIDUAL_STREAM
client.set_video_params(params)

# Subscribe when a participant joins, unsubscribe when they leave
@client.on_user_update
def on_user(op, participant):
    if op == rtms.USER_JOIN and participant.id:
        client.subscribe_video(participant.id, True)
    if op == rtms.USER_LEAVE and participant.id:
        client.subscribe_video(participant.id, False)

# Fires when a participant's video turns on or off
@client.on_participant_video
def on_participant_video(user_ids, is_on):
    print(f'Video {"on" if is_on else "off"} for users: {user_ids}')

# Fires with the subscription result for each subscribe_video() call
@client.on_video_subscribed
def on_video_subscribed(user_id, status, error):
    print(f'subscribe_video({user_id}): status={status}' + (f' error={error}' if error else ''))

@client.on_video_data
def on_video(data, size, timestamp, metadata):
    print(f'Video: {size}B from {metadata.userName}')
```

## asyncio Integration

`run_async()` is a drop-in replacement for `run()` that uses `asyncio.sleep()` between polls, so it composes naturally with aiohttp, FastAPI, asyncpg, and any other async framework on a shared event loop:

```python
import asyncio
import rtms
from aiohttp import web

routes = web.RouteTableDef()

@routes.post('/webhook')
async def webhook(request):
    payload = await request.json()
    if 'rtms_started' in payload.get('event', ''):
        client = rtms.Client()
        client.on_transcript_data(
            lambda d, s, t, m: print(m.userName, d.decode())
        )
        client.join(payload['payload'])
    return web.Response(text='ok')

async def main():
    app = web.Application()
    app.add_routes(routes)
    runner = web.AppRunner(app)
    await runner.setup()
    await web.TCPSite(runner, port=8080).start()
    await rtms.run_async()   # yields control between polls — never blocks

asyncio.run(main())
```

Async callbacks are detected automatically and scheduled on the running event loop:

```python
client = rtms.Client()

async def save_audio(data, size, timestamp, metadata):
    await db.insert('audio', data)   # fully non-blocking

client.on_audio_data(save_audio)     # coroutine detected, dispatched via loop
```

## Executor-Based Callback Dispatch

For CPU-bound or I/O-heavy callbacks that should not block the poll loop, pass a `concurrent.futures.Executor`:

```python
from concurrent.futures import ThreadPoolExecutor
import rtms

# Per-client executor
client = rtms.Client(executor=ThreadPoolExecutor(max_workers=8))
client.on_audio_data(run_asr_model)          # dispatched to thread pool
client.on_transcript_data(write_to_database) # dispatched to thread pool

# Global default for all clients in the event loop
rtms.run(executor=ThreadPoolExecutor(max_workers=32))
```

Callbacks without an executor continue to run inline — identical to v1.0 behavior.

## Explicit Event Loops

By default, `rtms.run()` and `rtms.run_async()` manage a single implicit event loop. For more control — multiple threads, custom routing, or independent loop lifecycles — use `EventLoop` and `EventLoopPool` directly.

### EventLoop

An `EventLoop` owns a set of clients and drives their poll cycles on a single thread. Clients added to a loop have their C SDK handle allocated on that loop's thread, satisfying the SDK's thread-affinity requirement.

```python
import rtms

loop = rtms.EventLoop(poll_interval=0.01, name='my-loop')

@rtms.on_webhook_event
def handle(payload):
    if 'rtms_started' not in payload.get('event', ''):
        return
    client = rtms.Client()

    @client.on_transcript_data
    def _(data, size, timestamp, metadata):
        print(f'{metadata.userName}: {data.decode()}')

    loop.add(client)           # assigns client to this loop's thread
    client.join(payload['payload'])

loop.run()                     # blocks current thread
```

`EventLoop` methods:

| Method | Description |
|---|---|
| `add(client)` | Assign a client to this loop. Must be called before `join()`. |
| `run(stop_on_empty=False)` | Block the current thread, polling all clients. |
| `run_async(stop_on_empty=False)` | Async equivalent — `await` between poll cycles. |
| `start()` | Run in a background daemon thread. Returns `self` for chaining. |
| `stop()` | Signal the loop to stop after the current poll cycle. |
| `join(timeout=None)` | Wait for the background thread to finish (after `start()`). |
| `client_count` | Property — number of clients on this loop. |

### EventLoopPool

An `EventLoopPool` manages N `EventLoop` threads and routes each new client to a loop automatically:

```python
from concurrent.futures import ThreadPoolExecutor
import rtms

pool = rtms.EventLoopPool(threads=4, strategy='least_loaded')
executor = ThreadPoolExecutor(max_workers=32)

@rtms.on_webhook_event
def handle(payload):
    if 'rtms_started' not in payload.get('event', ''):
        return
    client = rtms.Client(executor=executor)

    @client.on_audio_data
    def _(data, size, timestamp, metadata):
        process_audio(data)

    pool.add(client)           # routed to the loop with fewest clients
    client.join(payload['payload'])

pool.run()                     # blocks; N-1 loops run as daemon threads
```

Constructor parameters:

| Parameter | Default | Description |
|---|---|---|
| `threads` | 4 | Number of SDK I/O threads |
| `poll_interval` | 0.01 | Seconds between poll cycles per loop |
| `strategy` | `'least_loaded'` | `'least_loaded'` or `'round_robin'` |

`EventLoopPool` methods:

| Method | Description |
|---|---|
| `add(client)` | Route a client to a loop per the strategy. Returns the assigned `EventLoop`. |
| `run(stop_on_empty=False)` | Start N-1 loops as daemon threads, run the last on the current thread. |
| `run_async(stop_on_empty=False)` | Run all loops as concurrent asyncio coroutines. |
| `stop()` | Stop all loops. |
| `client_count` | Property — total clients across all loops. |
| `loops` | Property — the underlying `List[EventLoop]`. |

### Stopping the Event Loop

Call `rtms.stop()` to shut down the default event loop from another thread or coroutine:

```python
import signal
import rtms

signal.signal(signal.SIGTERM, lambda *_: rtms.stop())

rtms.run()  # stops cleanly on SIGTERM
```

For explicit `EventLoop` or `EventLoopPool` instances, call their `.stop()` method directly.

## Scaling Strategy

The Python SDK provides a layered set of primitives for scaling from a single meeting to hundreds of concurrent streams.

### Layer 1 — Single process, inline callbacks (default, zero config)

Appropriate for < ~20 concurrent meetings with lightweight callbacks (logging, forwarding, simple storage).

```python
import rtms

@rtms.on_webhook_event
def handle(payload):
    if payload.get('event') != 'meeting.rtms_started':
        return
    client = rtms.Client()
    client.on_transcript_data(lambda d, s, t, m: print(m.userName, d.decode()))
    client.join(payload['payload'])

rtms.run()
```

- **Event loop**: `rtms.run()` — synchronous, single-threaded
- **Callbacks**: inline, no dispatch overhead
- **GIL**: released during `poll()`, so other Python threads (e.g. the webhook HTTP server) stay responsive

### Layer 2 — Single process, executor dispatch

Appropriate for CPU-bound callbacks (ML inference, audio processing) or I/O callbacks (database writes, S3 uploads) that would otherwise block the poll loop.

```python
from concurrent.futures import ThreadPoolExecutor
import rtms

executor = ThreadPoolExecutor(max_workers=32)

@rtms.on_webhook_event
def handle(payload):
    if payload.get('event') != 'meeting.rtms_started':
        return
    client = rtms.Client(executor=executor)
    client.on_audio_data(run_asr_model)          # dispatched to thread pool
    client.on_transcript_data(write_to_database) # dispatched to thread pool
    client.join(payload['payload'])

rtms.run()
```

- **Event loop**: `rtms.run()` — poll loop stays fast; callbacks run in the pool
- **Thread pool size**: tune `max_workers` to match your callback latency profile
- **Back-pressure**: executor queue depth limits unbounded growth

### Layer 3 — asyncio native

Appropriate when the rest of your stack is already async (aiohttp, FastAPI, asyncpg). Callbacks declared as `async def` are auto-scheduled on the running event loop.

```python
import asyncio
import rtms

async def on_transcript(data, size, timestamp, metadata):
    await db.execute('INSERT INTO transcripts VALUES ($1, $2)', metadata.userId, data)

@rtms.on_webhook_event
def handle(payload):
    if payload.get('event') != 'meeting.rtms_started':
        return
    client = rtms.Client()
    client.on_transcript_data(on_transcript)   # coroutine auto-detected
    client.join(payload['payload'])

async def main():
    await asyncio.gather(
        rtms.run_async(),
        start_web_server(),
    )

asyncio.run(main())
```

- **Event loop**: `rtms.run_async()` — `await asyncio.sleep()` between polls, never blocks
- **Coroutine dispatch**: `asyncio.run_coroutine_threadsafe` bridges SDK callbacks to the loop
- **Composable**: runs alongside any other async service on the same loop

### Layer 4 — Multi-thread (EventLoopPool)

Appropriate for 25–100+ concurrent meetings in a single process. Distributes clients across N SDK I/O threads so no single thread is overwhelmed.

```python
from concurrent.futures import ThreadPoolExecutor
import rtms

pool = rtms.EventLoopPool(threads=4)
executor = ThreadPoolExecutor(max_workers=32)

@rtms.on_webhook_event
def handle(payload):
    if 'rtms_started' not in payload.get('event', ''):
        return
    client = rtms.Client(executor=executor)
    client.on_transcript_data(write_to_database)
    pool.add(client)           # routed to least-loaded loop
    client.join(payload['payload'])

pool.run()
```

- **Thread count**: 1 thread per ~25 clients is a reasonable starting point
- **Routing**: `'least_loaded'` (default) or `'round_robin'`
- **Monitoring**: `pool.client_count` and `loop.client_count` for load visibility
- **Composable**: works with executor dispatch and `run_async()`

### Layer 5 — Multi-process

Appropriate when a single process can't keep up. Each process runs its own event loop (or pool); a load balancer routes webhook events to available workers.

```
Zoom → Load Balancer (nginx/HAProxy)
         ├── Worker 0 (pool.run(), meetings 0..N)
         ├── Worker 1 (pool.run(), meetings N..2N)
         └── Worker 2 (pool.run(), meetings 2N..3N)
```

Use a message queue (Redis pub/sub, RabbitMQ, Kafka) to distribute join requests:

```python
# worker.py — one process per worker
import rtms, redis, json

r = redis.Redis()
sub = r.pubsub()
sub.subscribe('rtms:join')

@rtms.on_webhook_event
def handle(payload):
    # Publish to queue; a free worker picks it up
    r.publish('rtms:join', json.dumps(payload))

rtms.run()
```

### Choosing the Right Layer

| Workload | Recommended Layer |
|---|---|
| Prototyping / light callbacks | Layer 1 — inline |
| CPU-bound (ASR, video) | Layer 2 — executor |
| Async framework (FastAPI, aiohttp) | Layer 3 — run_async |
| 25–100 concurrent meetings | Layer 4 — EventLoopPool |
| Beyond single-process capacity | Layer 5 — multi-process |

All layers use the same `Client` API. You can mix layers — e.g. Layer 3 for the web server, Layer 2 executors for heavy callbacks, and Layer 4 for multi-threaded polling — without any API changes.

## Environment Setup

```bash
python3 -m venv .venv
source .venv/bin/activate      # Windows: .venv\Scripts\activate
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
ZM_RTMS_LOG_LEVEL=info          # error, warn, info, debug, trace
ZM_RTMS_LOG_FORMAT=progressive  # progressive or json
ZM_RTMS_LOG_ENABLED=true
```

## Related

- [Node.js API Reference](node.md)
- [Full API Reference](https://zoom.github.io/rtms/py/)
