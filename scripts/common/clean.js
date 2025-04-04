#!/usr/bin/env node

import { getProjectRoot, removeDir, success } from './utils.js';
import { join } from 'path';

const PREFIX = "Clean";

function cleanProject() {
  const rootDir = getProjectRoot();
  
  // Clean build artifacts
  removeDir(join(rootDir, 'dist'), PREFIX);
  removeDir(join(rootDir, 'build'), PREFIX);
  removeDir(join(rootDir, 'docs'), PREFIX);
  removeDir(join(rootDir, 'node_modules'), PREFIX);
  removeDir(join(rootDir, 'prebuilds'), PREFIX);


  
  success(PREFIX, 'Clean completed successfully');
}

cleanProject();