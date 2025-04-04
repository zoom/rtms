#!/usr/bin/env node

import { log, getProjectRoot, executeScript, removeDir, success } from './utils.js';
import { join } from 'path';

const PREFIX = "Clean";

function cleanProject() {
  const rootDir = getProjectRoot();
  
  // Clean build artifacts
  removeDir(join(rootDir, 'dist'), PREFIX);
  removeDir(join(rootDir, 'build'), PREFIX);
  removeDir(join(rootDir, 'docs'), PREFIX);
  removeDir(join(rootDir, 'node_modules'), PREFIX);

  
  success(PREFIX, 'Clean completed successfully');
}

function cleanAll() {
  const rootDir = getProjectRoot();
  
  cleanProject();

  // Clean prebuild artifacts
  removeDir(join(rootDir, 'prebuilds'), PREFIX);
  
  success(PREFIX, 'Prebuild clean completed successfully');
}


// Run using our common execution wrapper
executeScript(PREFIX, {
  all: cleanAll,
  project: cleanProject,
});