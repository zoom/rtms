# Changelog

All notable changes to the Zoom RTMS SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.3] - 2026-02-17

### Fixed

- **Release crash on ended sessions**: `release()` no longer throws when the meeting has already ended externally; cleanup always completes and `RTMS_SDK_NOT_EXIST` is treated as success ([#93](https://github.com/zoom/rtms/issues/93))
- **Video params ignored after audio params**: Calling `setAudioParams()` before `setVideoParams()` no longer overwrites video configuration with defaults; individual param setters skip the default-filling logic ([#94](https://github.com/zoom/rtms/issues/94))
- **Express middleware compatibility**: `createWebhookHandler` now reads from `req.body` when pre-parsed by middleware (e.g., `express.json()`), preventing hangs when the request stream has already been consumed ([#96](https://github.com/zoom/rtms/issues/96))
- **Webhook schema validation**: Webhook handlers (Node.js and Python) now validate that the `event` field is present in the payload and return 400 Bad Request for invalid payloads ([#95](https://github.com/zoom/rtms/issues/95))

## [1.0.2] - 2026-01-30

### Added

- **Video SDK support**: Added `session_id` parameter to `join()` for Video SDK events (`session.rtms_started`) alongside existing Meeting SDK support (`meeting.rtms_started`) ([#80](https://github.com/zoom/rtms/issues/80))
- **Python `rtms.run()`**: New simplified threading model that handles event loop management, client polling, and graceful shutdown automatically ([#88](https://github.com/zoom/rtms/issues/88))

### Fixed

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
