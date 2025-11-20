# Publishing Guide for Maintainers

This guide covers the complete workflow for building, testing, and publishing the RTMS SDK packages for Node.js and Python.

## Overview

The RTMS SDK supports multiple languages with **independent versioning**:
- **Node.js**: Published to npm as `@zoom/rtms` (current version: see package.json)
- **Python**: Published to PyPI as `rtms` (current version: see pyproject.toml)
- **Go**: Planned (will have independent versioning)

### Build System

The project uses **[Task (go-task)](https://taskfile.dev/)** for build orchestration. Task provides:
- **Smart caching**: Skips unchanged builds (checksum-based)
- **Parallel execution**: Runs independent tasks concurrently
- **Environment validation**: Checks tool versions before building
- **Cross-platform**: Works on macOS, Linux, Windows

**Command structure:**
```
task <action>:<lang>:<platform>
```

**Examples:**
- `task build:js` - Build Node.js for local platform
- `task build:py:linux` - Build Python wheel for Linux
- `task publish:js` - Upload Node.js prebuilds to GitHub
- `task doctor` - Verify environment meets requirements

**Supported Platforms:**
- `darwin-arm64` (macOS Apple Silicon)
- `linux-x64` (Linux 64-bit)

## Prerequisites

### Required Tools

**Verify your environment with `task doctor` before starting!**

- **Task** (go-task) 3.0+
  - Build orchestration tool
  - Install: `brew install go-task` (macOS) or see https://taskfile.dev/installation/
  - Check version: `task --version`
- **Node.js** >= 20.3.0 (Node.js 24 LTS recommended)
  - Minimum: 20.3.0 (for N-API v9 support, EOL April 2026)
  - Recommended: 24.x LTS (current LTS, Krypton)
  - Also supported: 22.x LTS (previous LTS, Jod)
  - Note: Node.js 18.x is EOL (April 2025) and no longer supported
  - Check version: `node --version`
- **Python** 3.10+ with pip
  - Minimum: 3.10 (EOL Oct 2026)
  - Recommended: 3.12 or 3.13 (latest stable versions)
  - Check version: `python3 --version`
- **CMake** 3.25+
  - Check version: `cmake --version`
- **C/C++ build tools** (GCC 9+ or Clang)
- **Docker** and Docker Compose (recommended for consistent builds)
  - Check version: `docker --version`
- **Zoom RTMS C SDK files** (proprietary, contact Zoom for access)

### Python Build Tools

```bash
pip install build twine wheel
```

### Access to Zoom RTMS C SDK

The SDK library files must be placed in:
- `lib/darwin-arm64/` - macOS libraries
- `lib/linux-x64/` - Linux libraries
- `lib/include/` - Header files

**Note:** These files are .gitignored and must be manually obtained from Zoom.

## Authentication & Credentials

### Node.js Publishing

**npm Registry:**
```bash
export NPM_TOKEN="your-npm-token"
# Or use: npm login
```

**GitHub Releases** (for prebuilds):
```bash
export GITHUB_TOKEN="your-github-personal-access-token"
```

### Python Publishing

**PyPI & TestPyPI:**
```bash
# Use API tokens (recommended)
export TWINE_USERNAME="__token__"
export TWINE_PASSWORD="pypi-your-api-token-here"

# Or use username/password
export TWINE_USERNAME="your-pypi-username"
export TWINE_PASSWORD="your-pypi-password"
```

**How to get PyPI tokens:**
1. Create account on [PyPI](https://pypi.org/) and [TestPyPI](https://test.pypi.org/)
2. Go to Account Settings → API tokens
3. Create token with "Upload packages" scope
4. Save token securely (it's only shown once)

## GitHub Actions CI/CD Workflow

The repository uses GitHub Actions for automated testing and documentation deployment.

### Workflow Triggers

The main workflow (`.github/workflows/main.yml`) runs on:
- **Push** to `main` or `dev` branches
- **Pull Requests** targeting `main` or `dev` branches

### Workflow Jobs

#### 1. Test Node.js SDK (`test-nodejs`)

- **Runs on:** ubuntu-latest
- **Purpose:** Validate Node.js bindings
- **Steps:**
  1. Checkout code
  2. Setup Docker Compose
  3. Run tests: `docker compose run --rm test-js`
- **Runs on:** Every push and PR

#### 2. Test Python SDK (`test-python`)

- **Runs on:** ubuntu-latest with Python 3.10-3.13 matrix
- **Purpose:** Validate Python bindings
- **Steps:**
  1. Checkout code
  2. Setup Python environment
  3. Install dependencies: `pip install pytest python-dotenv`
  4. Run tests: `pytest tests/test_rtms.py -v`
- **Runs on:** Every push and PR

#### 3. Build Documentation (`test-and-build-docs`)

- **Runs on:** ubuntu-latest
- **Requires:** Both test jobs must pass
- **Only runs on:** `main` or `master` branches
- **Purpose:** Generate API documentation
- **Steps:**
  1. Checkout code
  2. Build JavaScript docs: `docker compose run --rm docs-js`
     - Uses TypeDoc to generate docs from TypeScript definitions
     - Output: `docs/js/`
  3. Build Python docs: `pdoc --html --output-dir docs/py --force src/rtms`
     - Uses pdoc3 to generate docs from Python docstrings
     - Output: `docs/py/`
  4. Upload documentation artifact (retained for 30 days)

#### 4. Deploy Documentation (`deploy-documentation`)

- **Runs on:** ubuntu-latest
- **Requires:** Documentation build must succeed
- **Only runs on:** `main` or `master` branches
- **Purpose:** Publish docs to GitHub Pages
- **Steps:**
  1. Download documentation artifact
  2. Configure GitHub Pages
  3. Upload site to GitHub Pages
  4. Deploy to https://zoom.github.io/rtms/
- **Result:** Documentation available at:
  - JavaScript: https://zoom.github.io/rtms/js/
  - Python: https://zoom.github.io/rtms/py/

### Monitoring Workflow Runs

**View workflow runs:**
1. Go to repository: https://github.com/zoom/rtms
2. Click "Actions" tab
3. See all workflow runs with status indicators
4. Click on any run to see detailed logs

**Workflow status badges** (can add to README):
```markdown
![Tests](https://github.com/zoom/rtms/workflows/Test,%20Build%20&%20Deploy%20Documentation/badge.svg)
```

### What's Automated vs. Manual

**Automated (GitHub Actions):**
- ✅ Running tests on every commit/PR
- ✅ Building documentation on main branch
- ✅ Deploying docs to GitHub Pages
- ✅ Validating code quality

**Manual (Maintainer must do):**
- ❌ Version bumping (package.json, pyproject.toml)
- ❌ Building platform-specific wheels/prebuilds
- ❌ Uploading to npm registry
- ❌ Uploading to PyPI/TestPyPI
- ❌ Creating GitHub releases
- ❌ Writing changelogs
- ❌ Tagging releases

### Local Testing Before Push

To ensure CI will pass, run tests locally:

```bash
# Test Node.js (same as CI)
docker compose run --rm test-js

# Test Python (similar to CI)
pip install pytest python-dotenv
pytest tests/test_rtms.py -v

# Build docs (same as CI)
docker compose run --rm docs-js
pip install pdoc3
pdoc --html --output-dir docs/py --force src/rtms
```

### CI/CD Best Practices

1. **Always test locally** before pushing to main
2. **Monitor Actions tab** after pushing to ensure workflows pass
3. **Check documentation** after deployment to verify it looks correct
4. **Fix failing tests** immediately - don't merge PRs with failing tests
5. **Review workflow logs** if something fails to understand the issue

## Versioning Strategy

### Independent Versioning

Each language maintains its own version number:

- **Node.js**: Version in `package.json` (currently **0.0.4**)
- **Python**: Version in `pyproject.toml` (currently **0.0.1**)
- **Go**: Will have separate version when implemented

This allows for:
- Language-specific hotfixes
- Different release cadences
- Independent feature development

### Version Bump Process

**For Node.js:**
```bash
# Edit package.json, update "version" field
vim package.json

# Commit version change
git add package.json
git commit -m "chore(js): bump version to X.Y.Z"
```

**For Python:**
```bash
# Edit pyproject.toml, update [project] version field
vim pyproject.toml

# Commit version change
git add pyproject.toml
git commit -m "chore(py): bump version to X.Y.Z"
```

**After release, tag the version:**
```bash
# Node.js releases
git tag js-vX.Y.Z
git push origin js-vX.Y.Z

# Python releases
git tag python-vX.Y.Z
git push origin python-vX.Y.Z
```

## Pre-Release Checklist

Before publishing any release, ensure:

### General
- [ ] All tests passing locally
- [ ] All CI tests passing (check Actions tab)
- [ ] Documentation builds successfully
- [ ] Changelog/release notes prepared
- [ ] SDK library files present in lib/ directories
- [ ] Clean build directories: `task clean`

### Node.js Specific
- [ ] Version bumped in `package.json`
- [ ] Tests pass: `task test:js`
- [ ] Prebuilds created for both platforms
- [ ] GitHub token configured: `GITHUB_TOKEN`

### Python Specific
- [ ] Version bumped in `pyproject.toml`
- [ ] Tests pass: `task test:py`
- [ ] Wheels built for both platforms
- [ ] Twine credentials configured: `TWINE_USERNAME`, `TWINE_PASSWORD`
- [ ] Tested install: `import rtms` works

## Node.js Publishing Workflow

### Step 1: Build for Specific Platform

```bash
# On macOS
task build:js:darwin

# On Linux (or using Docker)
task build:js:linux

# Or build for current platform
task build:js
```

### Step 2: Create Prebuilds

Prebuilds are platform-specific binary packages for faster installation. The build process creates multiple prebuilds for **N-API version compatibility**.

**N-API Matrix:**
- **N-API versions:** 9 and 10 (defined in `package.json` binary.napi_versions)
- **Platforms:** darwin-arm64, linux-x64
- **Total prebuilds:** 4 (2 platforms × 2 N-API versions)

```bash
# Create prebuild for specific platform (builds for N-API 9 and 10)
task prebuild:js:darwin   # macOS arm64 (N-API 9 + 10)
task prebuild:js:linux    # Linux x64 (N-API 9 + 10)

# Or create prebuilds for all platforms
task prebuild:js          # All 4 combinations
```

**Output:** Prebuilds are created in `prebuilds/` directory with names like:
- `rtms-v0.0.5-napi-v9-darwin-arm64.tar.gz`
- `rtms-v0.0.5-napi-v10-darwin-arm64.tar.gz`
- `rtms-v0.0.5-napi-v9-linux-x64.tar.gz`
- `rtms-v0.0.5-napi-v10-linux-x64.tar.gz`

### Step 3: Upload Prebuilds to GitHub Releases

Prebuilds must be uploaded to GitHub releases before publishing to npm. The upload process **automatically loops through all N-API versions and platforms**.

```bash
# Set GitHub token
export GITHUB_TOKEN="ghp_your_token_here"

# Upload all prebuilds (all platforms + N-API versions)
task publish:js

# Or upload for specific platform only (but all N-API versions)
task publish:js:darwin    # Uploads N-API 9 + 10 for darwin-arm64
task publish:js:linux     # Uploads N-API 9 + 10 for linux-x64
```

**What this does:**
- Loops through each platform/N-API combination
- Creates a GitHub release (if not exists) tagged with package version
- Uploads all 4 `.tar.gz` prebuild files to the release
- When users run `npm install @zoom/rtms`, the install script downloads the appropriate prebuild for their platform and Node.js version
- Node.js 20.x uses N-API v9, Node.js 22.x and 24.x use N-API v10

### Step 4: Publish to npm

```bash
# Ensure you're logged in
npm whoami

# Or login
npm login

# Publish to npm registry
npm publish

# For scoped packages (if needed)
npm publish --access public
```

**Verify:**
```bash
# Check package is published
npm view @zoom/rtms

# Test installation
npm install @zoom/rtms
```

### Step 5: Verify Documentation

After publishing, verify documentation is updated:
- Visit: https://zoom.github.io/rtms/js/
- Check version number is correct
- Verify API docs are current

## Python Publishing Workflow

### Step 1: Build Wheels

Python wheels are platform-specific binary distributions. Linux wheels require **manylinux tagging** for PyPI compliance.

```bash
# Build for specific platform
task build:py:darwin    # macOS (darwin-arm64)
task build:py:linux     # Linux (linux-x64) - automatically repairs with auditwheel

# Or build for current platform
task build:py
```

**Linux Wheel Repair Process:**
When building for Linux (`task build:py:linux`), the wheel undergoes automatic repair:
1. Initial build creates wheel with `linux_x86_64` tag
2. `auditwheel repair` converts to proper `manylinux_*` tag (e.g., `manylinux_2_17_x86_64`)
3. Excludes `librtmsdk.so.0` (already bundled in wheel) from repair
4. Auto-detects appropriate manylinux version based on system libraries
5. Removes original non-compliant wheel

**Why manylinux?**
PyPI requires Linux wheels to use `manylinux` tags to ensure compatibility across different Linux distributions. The manylinux standard limits which system libraries can be dynamically linked.

**Output:** Wheels are created in `dist/py/` directory
- macOS: `rtms-X.Y.Z-cp310-abi3-macosx_11_0_arm64.whl`
- Linux: `rtms-X.Y.Z-cp310-abi3-manylinux_2_17_x86_64.whl` (after auditwheel repair)

### Step 2: Test on TestPyPI

Always test on TestPyPI before publishing to production.

```bash
# Set TestPyPI credentials
export TWINE_USERNAME="__token__"
export TWINE_PASSWORD="pypi-your-testpypi-token"

# Upload to TestPyPI
task publish:py --platform=darwin     # Upload macOS wheel
task publish:py --platform=linux      # Upload Linux wheel

# Or upload all wheels
task publish:py:test
```

**Test installation from TestPyPI:**
```bash
# Create test environment
python3 -m venv test-env
source test-env/bin/activate

# Install from TestPyPI
pip install -i https://test.pypi.org/simple/ rtms

# Test import
python -c "import rtms; print(rtms.__version__)"

# Run sample code
python tests/test_basic.py

# Cleanup
deactivate
rm -rf test-env
```

### Step 3: Publish to Production PyPI

After confirming TestPyPI works:

```bash
# Set production PyPI credentials
export TWINE_USERNAME="__token__"
export TWINE_PASSWORD="pypi-your-production-token"

# Upload to production PyPI
task publish:py --platform=darwin
task publish:py --platform=linux

# Or upload all wheels
task publish:py
```

**Verify:**
```bash
# Check package is published
pip search rtms

# Test installation
pip install rtms
python -c "import rtms; print(rtms.__version__)"
```

### Step 4: Verify Documentation

After publishing, verify documentation is updated:
- Visit: https://zoom.github.io/rtms/py/
- Check version number is correct
- Verify API docs are current

## Platform-Specific Build Notes

### macOS (darwin-arm64)

**Requirements:**
- Apple Silicon Mac (M1/M2/M3)
- Xcode Command Line Tools
- SDK files in `lib/darwin-arm64/`

**Build:**
```bash
task build:py:darwin
task build:js:darwin
```

**Notes:**
- Native builds work out of the box
- Output has `arm64` architecture
- May need to sign dylibs for distribution

### Linux (linux-x64)

**Requirements:**
- Linux x64 system or Docker
- GCC 9+ or Clang
- SDK files in `lib/linux-x64/`

**Build natively:**
```bash
task build:py:linux
task build:js:linux
```

**Build with Docker (recommended for cross-platform):**
```bash
docker compose up py      # Create Python wheel for linux-x64
docker compose up js      # Create Node.js prebuild for linux-x64
docker compose up test-py # Test Python on Linux
docker compose up test-js # Test Node.js on Linux
```

**Notes:**
- Docker creates **Linux distributable packages** from macOS
- Use these commands on darwin-arm64 to build linux-x64 packages
- Prebuilds go to `prebuilds/` directory
- Python wheels go to `dist/py/` directory
- manylinux tags ensure broad Linux compatibility

## Complete Release Workflow

### For Node.js Release

1. **Pre-release checks:**
   - ✅ Check CI: All tests passing in Actions tab
   - ✅ Check docs: https://zoom.github.io/rtms/js/ is up to date
   - ✅ Test locally: `task test:js`

2. **Update version in package.json:**
   ```json
   {
     "version": "X.Y.Z"
   }
   ```

3. **Update changelog:**
   - Add new version section
   - List new features, fixes, breaking changes

4. **Commit version changes:**
   ```bash
   git add package.json CHANGELOG.md
   git commit -m "chore(js): bump version to X.Y.Z"
   git push origin main
   ```

5. **Clean and build:**
   ```bash
   task clean
   task prebuild:js:darwin  # On macOS
   task prebuild:js:linux   # On Linux or Docker
   ```

6. **Upload prebuilds:**
   ```bash
   export GITHUB_TOKEN="your-token"
   task publish:js:darwin
   task publish:js:linux
   ```

7. **Publish to npm:**
   ```bash
   npm publish
   ```

8. **Create GitHub release:**
   - Go to: https://github.com/zoom/rtms/releases/new
   - Tag: `js-vX.Y.Z`
   - Title: "Node.js vX.Y.Z"
   - Copy changelog content
   - Publish release

9. **Tag and push:**
   ```bash
   git tag js-vX.Y.Z
   git push origin js-vX.Y.Z
   ```

10. **Verify:**
    - ✅ Check npm: `npm view @zoom/rtms`
    - ✅ Check GitHub release exists
    - ✅ Check Actions tab for any issues
    - ✅ Test install: `npm install @zoom/rtms`

### For Python Release

1. **Pre-release checks:**
   - ✅ Check CI: Python tests passing in Actions tab
   - ✅ Check docs: https://zoom.github.io/rtms/py/ is up to date
   - ✅ Test locally: `task test:py`

2. **Update version in pyproject.toml:**
   ```toml
   [project]
   name = "rtms"
   version = "X.Y.Z"
   ```

3. **Update changelog:**
   - Add new version section
   - List new features, fixes, breaking changes

4. **Commit version changes:**
   ```bash
   git add pyproject.toml CHANGELOG.md
   git commit -m "chore(py): bump version to X.Y.Z"
   git push origin main
   ```

5. **Clean and build wheels:**
   ```bash
   task clean
   task build:py:darwin  # On macOS
   task build:py:linux   # On Linux or Docker
   ```

6. **Test on TestPyPI:**
   ```bash
   export TWINE_USERNAME="__token__"
   export TWINE_PASSWORD="your-testpypi-token"

   task publish:py:test

   # Test installation
   pip install -i https://test.pypi.org/simple/ rtms
   python -c "import rtms; print(rtms.__version__)"
   ```

7. **If tests pass, upload to production PyPI:**
   ```bash
   export TWINE_PASSWORD="your-production-pypi-token"
   task publish:py
   ```

8. **Create GitHub release:**
   - Go to: https://github.com/zoom/rtms/releases/new
   - Tag: `python-vX.Y.Z`
   - Title: "Python vX.Y.Z"
   - Copy changelog content
   - Publish release

9. **Tag and push:**
   ```bash
   git tag python-vX.Y.Z
   git push origin python-vX.Y.Z
   ```

10. **Verify:**
    - ✅ Check PyPI: https://pypi.org/project/rtms/
    - ✅ Check GitHub release exists
    - ✅ Check Actions tab for any issues
    - ✅ Test install: `pip install rtms`
    - ✅ Verify docs: https://zoom.github.io/rtms/py/

## Troubleshooting

### CI Tests Failing

**Symptoms:** Tests fail in GitHub Actions but pass locally

**Solutions:**
1. Check Actions tab for error messages
2. Run same Docker command locally:
   ```bash
   docker compose run --rm test-js
   docker compose run --rm test-py
   ```
3. Check if SDK files are properly available
   - They're .gitignored, so not in repository
   - CI may need SDK files added to secrets or artifacts
4. Review logs in workflow run details
5. Check for environment-specific issues (paths, permissions)

### Segmentation Fault with Older Node.js

**Symptoms:** Users report immediate crash/segfault when loading the module

**Error Example:**
```
Core was generated by `/path/to/node`.
Program terminated with signal SIGSEGV, Segmentation fault.
#0 napi_module_register_by_symbol(...)
```

**Root Cause:** User is running Node.js < 20.3.0

**Why this happens:**
- Package uses N-API versions 9 and 10
- N-API v9 requires Node.js >= 20.3.0 (Node.js 18.x is EOL)
- Older versions lack required N-API symbols
- Results in segfault during module registration

**Solutions for Users:**
```bash
# Check Node.js version
node --version

# Upgrade to supported version
nvm install 24         # Recommended: Node.js 24 LTS
nvm use 24

# Or minimum version
nvm install 20
nvm use 20

# Reinstall package
rm -rf node_modules package-lock.json
npm install
```

**Solutions for Maintainers:**
1. **Verify `engines` field in package.json:**
   ```json
   "engines": {
     "node": ">=20.3.0"
   }
   ```

2. **Check prebuild NAPI versions:**
   ```json
   "binary": {
     "napi_versions": [9, 10]
   }
   ```

3. **Build prebuilds with supported Node.js version:**
   ```bash
   # Use Node.js 20.3.0+ for building
   node --version  # Should show 20.3.0+
   task prebuild:js
   ```

4. **Test on minimum version before publishing:**
   ```bash
   nvm use 20.3.0
   npm install
   node -e "require('@zoom/rtms')"  # Should load without crashing
   ```

**Prevention:**
- Always build with Node.js >= 20.3.0
- Test on minimum supported version
- Keep `engines` field accurate
- Update CI to test against minimum version

### Documentation Not Building

**Symptoms:** "Test Application & Build Documentation" job fails

**Solutions:**
1. Check Actions → "Test Application & Build Documentation" job
2. Verify TypeScript types are valid:
   ```bash
   task build:js
   ```
3. Verify Python docstrings are valid:
   ```bash
   task docs:py
   ```
4. Check if typedoc or pdoc3 dependency failed
5. Look for syntax errors in .ts or .py files

### Documentation Not Deploying

**Symptoms:** Docs build but don't appear on GitHub Pages

**Solutions:**
1. Check "Deploy Documentation Site" job in Actions
2. Verify GitHub Pages is enabled:
   - Go to Settings → Pages
   - Source should be "GitHub Actions"
3. Check permissions: workflow needs `pages: write` permission
4. May take 5-10 minutes to propagate to GitHub Pages CDN
5. Check https://zoom.github.io/rtms/ (may need to clear cache)

### Wheel Upload Fails

**Symptoms:** `twine upload` returns error

**Common errors and solutions:**

**"HTTPError: 403 Forbidden"**
- Credentials are wrong
- Check TWINE_USERNAME and TWINE_PASSWORD
- Verify token has "Upload packages" scope

**"File already exists"**
- Version number already published
- Bump version in pyproject.toml
- PyPI doesn't allow re-uploading same version

**"No files found"**
- Wheels weren't built
- Check dist/py/ directory exists
- Run: `task build:py:darwin` or `task build:py:linux`

**"Invalid distribution file"**
- Wheel is corrupted
- Clean and rebuild: `task clean && task build:py`

### GitHub Release Upload Fails

**Symptoms:** `task publish:js:darwin` fails

**Solutions:**
1. Verify GITHUB_TOKEN is set and valid:
   ```bash
   echo $GITHUB_TOKEN
   ```
2. Token needs `repo` permissions
3. Check if release/tag already exists:
   - Go to: https://github.com/zoom/rtms/releases
   - Delete or rename conflicting release
4. Ensure prebuild files exist:
   ```bash
   ls -la prebuilds/
   ```

### Import Errors After Installation

**Symptoms:** `import rtms` or `require('@zoom/rtms')` fails

**Solutions:**
1. **Python:**
   ```bash
   # Check if installed
   pip list | grep rtms

   # Check if SDK libraries bundled
   python -c "import rtms; print(rtms.__file__)"
   ls -la $(python -c "import rtms; print(rtms.__path__[0])")

   # Verify RPATH settings
   otool -L /path/to/_rtms.so  # macOS
   ldd /path/to/_rtms.so       # Linux
   ```

2. **Node.js:**
   ```bash
   # Check if installed
   npm list @zoom/rtms

   # Check if prebuild downloaded
   ls -la node_modules/@zoom/rtms/build/Release/

   # Try rebuilding
   npm rebuild @zoom/rtms
   ```

3. **General:**
   - Test in clean virtual environment
   - Check RPATH settings in CMakeLists.txt
   - Verify SDK libraries are bundled in package

### Platform-Specific Issues

**macOS codesigning:**
```bash
# May need to sign dylibs for distribution
codesign --force --sign - lib/darwin-arm64/*.dylib
```

**Linux glibc compatibility:**
- Use manylinux wheel tags for broad compatibility
- Test on multiple Linux distributions
- Check glibc version requirements

**Missing SDK libraries:**
- Ensure SDK files in `lib/platform-arch/`
- Check file permissions: `chmod +x lib/**/*.{so,dylib}`
- Verify paths in CMakeLists.txt

## Rollback Procedures

### npm (Node.js)

**Deprecate a version:**
```bash
npm deprecate @zoom/rtms@X.Y.Z "Reason for deprecation"
```

**Unpublish (within 72 hours only):**
```bash
npm unpublish @zoom/rtms@X.Y.Z
```

**Best practice:** Always deprecate instead of unpublish

### PyPI (Python)

**Yank a release:**
```bash
# Makes version unavailable but doesn't delete it
twine yank rtms X.Y.Z -r pypi --reason "Reason for yanking"
```

**Note:** PyPI doesn't allow unpublishing. Yanking is the only option.

### GitHub Releases

**Delete a release:**
1. Go to: https://github.com/zoom/rtms/releases
2. Click release to delete
3. Click "Delete this release"
4. Delete associated tag:
   ```bash
   git tag -d js-vX.Y.Z
   git push origin :refs/tags/js-vX.Y.Z
   ```

## Future CI/CD Enhancements

Consider automating these manual steps:

### Automated Publishing on Tags
- Trigger npm/PyPI publish when version tags pushed
- Use GitHub Actions workflows
- Example: Tag `python-v0.0.2` → auto-publish to PyPI

### Multi-Platform Builds in CI
- Build wheels for both darwin-arm64 and linux-x64 in GitHub Actions
- Use matrix builds or cross-compilation
- Upload artifacts for easy downloading

### Release Notes Generation
- Auto-generate changelogs from commit messages
- Use conventional commits
- Tools: conventional-changelog, release-please

### Version Validation
- Check if version was bumped before allowing merge
- Prevent publishing same version twice
- Use pre-commit hooks or GitHub Actions

### Security Scanning
- Add vulnerability scanning for dependencies
- Use: Dependabot, Snyk, npm audit, safety
- Block PRs with high-severity vulnerabilities

## Resources

- **npm Publishing:** https://docs.npmjs.com/cli/v8/commands/npm-publish
- **PyPI Publishing:** https://packaging.python.org/tutorials/packaging-projects/
- **Twine Documentation:** https://twine.readthedocs.io/
- **scikit-build-core:** https://scikit-build-core.readthedocs.io/
- **prebuild:** https://github.com/prebuild/prebuild
- **GitHub Actions:** https://docs.github.com/en/actions
- **GitHub Pages:** https://docs.github.com/en/pages

## Getting Help

- **Issues:** https://github.com/zoom/rtms/issues
- **Discussions:** https://github.com/zoom/rtms/discussions
- **Internal:** Contact Max Mansfield (max.mansfield@zoom.us)
