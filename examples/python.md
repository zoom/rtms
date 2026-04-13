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

## Scaling Strategy

The Python SDK provides a layered set of primitives for scaling from a single meeting to hundreds of concurrent streams.

### Layer 1 — Inline callbacks (default)

Appropriate for < ~20 concurrent streams with lightweight callbacks.

```python
import rtms

@client.on_webhook_event
def handle(payload):
    if 'rtms_started' not in payload.get('event', ''):
        return
    client = rtms.Client()
    client.on_transcript_data(lambda d, s, t, m: print(m.userName, d.decode()))
    client.join(payload['payload'])

rtms.run()
```

GIL is released during `poll()`, so the webhook HTTP server thread stays responsive.

### Layer 2 — Executor dispatch

Appropriate for CPU-bound callbacks (ML inference, audio processing) or I/O callbacks that would block the poll loop.

```python
from concurrent.futures import ThreadPoolExecutor
import rtms

executor = ThreadPoolExecutor(max_workers=32)

@rtms.on_webhook_event
def handle(payload):
    if 'rtms_started' not in payload.get('event', ''):
        return
    client = rtms.Client(executor=executor)
    client.on_audio_data(run_asr_model)
    client.on_transcript_data(write_to_database)
    client.join(payload['payload'])

rtms.run()
```

### Layer 3 — asyncio native

Appropriate when the rest of your stack is already async (aiohttp, FastAPI, asyncpg).

```python
import asyncio, rtms

async def on_transcript(data, size, timestamp, metadata):
    await db.execute('INSERT INTO transcripts VALUES ($1, $2)', metadata.userId, data)

@rtms.on_webhook_event
def handle(payload):
    if 'rtms_started' not in payload.get('event', ''):
        return
    client = rtms.Client()
    client.on_transcript_data(on_transcript)   # coroutine auto-detected
    client.join(payload['payload'])

async def main():
    await asyncio.gather(rtms.run_async(), start_web_server())

asyncio.run(main())
```

### Layer 4 — Multi-process

Appropriate for 50+ concurrent streams. Each process runs its own `rtms.run()` loop; a load balancer routes webhook events to available workers.

```
Zoom → Load Balancer (nginx/HAProxy)
         ├── Worker 0 (rtms.run(), streams 0..N)
         ├── Worker 1 (rtms.run(), streams N..2N)
         └── Worker 2 (rtms.run(), streams 2N..3N)
```

Use a message queue (Redis, RabbitMQ, Kafka) to distribute join requests:

```python
import rtms, redis, json

r = redis.Redis()

@rtms.on_webhook_event
def handle(payload):
    r.publish('rtms:join', json.dumps(payload))

rtms.run()
```

### Choosing the Right Layer

| Workload | Recommended layer |
|---|---|
| Prototyping / light callbacks | Layer 1 — inline |
| CPU-bound (ASR, video processing) | Layer 2 — executor |
| Async framework (FastAPI, aiohttp) | Layer 3 — run_async |
| 50+ concurrent streams | Layer 4 — multi-process |

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
