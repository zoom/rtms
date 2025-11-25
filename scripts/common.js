/**
 * Common utilities shared across build scripts
 */

/**
 * Parse --platform argument from command line args
 * Supports both --platform=VALUE and --platform VALUE formats
 *
 * @param {string[]} args - Process arguments (typically process.argv.slice(2))
 * @returns {string|null} - Platform name or null if not specified
 */
export function parsePlatformArg(args) {
  const platformFlagIndex = args.findIndex(arg => arg === '--platform' || arg.startsWith('--platform='));

  if (platformFlagIndex === -1) {
    return null;
  }

  const arg = args[platformFlagIndex];

  if (arg.startsWith('--platform=')) {
    return arg.split('=')[1];
  }

  if (args[platformFlagIndex + 1]) {
    return args[platformFlagIndex + 1];
  }

  return null;
}

/**
 * Color codes for console output
 */
export const colors = {
  reset: '\x1b[0m',
  cyan: '\x1b[36m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  red: '\x1b[31m',
  dim: '\x1b[2m',
  bold: '\x1b[1m'
};

/**
 * Logging utilities
 */
export function log(prefix, message) {
  console.log(`${colors.cyan}[${prefix}]${colors.reset} ${message}`);
}

export function success(prefix, message) {
  console.log(`${colors.green}[${prefix} Success]${colors.reset} ${message}`);
}

export function error(prefix, message) {
  console.error(`${colors.red}[${prefix} Error]${colors.reset} ${message}`);
}

export function warning(prefix, message) {
  console.log(`${colors.yellow}[${prefix} Warning]${colors.reset} ${message}`);
}
