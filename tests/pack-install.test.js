/**
 * Integration test: npm pack → install → load
 *
 * This test validates the end-user installation flow by:
 * 1. Creating an npm tarball with `npm pack`
 * 2. Installing the tarball in a temporary directory
 * 3. Verifying the native module loads correctly
 *
 * This catches issues like missing files, broken install scripts,
 * or unextracted framework archives that would affect users.
 */

const { execSync } = require('child_process');
const { mkdtempSync, rmSync, existsSync, readdirSync, writeFileSync } = require('fs');
const { join } = require('path');
const { tmpdir } = require('os');

describe('npm pack → install → load integration', () => {
  let tempDir;
  let tarballPath;

  beforeAll(() => {
    // Create tarball from current package
    const output = execSync('npm pack --json', { encoding: 'utf8' });
    const packages = JSON.parse(output);
    const { filename } = packages[0];
    tarballPath = join(process.cwd(), filename);
  });

  beforeEach(() => {
    // Create fresh temp directory for each test
    tempDir = mkdtempSync(join(tmpdir(), 'rtms-test-'));
  });

  afterEach(() => {
    // Clean up temp directory
    if (tempDir && existsSync(tempDir)) {
      rmSync(tempDir, { recursive: true, force: true });
    }
  });

  afterAll(() => {
    // Clean up tarball
    if (tarballPath && existsSync(tarballPath)) {
      rmSync(tarballPath);
    }
  });

  test('tarball contains required files', () => {
    const output = execSync(`tar -tzf ${tarballPath}`, { encoding: 'utf8' });
    const files = output.split('\n').filter(Boolean);

    // Core files that must be present
    expect(files).toEqual(
      expect.arrayContaining([
        'package/scripts/install.js',
        'package/index.ts',
        'package/rtms.d.ts',
        'package/package.json',
        'package/CMakeLists.txt',
        'package/src/node.cpp',
        'package/src/rtms.cpp',
        'package/src/rtms.h'
      ])
    );
  });

  test('native module loads after install from tarball', () => {
    // Create minimal package.json for ESM
    writeFileSync(join(tempDir, 'package.json'), JSON.stringify({ type: 'module' }));

    // Install from tarball (this runs scripts/install.js which downloads prebuilds)
    execSync(`npm install ${tarballPath}`, {
      cwd: tempDir,
      stdio: 'pipe',
      env: { ...process.env, npm_config_loglevel: 'error' }
    });

    // Verify native module loads and has expected exports (matching test.js usage patterns)
    const testScript = `
      import rtms from '@zoom/rtms';
      const checks = [
        typeof rtms.Client === 'function',
        typeof rtms.onWebhookEvent === 'function',
        typeof rtms.MEDIA_TYPE_AUDIO !== 'undefined',
        typeof rtms.MEDIA_TYPE_VIDEO !== 'undefined'
      ];
      console.log(checks.every(Boolean) ? 'OK' : 'FAIL');
    `;

    const result = execSync(`node -e "${testScript}"`, {
      cwd: tempDir,
      encoding: 'utf8'
    });

    expect(result.trim()).toBe('OK');
  });

  test('macOS frameworks are extracted (not left as tar.gz)', () => {
    if (process.platform !== 'darwin') {
      // Skip on non-macOS - frameworks only exist on darwin
      return;
    }

    writeFileSync(join(tempDir, 'package.json'), JSON.stringify({ type: 'module' }));
    execSync(`npm install ${tarballPath}`, {
      cwd: tempDir,
      stdio: 'pipe',
      env: { ...process.env, npm_config_loglevel: 'error' }
    });

    const buildDir = join(tempDir, 'node_modules/@zoom/rtms/build/Release');

    if (!existsSync(buildDir)) {
      // Prebuild might not be available for this platform - skip
      console.log('Build directory not found - prebuild may not be available');
      return;
    }

    const files = readdirSync(buildDir);

    // Frameworks should be extracted as directories
    expect(files).toContain('tp.framework');
    expect(files).toContain('util.framework');
    expect(files).toContain('curl64.framework');

    // Archives should NOT remain (they should be extracted and deleted)
    expect(files).not.toContain('tp.framework.tar.gz');
    expect(files).not.toContain('util.framework.tar.gz');
    expect(files).not.toContain('curl64.framework.tar.gz');

    // Verify frameworks are actually directories with content
    const tpFramework = join(buildDir, 'tp.framework');
    expect(existsSync(tpFramework)).toBe(true);
    expect(readdirSync(tpFramework).length).toBeGreaterThan(0);
  });

  test('Client can be instantiated', () => {
    writeFileSync(join(tempDir, 'package.json'), JSON.stringify({ type: 'module' }));
    execSync(`npm install ${tarballPath}`, {
      cwd: tempDir,
      stdio: 'pipe',
      env: { ...process.env, npm_config_loglevel: 'error' }
    });

    // Test that Client class works
    const testScript = `
      import rtms from '@zoom/rtms';
      const client = new rtms.Client();
      console.log(typeof client.join === 'function' ? 'OK' : 'FAIL');
    `;

    const result = execSync(`node -e "${testScript}"`, {
      cwd: tempDir,
      encoding: 'utf8'
    });

    expect(result.trim()).toBe('OK');
  });
});
