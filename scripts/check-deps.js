import fs from 'fs';
import os from 'os';
import path from 'path';
import { execSync } from 'child_process';
import { fileURLToPath } from 'url';
import { dirname } from 'path';

// Import https for Node.js < 18 fetch support
import https from 'https';

// Utility: Get project root directory
function getProjectRoot() {
  const __filename = fileURLToPath(import.meta.url);
  const __dirname = dirname(__filename);
  return path.join(__dirname, '..');
}

const PREFIX = "Dependencies";

// Platform detection and mapping
function detectPlatform() {
  const platform = os.platform();
  const arch = os.arch();
  
  let rtmsPlatform, rtmsArch, tagPrefix;
  
  if (platform === 'darwin') {
    rtmsPlatform = 'darwin';
    if (arch === 'arm64') {
      rtmsArch = 'arm64';
      tagPrefix = 'macos';
    } else {
      rtmsArch = 'x64';
      tagPrefix = 'macos';
    }
  } else if (platform === 'linux') {
    rtmsPlatform = 'linux';
    if (arch === 'arm64' || arch === 'aarch64') {
      rtmsArch = 'arm64';
      tagPrefix = 'linux';
    } else {
      rtmsArch = 'x64';
      tagPrefix = 'linux';
    }
  } else {
    throw new Error(`Unsupported platform: ${platform}`);
  }
  
  return { rtmsPlatform, rtmsArch, tagPrefix };
}

// Check if lib directory is empty or missing required files
function isLibDirectoryEmpty(libPath) {
  if (!fs.existsSync(libPath)) {
    console.log(`[RTMS ${PREFIX}]`, `Directory does not exist: ${libPath}`);
    return true;
  }
  
  const files = fs.readdirSync(libPath);
  // Filter out .gitkeep files
  const relevantFiles = files.filter(file => file !== '.gitkeep');
  
  console.log(`[RTMS ${PREFIX}]`, `Directory ${libPath} contains: [${files.join(', ')}]`);
  console.log(`[RTMS ${PREFIX}]`, `Relevant files (excluding .gitkeep): [${relevantFiles.join(', ')}]`);
  
  return relevantFiles.length === 0;
}

// Simple fetch implementation using https module for better compatibility
function httpsGet(url) {
  return new Promise((resolve, reject) => {
    https.get(url, {
      headers: {
        'User-Agent': 'rtms-build-script'
      }
    }, (res) => {
      let data = '';
      
      res.on('data', (chunk) => {
        data += chunk;
      });
      
      res.on('end', () => {
        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve(data);
        } else {
          reject(new Error(`HTTP ${res.statusCode}: ${res.statusMessage}`));
        }
      });
    }).on('error', (err) => {
      reject(err);
    });
  });
}

// Download binary file using https module with redirect handling
function httpsDownload(url, filePath, maxRedirects = 5) {
  return new Promise((resolve, reject) => {
    const file = fs.createWriteStream(filePath);
    
    function doDownload(downloadUrl, redirectCount) {
      if (redirectCount > maxRedirects) {
        reject(new Error('Too many redirects'));
        return;
      }
      
      https.get(downloadUrl, {
        headers: {
          'User-Agent': 'rtms-build-script'
        }
      }, (res) => {
        // Handle redirects
        if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
          console.log(`[RTMS ${PREFIX}]`, `Following redirect to: ${res.headers.location}`);
          doDownload(res.headers.location, redirectCount + 1);
          return;
        }
        
        if (res.statusCode >= 200 && res.statusCode < 300) {
          res.pipe(file);
          
          file.on('finish', () => {
            file.close();
            resolve();
          });
          
          file.on('error', (err) => {
            try {
              fs.unlinkSync(filePath);
            } catch (e) {
              // Ignore cleanup errors
            }
            reject(err);
          });
        } else {
          try {
            fs.unlinkSync(filePath);
          } catch (e) {
            // Ignore cleanup errors
          }
          reject(new Error(`HTTP ${res.statusCode}: ${res.statusMessage}`));
        }
      }).on('error', (err) => {
        try {
          fs.unlinkSync(filePath);
        } catch (e) {
          // Ignore cleanup errors
        }
        reject(err);
      });
    }
    
    doDownload(url, 0);
  });
}

// Fetch latest release for platform from GitHub
async function fetchLatestRelease(tagPrefix) {
  try {
    console.log(`[RTMS ${PREFIX}]`, `Fetching latest ${tagPrefix} release from GitHub...`);
    
    const data = await httpsGet('https://api.github.com/repos/zoom/rtms-sdk-cpp/releases');
    const releases = JSON.parse(data);
    
    // Find the latest release that matches our platform
    const matchingRelease = releases.find(release => 
      release.tag_name.startsWith(tagPrefix) && !release.prerelease
    );
    
    if (!matchingRelease) {
      throw new Error(`No ${tagPrefix} release found`);
    }
    
    console.log(`[RTMS ${PREFIX}]`, `Found release: ${matchingRelease.tag_name}`);
    return matchingRelease;
  } catch (err) {
    throw new Error(`Failed to fetch release information: ${err.message}`);
  }
}

// Helper function to copy directory contents
function copyDirectoryContents(sourceDir, destDir) {
  if (!fs.existsSync(destDir)) {
    fs.mkdirSync(destDir, { recursive: true });
  }
  
  const items = fs.readdirSync(sourceDir);
  
  for (const item of items) {
    const sourcePath = path.join(sourceDir, item);
    const destPath = path.join(destDir, item);
    
    if (fs.statSync(sourcePath).isDirectory()) {
      copyDirectoryContents(sourcePath, destPath);
    } else {
      fs.copyFileSync(sourcePath, destPath);
      console.log(`[RTMS ${PREFIX}]`, `Copied: ${item}`);
    }
  }
}

// Helper function to find a file recursively
function findFileRecursively(dir, filename) {
  const items = fs.readdirSync(dir);
  
  for (const item of items) {
    const itemPath = path.join(dir, item);
    
    if (fs.statSync(itemPath).isDirectory()) {
      const found = findFileRecursively(itemPath, filename);
      if (found) return found;
    } else if (item === filename) {
      return itemPath;
    }
  }
  
  return null;
}

// Helper function to copy .dylib files and framework directories
function copyDylibsAndFrameworks(sourceDir, destDir) {
  function processDirectory(currentDir) {
    const items = fs.readdirSync(currentDir);
    
    for (const item of items) {
      const itemPath = path.join(currentDir, item);
      const stat = fs.statSync(itemPath);
      
      if (stat.isDirectory()) {
        if (item.endsWith('.framework')) {
          // Copy entire framework directory
          const destPath = path.join(destDir, item);
          console.log(`[RTMS ${PREFIX}]`, `Copying framework: ${item}`);
          copyDirectoryContents(itemPath, destPath);
        } else {
          // Recursively process subdirectories
          processDirectory(itemPath);
        }
      } else if (item.endsWith('.dylib')) {
        // Copy .dylib file
        const destPath = path.join(destDir, item);
        console.log(`[RTMS ${PREFIX}]`, `Copying dylib: ${item}`);
        fs.copyFileSync(itemPath, destPath);
      }
    }
  }
  
  processDirectory(sourceDir);
}

// Download and extract release asset with selective file placement
async function downloadAndExtractRelease(release, rtmsPlatform, rtmsArch) {
  const projectRoot = getProjectRoot();
  const tempDir = path.join(projectRoot, 'temp_extract');
  
  try {
    // Find the first asset (assuming there's only one per release)
    const asset = release.assets[0];
    if (!asset) {
      throw new Error('No assets found in release');
    }
    
    console.log(`[RTMS ${PREFIX}]`, `Downloading ${asset.name}...`);
    
    const includeDir = path.join(projectRoot, 'lib', 'include');
    const platformDir = path.join(projectRoot, 'lib', `${rtmsPlatform}-${rtmsArch}`);
    
    // Ensure directories exist
    fs.mkdirSync(tempDir, { recursive: true });
    fs.mkdirSync(includeDir, { recursive: true });
    fs.mkdirSync(platformDir, { recursive: true });
    
    // Download to temporary file
    const tempFile = path.join(tempDir, asset.name);
    await httpsDownload(asset.browser_download_url, tempFile);
    
    console.log(`[RTMS ${PREFIX}]`, `Downloaded ${asset.name}, extracting...`);
    
    // Extract to temporary directory first
    if (asset.name.endsWith('.tar.gz') || asset.name.endsWith('.tgz')) {
      execSync(`tar -xzf "${tempFile}" -C "${tempDir}"`, { 
        stdio: 'inherit' 
      });
    } else if (asset.name.endsWith('.zip')) {
      execSync(`unzip -q "${tempFile}" -d "${tempDir}"`, { 
        stdio: 'inherit' 
      });
    } else {
      throw new Error(`Unknown archive format: ${asset.name}`);
    }
    
    // Remove the downloaded archive
    fs.unlinkSync(tempFile);
    
    console.log(`[RTMS ${PREFIX}]`, 'Organizing extracted files...');
    
    // Check what was extracted to the temp directory
    const tempContents = fs.readdirSync(tempDir);
    console.log(`[RTMS ${PREFIX}]`, `Temp directory contents: ${tempContents.join(', ')}`);
    
    // First, check if files were extracted directly to tempDir
    // Look for common indicators that this is the root: h/ directory AND library files
    const hasHeaderDir = tempContents.includes('h') && fs.statSync(path.join(tempDir, 'h')).isDirectory();
    const hasLibFiles = tempContents.some(item => item.includes('librtmsdk') || item.endsWith('.so') || item.endsWith('.so.0'));
    
    let extractedDir;
    
    if (hasHeaderDir || hasLibFiles || tempContents.length > 3) {
      // Files were likely extracted directly to tempDir
      console.log(`[RTMS ${PREFIX}]`, 'Files extracted directly to temp directory');
      extractedDir = tempDir;
    } else {
      // Look for a single subdirectory that contains everything
      const subdirs = tempContents.filter(item => {
        const itemPath = path.join(tempDir, item);
        return fs.statSync(itemPath).isDirectory() && item !== '__MACOSX';
      });
      
      if (subdirs.length === 1) {
        extractedDir = path.join(tempDir, subdirs[0]);
        console.log(`[RTMS ${PREFIX}]`, `Using subdirectory: ${subdirs[0]}`);
      } else {
        // Fallback to temp directory
        console.log(`[RTMS ${PREFIX}]`, 'Multiple or no subdirectories found, using temp directory');
        extractedDir = tempDir;
      }
    }
    
    console.log(`[RTMS ${PREFIX}]`, `Using extracted directory: ${extractedDir}`);
    const extractedContents = fs.readdirSync(extractedDir);
    console.log(`[RTMS ${PREFIX}]`, `Contents: ${extractedContents.join(', ')}`);
    
    // Copy header files from h/ folder to lib/include/
    const headerSourceDir = path.join(extractedDir, 'h');
    if (fs.existsSync(headerSourceDir)) {
      console.log(`[RTMS ${PREFIX}]`, 'Copying header files from h/ to lib/include/...');
      copyDirectoryContents(headerSourceDir, includeDir);
      console.log(`[RTMS ${PREFIX} Success]`, 'Header files copied successfully');
    } else {
      console.warn(`[RTMS ${PREFIX} Warning]`, `No h/ directory found in ${extractedDir}`);
      console.log(`[RTMS ${PREFIX}]`, `Available directories: ${extractedContents.filter(item => 
        fs.statSync(path.join(extractedDir, item)).isDirectory()).join(', ')}`);
    }
    
    // Platform-specific file handling
    if (rtmsPlatform === 'linux') {
      // Copy librtmsdk.so.0 to platform directory
      console.log(`[RTMS ${PREFIX}]`, 'Looking for librtmsdk.so.0...');
      
      // First try to find it directly in the extracted directory
      let soFile = null;
      const directSoPath = path.join(extractedDir, 'librtmsdk.so.0');
      if (fs.existsSync(directSoPath)) {
        soFile = directSoPath;
      } else {
        // Search recursively
        soFile = findFileRecursively(extractedDir, 'librtmsdk.so.0');
      }
      
      if (soFile) {
        const destPath = path.join(platformDir, 'librtmsdk.so.0');
        fs.copyFileSync(soFile, destPath);
        console.log(`[RTMS ${PREFIX} Success]`, `Copied librtmsdk.so.0 from ${soFile} to ${platformDir}`);
      } else {
        console.warn(`[RTMS ${PREFIX} Warning]`, 'librtmsdk.so.0 not found in archive');
        // Show what files are available for debugging
        console.log(`[RTMS ${PREFIX}]`, `All files in extracted directory:`);
        function listFiles(dir, prefix = '') {
          const items = fs.readdirSync(dir);
          items.forEach(item => {
            const itemPath = path.join(dir, item);
            if (fs.statSync(itemPath).isDirectory()) {
              console.log(`[RTMS ${PREFIX}]`, `${prefix}${item}/`);
              listFiles(itemPath, prefix + '  ');
            } else {
              console.log(`[RTMS ${PREFIX}]`, `${prefix}${item}`);
            }
          });
        }
        listFiles(extractedDir);
      }
    } else if (rtmsPlatform === 'darwin') {
      // Copy all .dylib files and framework directories
      console.log(`[RTMS ${PREFIX}]`, 'Copying .dylib files and frameworks to platform directory...');
      copyDylibsAndFrameworks(extractedDir, platformDir);
      console.log(`[RTMS ${PREFIX} Success]`, 'Darwin-specific files copied successfully');
    }
    
    console.log(`[RTMS ${PREFIX} Success]`, `Successfully organized files for ${rtmsPlatform}-${rtmsArch}`);
  } catch (err) {
    throw new Error(`Failed to download and extract release: ${err.message}`);
  } finally {
    // Clean up temporary directory
    if (fs.existsSync(tempDir)) {
      fs.rmSync(tempDir, { recursive: true, force: true });
    }
  }
}

// Check and fetch SDK libraries if needed
async function checkSDKLibraries(force = false) {
  try {
    const { rtmsPlatform, rtmsArch, tagPrefix } = detectPlatform();
    const libPath = path.join(getProjectRoot(), 'lib', `${rtmsPlatform}-${rtmsArch}`);
    const includePath = path.join(getProjectRoot(), 'lib', 'include');
    
    console.log(`[RTMS ${PREFIX}]`, `Checking SDK libraries for ${rtmsPlatform}-${rtmsArch}...`);
    
    // Check if both platform-specific lib and include directories are empty
    const isPlatformLibEmpty = isLibDirectoryEmpty(libPath);
    const isIncludeEmpty = isLibDirectoryEmpty(includePath);
    
    const shouldFetchSDK = force || isPlatformLibEmpty || isIncludeEmpty;
    
    if (shouldFetchSDK) {
      if (force) {
        console.log(`[RTMS ${PREFIX}]`, `Force reinstalling SDK libraries for ${rtmsPlatform}-${rtmsArch}...`);
        // Clean existing files but preserve .gitkeep
        if (fs.existsSync(libPath)) {
          const files = fs.readdirSync(libPath).filter(f => f !== '.gitkeep');
          files.forEach(file => {
            const filePath = path.join(libPath, file);
            if (fs.statSync(filePath).isDirectory()) {
              fs.rmSync(filePath, { recursive: true, force: true });
            } else {
              fs.unlinkSync(filePath);
            }
          });
        }
        if (fs.existsSync(includePath)) {
          const files = fs.readdirSync(includePath).filter(f => f !== '.gitkeep');
          files.forEach(file => {
            const filePath = path.join(includePath, file);
            if (fs.statSync(filePath).isDirectory()) {
              fs.rmSync(filePath, { recursive: true, force: true });
            } else {
              fs.unlinkSync(filePath);
            }
          });
        }
      } else {
        console.log(`[RTMS ${PREFIX}]`, `SDK libraries missing for ${rtmsPlatform}-${rtmsArch} (platform: ${isPlatformLibEmpty}, include: ${isIncludeEmpty})`);
      }
      
      console.log(`[RTMS ${PREFIX}]`, 'Fetching from GitHub...');
      
      const release = await fetchLatestRelease(tagPrefix);
      await downloadAndExtractRelease(release, rtmsPlatform, rtmsArch);
      
      console.log(`[RTMS ${PREFIX} Success]`, `SDK libraries installed for ${rtmsPlatform}-${rtmsArch}`);
    } else {
      console.log(`[RTMS ${PREFIX}]`, `SDK libraries already present for ${rtmsPlatform}-${rtmsArch}`);
    }
  } catch (err) {
    console.error(`[RTMS ${PREFIX} Error]`, `Failed to check/fetch SDK libraries: ${err.message}`);
    return false;
  }
  
  return true;
}

// Main function - only checks/downloads SDK
async function checkDeps(force = false) {
  return await checkSDKLibraries(force);
}

export { checkDeps, checkSDKLibraries as checkSDKDeps };

// Main execution when run directly
if (import.meta.url === `file://${process.argv[1]}`) {
  checkDeps().then(result => {
    process.exit(result ? 0 : 1);
  }).catch(err => {
    console.error(`[RTMS ${PREFIX} Error]`, err.message);
    process.exit(1);
  });
}