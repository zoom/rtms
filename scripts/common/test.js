#!/usr/bin/env node

import { log, run, executeScript } from './utils.js';

const PREFIX = "Test";

function testNodeJS() {
  log(PREFIX, 'Testing Node.js module...');
  run('npx jest tests/', PREFIX);
}

function testNodeJSManual() {
  log(PREFIX, 'Running manual Node.js test...');
  run('NODE_ENV=test node --env-file=.env test.js', PREFIX);
}

function testPython() {
  log(PREFIX, 'Testing Python module...');
  run('pip install --force dist/py/*.whl && python3 test.py', PREFIX);
}

function testGo() {
  log(PREFIX, 'Testing Go module...');
  log(PREFIX, 'Go tests not yet implemented');
  // Placeholder for Go test command
}

function testAll() {
  testNodeJS();
  testPython();
  testGo();
}

executeScript(PREFIX, {
  'js': testNodeJS,
  'js:manual': testNodeJSManual,
  'python': testPython,
  'go': testGo,
  'all': testAll
});