---
name: Pull Request
about: Submit changes to the RTMS SDK
---

## Description

This PR introduces a comprehensive release infrastructure overhaul for the Zoom RTMS SDK, transitioning from manual ad-hoc releases to a fully automated CI/CD pipeline with Trusted Publishing (OIDC) and manual approval gates.

**Key highlights:**
- **Git tag-based publishing**: Push `js-v1.0.0` or `py-v1.0.0` tags to trigger automated builds and publishing
- **Pre-release tag support**: RC/alpha/beta tags for testing (Node.js=dry-run, Python=TestPyPI)
- **Trusted Publishing (OIDC)**: Secure authentication without long-lived API tokens for both npm and PyPI
- **Manual approval gates**: Human review required before packages are published to registries
- **Multi-platform builds**: Automated builds for Linux x64 and macOS ARM64
- **cibuildwheel integration**: Single command builds all 8 Python wheels (4 Python versions × 2 platforms)
- **Version 1.0.0**: Both Node.js and Python bindings are now production-ready

## Related Issues

Implements release infrastructure for v1.0.0 launch.

## Type of Change
- [ ] Bug fix (non-breaking change that fixes an issue)
- [x] New feature (non-breaking change that adds functionality)
- [x] Breaking change (fix or feature that would cause existing functionality to change)
- [x] Documentation update
- [ ] Refactoring (no functional changes)
- [ ] Performance improvement
- [x] Tests (adding or improving tests)
- [x] Build changes

## Affected Components
- [x] Core C++ implementation
- [x] Node.js bindings
- [x] Python bindings
- [ ] Go bindings
- [x] Build system
- [x] Documentation
- [x] Examples
- [ ] Other (please specify)

## Commits Included (22 total)

### Release Infrastructure
| Commit | Description |
|--------|-------------|
| fb01e9f | feat(ci): add git tag-based publishing with manual approval |
| c2da1d0 | feat(ci): add Trusted Publishing (OIDC) for npm and PyPI |
| 8607247 | feat(python): add cibuildwheel for multi-version wheel building |
| 72d1bcc | fix(ci): grant contents:write permission for publish workflow |
| (new) | feat(ci): add pre-release tag support (RC/alpha/beta) |

### CI/CD Fixes
| Commit | Description |
|--------|-------------|
| 58651e1 | fix(build): clean platform-specific wheels before Python builds |
| 9a9b689 | fix(ci): use correct artifact path for scoped package prebuilds |
| 0adcfd4 | fix(ci): bypass Docker dependency for Node.js prebuild jobs |
| 4dd3865 | fix(ci): resolve manylinux and prebuild build failures |
| b17a54d | fix(ci): replace manual tests with automated tests |
| f0077fe | fix(ci): CI/CD fixes |
| fd14bd9 | fix(docker): call task setup before build in test-js container |
| 5a764bf | fix(ci): use python -m pip to ensure correct Python interpreter |
| 1d8d781 | fix(ci): use GITHUB_TOKEN for SDK downloads to avoid rate limits |
| b5c3e5c | fix(ci): add missing 'build' package for Python builds |
| 63c5a6e | ci: enable workflows on feat/release-infra for testing |

### Python Fixes
| Commit | Description |
|--------|-------------|
| 5180172 | fix(python): correct RPATH for Linux shared library loading |
| 8a6be07 | fix(python): rename README.MD to README.md for standard convention |
| 6890f36 | fix(python): correct README filename case sensitivity |

### Documentation
| Commit | Description |
|--------|-------------|
| e554e2c | docs: add comprehensive git tag-based publishing guide |
| aa88d5e | docs: add Supported Products section to README |
| 08a697d | docs: add product-specific examples (Meetings, Webinars, Video SDK) |

### Version Bumps
| Commit | Description |
|--------|-------------|
| 990e319 | feat(events): redesign event system with typed callbacks (v1.0.0) |
| b57146a | chore: version bumps (intermediate versions) |

## Testing Performed

### CI/CD Testing
- [x] Workflow triggers correctly on `js-v*` and `py-v*` tags
- [x] Dry-run mode tested via workflow_dispatch
- [x] Build artifacts created successfully for all platforms
- [x] Manual approval gates function correctly

### Build Testing
- [x] Node.js prebuilds for darwin-arm64 and linux-x64 (N-API v9 and v10)
- [x] Python wheels for Python 3.10, 3.11, 3.12, 3.13 on both platforms
- [x] cibuildwheel produces all 8 expected wheels

### Automated Tests
- [x] Node.js tests pass on Linux (20.3.0, 22.x, 24.x) and macOS (24.x)
- [x] Python tests pass on Linux (3.10, 3.11, 3.12, 3.13) and macOS (3.10, 3.13)

## Checklist
- [x] My code follows the style guidelines of this project
- [x] I have performed a self-review of my code
- [x] I have commented my code, particularly in hard-to-understand areas
- [x] I have made corresponding changes to the documentation
- [x] My changes generate no new warnings
- [x] I have added tests that prove my fix is effective or that my feature works
- [x] New and existing unit tests pass locally with my changes
- [x] Any dependent changes have been merged and published

## Additional Context

### Breaking Changes

1. **Event System Redesign**: The event callback system has been redesigned with typed callbacks for both Node.js and Python. Existing code using the old event patterns will need to be updated.

2. **Minimum Version Requirements**:
   - Node.js: Now requires 20.3.0+ (was 18.x)
   - Python: Now requires 3.10+ (was 3.8+)

### Publishing Workflow

After this PR is merged:

1. Create version tags:
   ```bash
   git tag js-v1.0.0
   git tag py-v1.0.0
   git push origin js-v1.0.0 py-v1.0.0
   ```

2. The publish workflow will automatically:
   - Build all platform-specific artifacts
   - Pause for manual approval
   - Publish to npm and PyPI after approval
   - Create GitHub Releases

### Dry-Run Testing

To test the CI/CD without publishing:

1. Go to Actions → "Publish Packages" workflow
2. Click "Run workflow"
3. Select branch, language (node/python), version (1.0.0)
4. Set "Dry run" to `true`
5. Run and verify artifacts are built correctly

### Files Changed

**New Files:**
- `.github/workflows/publish.yml` - Git tag publishing workflow
- `PUBLISHING.md` - Comprehensive publishing guide
- `CHANGELOG.md` - Release changelog
- `examples/` - Product-specific example guides

**Modified Files:**
- `.github/workflows/main.yml` - Enhanced CI/CD with artifact reuse
- `Taskfile.yml` - New cibuildwheel tasks, wheel cleanup
- `pyproject.toml` - cibuildwheel configuration, v1.0.0
- `package.json` - v1.0.0
- `compose.yaml` - Automated test commands
- `README.md` - Supported products, updated badges
- `CLAUDE.md` - Updated documentation
