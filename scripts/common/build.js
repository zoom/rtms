#!/usr/bin/env node

import { log, run, executeScript, getBuildMode } from './utils.js';

const PREFIX = "Build";

function buildNodeJS() {
  log(PREFIX, 'Building Node.js module...');
  const buildMode = getBuildMode();
  const debugFlag = buildMode === 'debug' ? ' --debug' : '';
  
  run(`node-gyp rebuild${debugFlag}`, PREFIX);
  run('tsc', PREFIX);

  log(PREFIX, `Node.js module built in ${buildMode} mode`);
}

function buildPython() {
  log(PREFIX, 'Building Python module...');
  const buildMode = getBuildMode();
  const cmakeBuildType = buildMode === 'debug' ? 'Debug' : 'Release';
  
  run(`pipx run build --wheel -Ccmake.build-type='${cmakeBuildType}' --outdir dist/py`, PREFIX);
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
  run('prebuildify --napi --all --strip', PREFIX);
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
  go: buildGo,
  prebuild: prebuild,
  all: buildAll
});