#!/usr/bin/env node

import { setupFrameworks } from './frameworks.js';
import { log, run, executeScript, getBuildMode } from './utils.js';

const PREFIX = "Build";

function buildNodeJS() {
  log(PREFIX, 'Building Node.js module...');
  const buildMode = getBuildMode();
  const debugFlag = buildMode === 'debug' ? ' --debug' : '';
  
  run(`cmake-js compile${debugFlag}`, PREFIX);

  log(PREFIX, `Node.js module built in ${buildMode} mode`);
}

function buildPython() {
  log(PREFIX, 'Building Pythno module...');
  const buildMode = getBuildMode();
  const cmakeBuildType = buildMode === 'debug' ? 'Debug' : 'Release';
  
  run(`python3 -m build -Ccmake.build-type='${cmakeBuildType}' --outdir dist/py`, PREFIX);
  log(PREFIX, `Python module built in ${buildMode} mode`);
}

function buildGo() {
  log(PREFIX, 'Building Go module...');
  const buildMode = getBuildMode();
  const debugFlag = buildMode === 'debug' ? ' --debug' : '';
  
  run(`cmake-js configure${debugFlag} --CDGO=true`, PREFIX);
  
  log(PREFIX, `Go module built in ${buildMode} mode`);
}

const prebuildCmd = "prebuild -t 9 -r napi \
    --include-regex '\.(node|dylib|so.0|tar.gz|ts|js|js.map)$'  \
    --backend cmake-js";

function prebuild() {
  log(PREFIX, 'Generating prebuilds...');
  run(prebuildCmd, PREFIX);
}

function upload() {
  log(PREFIX, 'Uploading prebuilds...');
  run(`${prebuildCmd} -u "$GITHUB_TOKEN"`, PREFIX);

}

function upload_linux() {
  log(PREFIX, 'Uploading prebuilds...');
  run(`${prebuildCmd} --arch x64 --platform linux -u "$GITHUB_TOKEN"`, PREFIX);

}


function buildAll() {
  buildNodeJS();
  buildPython();
  buildGo();
}

// Run using our common execution wrapper
executeScript(PREFIX, {
  js: buildNodeJS,
  python: buildPython,
  upload,
  upload_linux,
  setupFrameworks,
  go: buildGo,
  prebuild: prebuild,
  all: buildAll
});