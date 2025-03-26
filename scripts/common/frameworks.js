import { promises as fs } from 'fs';
import { join, dirname } from 'path';
import { createRequire } from 'module';

import { log, getBuildMode } from './utils.js';
const buildMode = getBuildMode();
const buildDir= buildMode === 'debug' ? 'Debug' : 'Release';


async function extractFrameworks() {
    log(PREFIX, 'Extracting Framworks...');
    const require = createRequire(import.meta.url);
    const tar = require('tar');
    
    const frameworksDir = outputDir = join(dirname(import.meta.url), `build/${buildDir}`);

    try {
        // Create output directory if it doesn't exist
        await fs.mkdir(outputDir, { recursive: true });
        
        // Get all tar files
        const files = await fs.readdir(frameworksDir);
        const tarFiles = files.filter(file => file.endsWith('.tar.gz'));
        
        console.log(`Found ${tarFiles.length} framework archives to extract`);
        
        for (const tarFile of tarFiles) {
            const tarPath = join(frameworksDir, tarFile);
            console.log(`Extracting ${tarFile}...`);
            
            await tar.extract({
                file: tarPath,
                cwd: outputDir,
                strict: true
            });
            
            console.log(`Successfully extracted ${tarFile}`);
        }
    } catch (error) {
        console.error('Framework extraction failed:', error);
    }
}

// Call this function during installation/initialization
export async function setupFrameworks() {
    try {
        await extractFrameworks();
        return true;
    } catch (error) {
        console.error('Framework setup failed:', error);
        return false;
    }
}