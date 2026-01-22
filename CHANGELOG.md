# Changelog

All notable changes to the Zoom RTMS SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
