#!/usr/bin/env node

import { log, run, executeScript } from './utils.js';

const PREFIX = "Docs";

function generateNodeJSDocs() {
  log(PREFIX, 'Generating Node.js documentation...');

  // a bug forces me to use multiple highlightLanguages options
  run('npx typedoc rtms.d.ts  --highlightLanguages py --highlightLanguages js --highlightLanguages sh --highlightLanguages ts --plugin typedoc-plugin-missing-exports --out docs/js', PREFIX);
}

function generatePythonDocs() {
  log(PREFIX, 'Generating Python documentation...');
  run("pdoc3 -o docs/py/ --force --html rtms", PREFIX)
}

function generateAllDocs() {
  generateNodeJSDocs();
  generatePythonDocs();
}

// Run using our common execution wrapper
executeScript(PREFIX, {
  'js': generateNodeJSDocs,
  'python': generatePythonDocs,
  'all': generateAllDocs
});