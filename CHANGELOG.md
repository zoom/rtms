# Changelog

All notable changes to the Zoom RTMS SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-04-15

### Added

#### ZCC (Zoom Contact Center) Support
- **`engagement_id` in `join()`**: ZCC voice engagements identify sessions with an `engagement_id` instead of `meeting_uuid`; `join()` now accepts `engagement_id` as an alternative identifier (all bindings)
- **ZCC event constants**: `EVENT_CONSUMER_ANSWERED`, `EVENT_CONSUMER_END`, `EVENT_USER_ANSWERED`, `EVENT_USER_END`, `EVENT_USER_HOLD`, `EVENT_USER_UNHOLD` exported as flat module-level constants in both Node.js and Python

#### Transcript Configuration
- **`TranscriptParams`**: New parameter class for configuring transcript streams (language hint, content type, language detection)
- **`TranscriptLanguage` constants**: Full set of language constants (`ENGLISH`, `SPANISH`, `JAPANESE`, `CHINESE_SIMPLIFIED`, and many more) for skipping auto-detection (~30 s delay) by hinting the source language
- **`setTranscriptParams()`/`set_transcript_params()`**: New method on Client to apply transcript configuration before joining (Node.js and Python)

#### HTTP Proxy Support
- **`setProxy()`/`set_proxy()`**: New method on Client for configuring HTTP and HTTPS proxies for RTMS connections (Node.js and Python)

#### Individual Video Streams
- **`subscribeVideo()`/`subscribe_video()`**: Subscribe or unsubscribe per-participant video when using `VIDEO_SINGLE_INDIVIDUAL_STREAM` mode
- **`onParticipantVideo()`/`on_participant_video()`**: Callback fired when a participant's video turns on or off
- **`onVideoSubscribed()`/`on_video_subscribed()`**: Callback fired with the result of each `subscribeVideo()` call
- **`EVENT_PARTICIPANT_VIDEO_ON` / `EVENT_PARTICIPANT_VIDEO_OFF`**: Exported as flat module-level constants in both Node.js and Python

#### Python — Concurrency & Scaling
- **`EventLoop` / `EventLoopPool`**: New primitives for fine-grained control over polling threads; `EventLoop` wraps a dedicated background thread, `EventLoopPool` balances clients across a pool of threads
- **`run_async()`**: Drop-in async replacement for `run()` using `asyncio.sleep()` between polls; composes naturally with aiohttp, FastAPI, asyncpg
- **Executor dispatch**: `Client(executor=...)` and `run(executor=...)` / `run_async(executor=...)` dispatch data callbacks to a `concurrent.futures.Executor`, keeping the poll loop fast for CPU-bound or I/O-heavy callbacks
- **Async callback detection**: Coroutine callbacks are detected automatically and scheduled on the running event loop via `asyncio.run_coroutine_threadsafe`
- **GIL release in `poll()`**: The C++ poll call now releases the GIL, allowing other Python threads (e.g. the webhook HTTP server) to run freely during polling
- **Context manager support**: `with rtms.Client() as client:` guarantees `leave()` is called on exit
- **Lazy `alloc()` / thread affinity**: `Client()` construction is safe from any thread; the C SDK handle is allocated lazily on the EventLoop's own thread to satisfy the C SDK's thread-affinity constraint
- **Snake_case aliases**: All `camelCase` methods have `snake_case` aliases (`on_audio_data`, `set_audio_params`, `subscribe_event`, etc.)

#### Metadata & AI Interpreter Fields
- **Full metadata fields**: `Metadata` now exposes `userId`, `userName`, `startTs`, `endTs` in both bindings
- **`AiInterpreter` fields**: `Metadata` now exposes `aiInterpreter` with language pair targets from the AI language interpreter SDK feature

#### Constants & Enums
- **`IntEnum` for Python codec/rate/option types**: `AudioCodec`, `VideoCodec`, `AudioSampleRate`, `AudioChannel`, `DataOption` are now proper Python `IntEnum` values (comparisons with integers still work)
- **`DataOption`**: Unified stream delivery mode enum covering both audio (`AUDIO_MIXED_STREAM`, `AUDIO_MULTI_STREAMS`) and video (`VIDEO_SINGLE_ACTIVE_STREAM`, `VIDEO_SINGLE_INDIVIDUAL_STREAM`, `VIDEO_MIXED_GALLERY_VIEW`) options — mirrors the C++ `MEDIA_DATA_OPTION` enum directly; `AudioDataOption` and `VideoDataOption` kept as backward-compat aliases
- **Bidirectional param aliases**: All four param structs (`AudioParams`, `VideoParams`, `DeskshareParams`, `TranscriptParams`) now accept both `snake_case` and `camelCase` field names so existing code continues to work regardless of convention

### Fixed

- **Spurious configure warnings on leave**: `updateMediaConfiguration` was called during callback teardown after the session was already closed, printing 4 `"Failed to update media configuration"` warnings on every clean leave; suppressed by tracking `sdk_opened_` state and calling `markClosed()` before stopping callbacks

### Changed

- **Protocol enums moved to `rtms.h`**: `EVENT_TYPE`, `ZCC_VOICE_EVENT_TYPE`, `SESSION_STATE`, `STREAM_STATE`, `MESSAGE_TYPE`, `STOP_REASON`, and `TRANSCRIPT_LANGUAGE` are now defined in the shared C++ header, making them automatically available to both Node.js and Python bindings

### Documentation

- **Node.js API reference**: New "Media Configuration" section covering `setVideoParams`, `setAudioParams`, `setDeskshareParams`, `setTranscriptParams` with codec/resolution/option constants; updated "Individual Video Streams" section
- **Python API reference**: New "Media Callbacks", "Media Configuration", and "Individual Video Streams" sections; asyncio integration and executor dispatch examples; scaling strategy guide (4-layer progression from single-process to multi-process)
- **ZCC product guide**: Added Zoom Contact Center as a supported product with webhook event names and `engagement_id` usage

### Testing

- **C++ unit tests** (70 tests): Full mock-SDK test suite covering Client lifecycle, session/user events, media callbacks, proxy, transcript params, individual video subscriptions, metadata construction, and `AiInterpreter` bounds guarding
- **Python tests** (122 tests): Coverage for asyncio, executor dispatch, context manager, GIL release, ZCC `engagement_id` routing, individual video methods, transcript params, and all new constants
- **Node.js wrapper tests** (98 tests): Integration tests against the real built ESM module covering all Client methods, callbacks, event subscriptions, constants, and utility functions
- **Test reorganisation**: Tests moved from `tests/` root into `tests/ts/`, `tests/py/`, `tests/cpp/` subdirectories for clarity
- **CI test path fixes**: CI/CD workflows updated to reference new test locations; C++ unit tests added as dedicated `test-cpp-linux` and `test-cpp-macos` jobs using the mock SDK (no real Zoom SDK binary required in CI)

## [1.0.3] - 2026-02-17

### Fixed
- **Fixed Webinar support**: Fixed UUID priroity for `webinar_uuid` parameter to `join()` for Zoom Webinar events (`webinar.rtms_started`); priority: `meeting_uuid` > `webinar_uuid` > `session_id`
- **Exposed `onMediaConnectionInterrupted` callback**: The callback for `EVENT_MEDIA_CONNECTION_INTERRUPTED` events was previously not configured for auto-subscription (Node.js and Python)
- **Release crash on ended sessions**: `release()` no longer throws when the meeting has already ended externally; cleanup always completes and `RTMS_SDK_NOT_EXIST` is treated as success ([#93](https://github.com/zoom/rtms/issues/93))
- **Video params ignored after audio params**: Calling `setAudioParams()` before `setVideoParams()` no longer overwrites video configuration with defaults; individual param setters skip the default-filling logic ([#94](https://github.com/zoom/rtms/issues/94))
- **Express middleware compatibility**: `createWebhookHandler` now reads from `req.body` when pre-parsed by middleware (e.g., `express.json()`), preventing hangs when the request stream has already been consumed ([#96](https://github.com/zoom/rtms/issues/96))
- **Webhook schema validation**: Webhook handlers (Node.js and Python) now validate that the `event` field is present in the payload and return 400 Bad Request for invalid payloads ([#95](https://github.com/zoom/rtms/issues/95))
- **Python event callback coexistence**: Refactored to shared event dispatcher so `onParticipantEvent`, `onActiveSpeakerEvent`, and `onSharingEvent` can all be registered simultaneously (previously each overwrote the last)

## [1.0.2] - 2026-01-30

### Fixed
- **Fixed Video SDK support**: Exposed `session_id` parameter to `join()` for Video SDK events (`session.rtms_started`) alongside existing Meeting/Webinar support (`meeting.rtms_started`) ([#80](https://github.com/zoom/rtms/issues/80))
- **Python `rtms.run()`**: Resolved threading model that handles event loop management, client polling, and graceful shutdown automatically ([#88](https://github.com/zoom/rtms/issues/88))
- **Default media params**: Apply sensible defaults (e.g., `AUDIO_MULTI_STREAMS`) when callbacks are registered without explicit media configuration
- **Media param reconfiguration**: Calling `setAudioParams()`/`setVideoParams()` after registering a callback now triggers reconfiguration
- **Codec validation**: Added validation at C++ layer - JPG/PNG codecs require fps ≤ 5, H264 allows fps up to 30
- **Python docs deployment**: Fixed documentation deployment path (renamed `docs/rtms` to `docs/py`)

### Security

- Upgraded cmake-js to v8 to resolve tar dependency vulnerabilities (GHSA-8qq5-rm4j-mr97, GHSA-r6q2-hw4h-h46w, GHSA-34x7-hfp2-rc4v)

### Changed

- Updated Python installation instructions to use production PyPI instead of TestPyPI

## [1.0.0] - 2026-01-22

### Added

#### Release Infrastructure
- Git tag-based publishing workflow (`js-v*` and `py-v*` triggers)
- Trusted Publishing (OIDC) for npm and PyPI - no stored API tokens required
- Manual approval gates using GitHub Environments before publishing
- Multi-platform CI/CD pipeline (Linux x64, macOS ARM64)
- cibuildwheel integration for Python (builds 8 wheels: 4 Python versions × 2 platforms)
- Artifact reuse pattern ("build once, test many") for efficient CI
- Dry-run mode for testing publish workflow without actual publishing
- Comprehensive PUBLISHING.md guide for maintainers

#### Documentation
- Product-specific examples for Zoom Meetings, Webinars, and Video SDK
- Supported Products section in README
- Complete API documentation generation and GitHub Pages deployment

#### Build System
- Platform-specific wheel cleanup before Python builds
- Scoped package prebuild paths (`@zoom/rtms`)
- Automated testing across Node.js 20.x/22.x/24.x and Python 3.10-3.13

### Changed

#### Breaking Changes
- **Event System Redesign**: Both Node.js and Python bindings now use typed callbacks
  - Node.js: Event handlers use typed callback signatures
  - Python: Callback registration methods updated for consistency
- **Minimum Node.js version**: Requires Node.js 20.3.0+ (N-API v9 support)
- **Minimum Python version**: Requires Python 3.10+

#### CI/CD
- Replaced manual tests with automated pytest and Jest test suites
- Docker Compose services now run actual tests instead of manual mode
- SDK downloads use GITHUB_TOKEN to avoid rate limits

### Fixed

- Correct RPATH for Linux shared library loading in Python wheels
- README filename case sensitivity (README.md instead of README.MD)
- Platform-specific wheel cleanup prevents stale artifacts
- Python interpreter detection using `python -m pip`
- Docker containers call `task setup` before build

## [0.0.x] - Pre-release

Pre-release versions were used for development and testing. The 1.0.0 release marks the first production-ready release with:
- Stable API for both Node.js and Python
- Full CI/CD automation
- Production-ready publishing infrastructure
