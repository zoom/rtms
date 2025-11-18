# Version Requirements

This document serves as the single source of truth for version requirements across the RTMS SDK project.

## Node.js

### Minimum Version
- **Required:** `>= 20.3.0`
- **Source:** See `package.json` → `engines.node` field

### Recommended Versions
- **Node.js 22.x LTS** - Latest features and best performance (recommended)
- **Node.js 20.x LTS** - Stable, long-term support until April 2026 (minimum supported)

### N-API Versions
This package requires **N-API version 9 or 10**:
- **N-API v9:** Available in Node.js >= 20.3.0
- **N-API v10:** Available in Node.js >= 22.0.0

**Source:** See `package.json` → `binary.napi_versions` field

### Why Node.js 20.3.0+?
- **N-API v9 requirement:** Node.js < 20.3.0 lacks N-API v9 support, causing segmentation faults
- **Security updates:** Node.js 18.x reached EOL in April 2025 and no longer receives security updates
- **ABI stability:** N-API v9+ provides improved ABI stability for native modules

### Unsupported Versions
| Version | Status | Reason |
|---------|--------|--------|
| 20.0.0 - 20.2.x | ❌ Not Supported | N-API v9 unavailable |
| 18.x and older | ❌ Not Supported | EOL (April 2025) - No security updates |
| 16.x and older | ❌ Not Supported | N-API version too old |

## Python

### Minimum Version
- **Required:** `>= 3.10`
- **Source:** See `pyproject.toml` → `requires-python` field

### Recommended Versions
- **Python 3.13** - Latest stable (recommended)
- **Python 3.12** - Stable, excellent performance
- **Python 3.11** - Stable, good performance
- **Python 3.10** - Minimum supported version (EOL October 2026)

### Supported Versions
The SDK is tested on:
- Python 3.10
- Python 3.11
- Python 3.12
- Python 3.13

### Why Python 3.10+?
- **Modern type annotations:** Improved type hint support for better IDE experience
- **Security updates:** Python 3.9 reached EOL in October 2025
- **Performance improvements:** Python 3.10+ includes significant performance enhancements
- **Consistent support window:** Aligns with Node.js support policy

### Unsupported Versions
| Version | Status | Reason |
|---------|--------|--------|
| 3.9 and older | ❌ Not Supported | EOL (October 2025) - No security updates |
| 3.8 and older | ❌ Not Supported | EOL (October 2024) - No security updates |

## Version Support Policy

### End-of-Life (EOL) Policy
The RTMS SDK drops support for language versions **6 months after their official EOL date**.

**Rationale:**
- Provides transition time for users to upgrade
- Ensures all users have access to security updates
- Aligns with industry best practices
- Reduces maintenance burden for old, unsupported platforms

### Timeline Examples
- **Node.js 18.x:** EOL April 2025 → Dropped from RTMS SDK October 2025 ✅ (Already dropped)
- **Python 3.9:** EOL October 2025 → Dropped from RTMS SDK April 2026 ✅ (Already dropped)
- **Node.js 20.x:** EOL April 2026 → Will be dropped October 2026
- **Python 3.10:** EOL October 2026 → Will be dropped April 2027

## Platform Support

For detailed platform information, see [PLATFORM_SUPPORT.md](PLATFORM_SUPPORT.md).

**Supported platforms:**
- darwin-arm64 (macOS Apple Silicon)
- linux-x64 (Linux 64-bit)

## Checking Your Version

### Node.js
```bash
# Check your Node.js version
node --version

# Should show v20.3.0 or higher
```

### Python
```bash
# Check your Python version
python3 --version

# Should show 3.10.x or higher
```

## Upgrading

### Node.js

**Using nvm (recommended):**
```bash
# Install Node.js 22 LTS (recommended)
nvm install 22
nvm use 22

# Or install minimum version (Node.js 20)
nvm install 20
nvm use 20

# Verify
node --version
```

**Direct download:**
- Visit: https://nodejs.org/
- Download Node.js 20 LTS or 22 LTS

### Python

**Using pyenv (recommended):**
```bash
# Install Python 3.13 (recommended)
pyenv install 3.13
pyenv local 3.13

# Or install Python 3.12
pyenv install 3.12
pyenv local 3.12

# Verify
python3 --version
```

**Using system package manager:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install python3.12

# macOS (Homebrew)
brew install python@3.12
```

**Direct download:**
- Visit: https://www.python.org/downloads/
- Download Python 3.12 or 3.13

## Troubleshooting

### "Segmentation fault" with Node.js

**Symptom:** Immediate crash when loading the module

**Cause:** Using Node.js < 20.3.0

**Solution:**
```bash
# Check version
node --version

# If < 20.3.0, upgrade:
nvm install 22
nvm use 22

# Reinstall package
rm -rf node_modules package-lock.json
npm install
```

### "Module not found" or Import Errors

**Symptom:** Cannot import/require the RTMS module

**Cause:** Using Python < 3.10 or Node.js < 20.3.0

**Solution:** Upgrade to supported version (see "Upgrading" section above)

## See Also

- [PLATFORM_SUPPORT.md](PLATFORM_SUPPORT.md) - Supported platforms and architectures
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Development environment setup
- [PUBLISHING.md](../PUBLISHING.md) - Maintainer guide for publishing releases
- [package.json](../package.json) - Node.js version configuration
- [pyproject.toml](../pyproject.toml) - Python version configuration
