#!/usr/bin/env node

import { log, run, executeScript, getBuildMode } from './utils.js';

const PREFIX = "Build";

function buildNodeJS() {
  log(PREFIX, 'Building Node.js module...');
  const buildMode = getBuildMode();
  const debugFlag = buildMode === 'debug' ? ' --debug' : '';
  
  run(`cmake-js compile${debugFlag}`, PREFIX);
  run('npx tsc', PREFIX);

  log(PREFIX, `Node.js module built in ${buildMode} mode`);
}

function buildPython() {
  log(PREFIX, 'Building Python module...');
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

function prebuild() {
  log(PREFIX, 'Generating prebuilds...');
  run('npx prebuild --strip -t 9 -r napi --backend cmake-js', PREFIX);
}

function upload() {
  log(PREFIX, 'Uploading prebuilds...');
  run('npx prebuild --backend cmake-js --upload-all', PREFIX);
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
  go: buildGo,
  prebuild: prebuild,
  all: buildAll
});