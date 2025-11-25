# Contributing to Zoom RTMS SDK

Thank you for your interest in contributing to the Zoom RTMS SDK! This document provides guidelines and instructions for contributing to this project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Development Setup](#development-setup)
  - [Prerequisites](#prerequisites)
  - [System Requirements](#system-requirements)
  - [Getting Started](#getting-started)
- [Build and Test](#build-and-test)
- [Pull Request Process](#pull-request-process)
- [Coding Standards](#coding-standards)
- [Documentation](#documentation)
- [Issue Reporting](#issue-reporting)
- [License](#license)

## Code of Conduct

This project adheres to the Zoom Open Source Code of Conduct. By participating, you are expected to uphold this code.

## Development Setup

### Prerequisites

#### Required Tools

| Tool | Minimum Version | Recommended Version | Notes |
|------|----------------|-------------------|--------|
| **Node.js** | 20.3.0 | 24.x LTS | N-API v9 required |
| **Python** | 3.10 | 3.13 | Type hints, performance |
| **CMake** | 3.25 | 3.28+ | Modern CMake features |
| **Task** | 3.0 | 3.x latest | Build orchestration |
| **Docker** | 20.0 | 28.5+ | For Linux builds (optional) |

#### Platform-Specific Build Tools

- **Linux**: GCC 9+ and make
- **macOS**: Xcode Command Line Tools

#### Installing Task

Task (go-task) is the build orchestration tool used by this project.

**macOS:**
```bash
brew install go-task
```

**Linux:**
```bash
sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d -b ~/.local/bin
```

**Other platforms:** See https://taskfile.dev/installation/

### System Requirements

Before you start, verify your system meets all requirements:

```bash
task doctor
```

This command checks:
- ✓ Node.js version >= 20.3.0
- ✓ Python version >= 3.10
- ✓ CMake version >= 3.25
- ✓ Docker version >= 20.0 (if installed)
- ✓ Zoom RTMS SDK files present

**Example output:**
```
✓ node 24.11.1 (recommended)
✓ python 3.13.0 (recommended)
⚠ cmake 3.25.1 (works, but 3.28+ recommended)
✓ docker 28.5.2 (recommended)
✓ Zoom RTMS SDK found (darwin-arm64)

✓ All requirements met!
```

### Getting Started

1. **Fork the repository** on GitHub

2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR-USERNAME/rtms.git
   cd rtms
   ```

3. **Set up the upstream remote**:
   ```bash
   git remote add upstream https://github.com/zoom/rtms.git
   ```

4. **Verify your environment**:
   ```bash
   task doctor
   ```

5. **Setup the project** (fetches SDK if needed, installs dependencies):
   ```bash
   task setup
   ```

6. **Copy the environment template**:
   ```bash
   cp .env.example .env
   ```
   Then edit the `.env` file with your Zoom developer credentials.

## Build and Test

The project uses [Task](https://taskfile.dev/) for build orchestration. Task provides smart caching, parallel execution, and environment validation.

### Quick Reference

```bash
# See all available commands
task --list

# Verify environment
task doctor                   # Check versions and SDK presence

# Building
task build:js                 # Build Node.js (local platform)
task build:py                 # Build Python (local platform)
task build:local              # Build all bindings (local platform)
task build:linux              # Build all bindings for Linux (via Docker)

# Testing
task test:js                  # Run Node.js tests
task test:py                  # Run Python tests
task test:local               # Run all tests locally
task test:linux               # Run all tests in Linux Docker

# Manual/Interactive testing
task manual:js                # Interactive Node.js test
task manual:py                # Interactive Python test

# Documentation
task docs:js                  # Generate Node.js docs
task docs:py                  # Generate Python docs
task docs:all                 # Generate all docs

# Cleanup
task clean                    # Remove all build artifacts
task clean:build              # Remove only build outputs
```

### Build Modes

Task supports debug and release builds via the `BUILD_TYPE` variable:

```bash
# Release mode (default, optimized)
task build:js

# Debug mode (symbols, no optimization)
BUILD_TYPE=Debug task build:js
```

### Development Workflow

**Typical workflow for contributing:**

```bash
# 1. Verify environment
task doctor

# 2. Create a feature branch
git checkout -b feature/my-new-feature

# 3. Make changes to code
# ... edit files ...

# 4. Build and test locally
task build:js
task test:js

# 5. Test on Linux (if you changed platform-specific code)
task build:linux
task test:linux

# 6. Generate documentation (if you changed APIs)
task docs:all

# 7. Commit and push
git add .
git commit -m "feat: Add my new feature"
git push origin feature/my-new-feature
```

### Before Submitting a PR

Ensure that:

1. **Environment is valid**: `task doctor` passes
2. **All tests pass**: `task test:local` succeeds
3. **Linux builds work**: `task build:linux` succeeds (if applicable)
4. **Code is documented**: Update JSDoc/docstrings
5. **Documentation builds**: `task docs:all` succeeds (if you changed APIs)

## Pull Request Process

1. **Create a branch** for your feature or bugfix:
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/your-bugfix-name
   ```

2. **Make your changes** and commit them using meaningful commit messages:
   ```bash
   git commit -m "feat: Add support for new feature"
   ```
   Follow [Conventional Commits](https://www.conventionalcommits.org/) for commit messages.

3. **Push your branch** to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

4. **Open a Pull Request** against the main branch of the upstream repository.

5. **PR Requirements**:
   - Include a clear description of the changes
   - Update documentation as needed (README, API docs)
   - Add or update tests as appropriate
   - Ensure CI passes (all platforms, all tests)
   - Obtain at least one approval from a maintainer

## Coding Standards

### JavaScript/TypeScript

- Follow the project's ESLint configuration
- Use TypeScript for new code when possible
- Maintain consistent error handling patterns
- Add JSDoc comments to all public APIs

### Python

- Follow PEP 8 style guide
- Support Python 3.10+ (minimum version)
- Use type annotations where appropriate (PEP 484)
- Add docstrings to all public functions/classes

### C++

- Follow the project's existing C++ style
- Use C++20 features
- Ensure memory safety and proper resource management
- Maintain ABI compatibility
- Document all public APIs in header files

### Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation changes
- `refactor:` - Code refactoring
- `test:` - Test additions or changes
- `chore:` - Maintenance tasks

Examples:
```
feat: Add support for Python 3.13
fix: Resolve segfault in Session destructor
docs: Update API documentation for Client.join()
refactor: Simplify build script architecture
test: Add tests for audio parameter validation
chore: Upgrade cmake-js to v7.3.0
```

## Documentation

### When to Update Documentation

- **README.md**: Add significant features, change build process, update requirements
- **API Documentation**: Add JSDoc (Node.js) or docstrings (Python) for all public APIs
- **CLAUDE.md**: Update if you change project structure or development workflow
- **Examples**: Add examples for complex features

### Generating Documentation

```bash
# Generate all documentation
task docs:all

# Or generate for specific language
task docs:js    # Node.js API docs → docs/js/
task docs:py    # Python API docs → docs/py/
```

### Documentation Standards

- **Node.js**: Use JSDoc format with TypeScript type annotations
- **Python**: Use Google-style docstrings with type hints
- **Examples**: Include working code examples that can be run

## Package Distribution

### Node.js Prebuilds

Node.js native addons are distributed as **prebuilds** stored on **GitHub Releases**.

**How it works:**
- Prebuilds are created for N-API versions **9 and 10** (specified in `package.json` binary.napi_versions)
- Built for platforms: **darwin-arm64** and **linux-x64**
- Each combination results in a separate prebuild (4 total: 2 platforms × 2 N-API versions)
- When users run `npm install @zoom/rtms`, the install script automatically downloads the appropriate prebuild
- Upload process: `task publish:js` loops through each platform/N-API combination and uploads to GitHub Releases

**Commands:**
```bash
task prebuild:js         # Create prebuilds for all platforms
task prebuild:js:linux   # Create prebuilds for Linux only
task prebuild:js:darwin  # Create prebuilds for macOS only
task publish:js          # Upload all prebuilds to GitHub Releases
```

### Python Wheels

Python bindings are distributed as **wheels** stored on **PyPI** (or TestPyPI for development).

**How it works:**
- Wheels are platform-specific binary distributions
- Built for platforms: **darwin-arm64** (macOS) and **linux-x64** (manylinux)
- Linux wheels are repaired with `auditwheel` to convert `linux_x86_64` tags to proper `manylinux` tags (PyPI requirement)
- The `auditwheel` step excludes `librtmsdk.so.0` (already bundled) and auto-detects the correct manylinux version
- Upload process: `task publish:py` or `task publish:py:test` uploads all wheels to PyPI/TestPyPI

**Commands:**
```bash
task build:py            # Build wheel for local platform
task build:py:linux      # Build + repair wheel for Linux (manylinux)
task build:py:darwin     # Build wheel for macOS
task repair:py           # Repair Linux wheels with auditwheel (called automatically)
task publish:py:test     # Upload to TestPyPI (testing)
task publish:py          # Upload to production PyPI
```

**Why manylinux?**
PyPI requires Linux wheels to use `manylinux` tags instead of platform-specific tags like `linux_x86_64`. The `manylinux` standard ensures wheels are compatible across different Linux distributions by limiting which system libraries can be dynamically linked.

## Issue Reporting

### Bug Reports

Use the bug report template and include:

- System information (`task doctor` output)
- Steps to reproduce
- Expected vs actual behavior
- Error messages and stack traces
- Node.js/Python version

### Feature Requests

Use the feature request template and include:

- Use case description
- Proposed API or behavior
- Alternative solutions considered

### Security Issues

**Do not** open public issues for security vulnerabilities. Instead, email security@zoom.us.

## Development Tips

### Smart Caching

Task uses checksum-based caching. If source files haven't changed, builds are skipped:

```bash
$ task build:js
# ... builds ...

$ task build:js
task: Task "build:js" is up to date  # Skipped!

$ touch src/rtms.cpp
$ task build:js
# ... rebuilds (detected change) ...
```

### Parallel Execution

Task runs independent tasks in parallel:

```bash
task test:local  # Runs test:js and test:py in parallel
```

### Verbose Mode

See exactly what commands Task is running:

```bash
task -v build:js
```

### Dry Run

Preview what Task would do without executing:

```bash
task -p build:js
```

### Force Execution

Ignore caching and force rebuild:

```bash
task --force build:js
```

## Troubleshooting

### "Task not found"

Ensure Task is installed and in your PATH:

```bash
# macOS
brew install go-task

# Linux
sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d -b ~/.local/bin
export PATH=$HOME/.local/bin:$PATH
```

### "System does not meet requirements"

Run `task doctor` and follow the installation links for missing tools.

### "RTMS SDK not found"

The Zoom RTMS C SDK is proprietary and not included in the repository. Contact Zoom for access, then run:

```bash
task setup
```

### Build Errors

1. **Clean and rebuild**:
   ```bash
   task clean
   task build:js
   ```

2. **Check for platform issues**:
   ```bash
   # Build in Docker for a clean Linux environment
   task build:linux
   ```

3. **Enable debug mode**:
   ```bash
   BUILD_TYPE=Debug task build:js
   ```

## License

By contributing to this project, you agree that your contributions will be licensed under the project's [MIT License](LICENSE.md).

---

## Quick Command Reference

| Task | Description |
|------|-------------|
| `task doctor` | Verify environment meets requirements |
| `task setup` | Fetch SDK and install dependencies |
| `task build:js` | Build Node.js bindings (local) |
| `task build:py` | Build Python bindings (local) |
| `task test:js` | Run Node.js tests |
| `task test:py` | Run Python tests |
| `task docs:all` | Generate all documentation |
| `task clean` | Remove build artifacts |
| `task --list` | Show all available tasks |

For more details, see [README.md](README.md) or run `task --list`.
