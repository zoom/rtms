# Platform Support

This document details the platforms and architectures supported by the Zoom RTMS SDK.

## Currently Supported Platforms

The RTMS SDK currently supports the following platform and architecture combinations:

| Platform | Architecture | Status | Notes |
|----------|-------------|--------|-------|
| macOS | arm64 (Apple Silicon) | ✅ Fully Supported | M1, M2, M3, M4 chips |
| Linux | x64 (64-bit) | ✅ Fully Supported | manylinux compatible |

### Platform Identifiers

In the codebase and build system, platforms are identified as:
- **darwin-arm64** - macOS on Apple Silicon (ARM64)
- **linux-x64** - Linux on 64-bit x86 architecture

## Architecture Constraints

### Linux
- **Only x64 (64-bit) is supported**
- No arm64 or ia32 support on Linux at this time
- Uses manylinux wheel tags for broad compatibility

### macOS (Darwin)
- **Only arm64 (Apple Silicon) is supported**
- No x64 (Intel) support on macOS at this time
- Requires macOS 11.0 (Big Sur) or later

## Language-Specific Details

### Node.js

**Prebuilds are available for:**
- darwin-arm64 (N-API v9 and v10)
- linux-x64 (N-API v9 and v10)

**Installation:**
```bash
npm install @zoom/rtms
```

The package automatically downloads the correct prebuild for your platform during installation. If no prebuild is available, it will attempt to build from source (requires build tools and SDK files).

### Python

**Wheels are available for:**
- darwin-arm64 (Python 3.10-3.13)
- linux-x64 (Python 3.10-3.13)

**Installation:**
```bash
# From PyPI (when available)
pip install rtms

# From TestPyPI (current)
pip install -i https://test.pypi.org/simple/ rtms
```

The package automatically selects the correct wheel for your platform and Python version.

## Planned Future Support

We are actively working to expand platform support. Planned additions include:

### High Priority
- **Windows (win32-x64)** - Windows 10/11 on 64-bit Intel/AMD
- **macOS (darwin-x64)** - macOS on Intel processors (for legacy support)

### Under Consideration
- **Linux (linux-arm64)** - Linux on ARM64 (e.g., AWS Graviton, Raspberry Pi)
- **Windows (win32-arm64)** - Windows on ARM64

**Timeline:** To be determined. Check the [GitHub Issues](https://github.com/zoom/rtms/issues) for updates.

## Platform Detection

The build system automatically detects your platform and architecture:

```javascript
// Detected as: darwin-arm64, linux-x64, etc.
const platform = `${os.platform()}-${os.arch()}`;
```

## Requirements by Platform

### macOS (darwin-arm64)

**System Requirements:**
- Apple Silicon Mac (M1 or later)
- macOS 11.0 (Big Sur) or later
- Xcode Command Line Tools

**Install Command Line Tools:**
```bash
xcode-select --install
```

**Verify Platform:**
```bash
uname -sm
# Should output: Darwin arm64
```

### Linux (linux-x64)

**System Requirements:**
- 64-bit x86 processor (Intel or AMD)
- glibc 2.17+ (manylinux2014 compatible)
- Common distributions: Ubuntu 20.04+, Debian 10+, CentOS 7+, RHEL 7+, Fedora, etc.

**Install Build Tools (if building from source):**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake

# RHEL/CentOS/Fedora
sudo yum install gcc gcc-c++ make cmake

# Or use dnf on newer systems
sudo dnf install gcc gcc-c++ make cmake
```

**Verify Platform:**
```bash
uname -sm
# Should output: Linux x86_64
```

## SDK Library Files

The Zoom RTMS C SDK library files must be placed in the appropriate platform directory:

```
lib/
├── darwin-arm64/
│   ├── librtms_sdk.dylib
│   ├── libcrypto.dylib
│   ├── libssl.dylib
│   └── ... (other dependencies)
├── linux-x64/
│   ├── librtms_sdk.so.0
│   ├── libcrypto.so
│   ├── libssl.so
│   └── ... (other dependencies)
└── include/
    ├── rtms_csdk.h
    ├── rtms_common.h
    └── rtms_sdk.h
```

**Note:** These SDK files are proprietary and must be obtained from Zoom. They are not included in the repository.

## Cross-Platform Building

### Building Linux Packages from macOS

Use Docker to build Linux packages while developing on macOS:

```bash
# Build Node.js prebuild for Linux
docker compose up js

# Build Python wheel for Linux
docker compose up py

# Test on Linux
docker compose up test-js
docker compose up test-py
```

This creates distributable packages (`.tar.gz` prebuilds for Node.js, `.whl` wheels for Python) that work on linux-x64 systems.

### Building macOS Packages from Linux

Building macOS packages from Linux is not currently supported. You must build macOS packages on a macOS system.

## Unsupported Platforms

The following platforms are **not supported** and will not work:

- Windows (any architecture)
- macOS Intel (darwin-x64)
- Linux ARM64 (linux-arm64)
- Linux 32-bit (linux-ia32)
- Any other platform not explicitly listed in "Currently Supported Platforms"

## Troubleshooting

### "Platform not supported" Error

**Symptom:** Installation fails with platform error

**Cause:** Attempting to install on an unsupported platform

**Solution:**
1. Verify your platform:
   ```bash
   # macOS
   uname -sm  # Should show: Darwin arm64

   # Linux
   uname -sm  # Should show: Linux x86_64
   ```

2. If you're on an unsupported platform:
   - Check [Planned Future Support](#planned-future-support) section
   - Watch the [GitHub repository](https://github.com/zoom/rtms) for updates
   - Consider contributing support for your platform

### Missing Prebuild/Wheel

**Symptom:** Package tries to build from source during installation

**Cause:** No prebuild/wheel available for your exact platform/version combination

**Node.js Solution:**
```bash
# Check if GitHub releases have prebuilds
# Visit: https://github.com/zoom/rtms/releases

# Build from source (requires SDK files and build tools)
npm install @zoom/rtms
```

**Python Solution:**
```bash
# Check if wheel exists for your Python version
pip download rtms --no-deps

# If not available, you may need to build from source
# (requires SDK files and build tools)
```

### Architecture Mismatch

**Symptom:** Module loads but crashes or shows wrong architecture

**Cause:** Running on different architecture than the build

**macOS Solution:**
```bash
# Check if running under Rosetta (Intel emulation)
sysctl sysctl.proc_translated
# If returns 1, you're running under Rosetta

# Run natively on Apple Silicon:
arch -arm64 /bin/bash
node --version
```

**Linux Solution:**
```bash
# Verify you're on x86_64
uname -m
# Should output: x86_64

# If on ARM64 (aarch64), RTMS SDK is not currently supported
```

## Configuration Files

Platform support is configured in:

- **Node.js:** `package.json`
  ```json
  {
    "os": ["linux", "darwin"],
    "cpu": ["x64", "arm64"]
  }
  ```

- **Python:** `pyproject.toml` and build system
  - Wheels are built per-platform with appropriate tags
  - Tags: `macosx_11_0_arm64`, `manylinux_2_17_x86_64`

- **Build System:** `scripts/common/build.js`
  - Platform detection and validation
  - Architecture constraints enforced

## See Also

- [VERSION_REQUIREMENTS.md](VERSION_REQUIREMENTS.md) - Language version requirements
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Development environment setup
- [PUBLISHING.md](../PUBLISHING.md) - Platform-specific build and publishing workflows
- [compose.yaml](../compose.yaml) - Docker configuration for cross-platform builds
- [GitHub Releases](https://github.com/zoom/rtms/releases) - Available prebuilds and wheels
