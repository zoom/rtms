#!/usr/bin/env node

import { log, getProjectRoot, removeDir, success } from './utils.js';
import { join } from 'path';

const PREFIX = "Clean";

function cleanProject() {
  const rootDir = getProjectRoot();
  
  // Clean build artifacts
  removeDir(join(rootDir, 'dist'), PREFIX);
  removeDir(join(rootDir, 'build'), PREFIX);
  removeDir(join(rootDir, 'prebuilds'), PREFIX);
  removeDir(join(rootDir, 'docs'), PREFIX);
  
  // Clean temp files
  removeDir(join(rootDir, 'node_modules/.cache'), PREFIX);
  
  success(PREFIX, 'Clean completed successfully');
}

// Run the clean operation
cleanProject();