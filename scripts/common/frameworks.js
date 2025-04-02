import { promises as fs } from 'fs';
import { join, dirname } from 'path';
import { createRequire } from 'module';
import { fileURLToPath } from 'url';


import { log, getBuildMode } from './utils.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const buildMode = getBuildMode();
const buildDir= buildMode === 'debug' ? 'Debug' : 'Release';

const PREFIX = 'Frameworks';

async function extractFrameworks() {
    const require = createRequire(import.meta.url);
    const tar = require('tar');
    
    const dir = join(__dirname, `../../build/${buildDir}/`);
    log(PREFIX, `Extracting Framworks... ${dir}`);
    const frameworksDir = dir
    const outputDir = dir

    try {
        // Create output directory if it doesn't exist
        await fs.mkdir(outputDir, { recursive: true });
        
        // Get all tar files
        const files = await fs.readdir(frameworksDir);
        console.log("Files: ", files);
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