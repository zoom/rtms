#!/usr/bin/env node

import { log, run, executeScript } from './utils.js';

const PREFIX = "Docs";

function generateNodeJSDocs() {
  log(PREFIX, 'Generating Node.js documentation...');
  run('npx typedoc --plugin typedoc-plugin-missing-exports --out docs/js', PREFIX);
}

function generatePythonDocs() {
  log(PREFIX, 'Generating Python documentation...');
  // Placeholder for Python docs command
  log(PREFIX, 'Python documentation generation not yet implemented');
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