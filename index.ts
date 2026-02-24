import * as fs from 'fs';
import * as http from 'http';
import * as https from 'https';
import { createHmac } from 'crypto';
import { createRequire } from 'module';
import { IncomingMessage, Server, ServerResponse } from 'http';

import type { 
  JoinParams, SignatureParams, WebhookCallback, RawWebhookCallback,
  VideoParams, AudioParams, DeskshareParams
} from "./rtms.d.ts";

const require = createRequire(import.meta.url);
const nativeRtms = require('bindings')('rtms');

let isInitialized = false;
let webhookServer: Server | undefined;

/**
 * Available log levels for the RTMS SDK
 */
export enum LogLevel {
  /** Error messages only */
  ERROR = 0,
  /** Error and warning messages */
  WARN = 1,
  /** Error, warning, and informational messages (default) */
  INFO = 2,
  /** All messages including debug information */
  DEBUG = 3,
  /** All messages including detailed trace information */
  TRACE = 4
}

/**
 * Available log formats for the RTMS SDK
 */
export enum LogFormat {
  /** Human-readable progressive format for console output */
  PROGRESSIVE = 'progressive',
  /** Machine-readable JSON format for log processing */
  JSON = 'json'
}

/**
 * Configuration options for the logger
 */
export interface LoggerConfig {
  /** The minimum log level to display */
  level: LogLevel;
  /** The format for log output */
  format: LogFormat;
  /** Whether logging is enabled */
  enabled: boolean;
}

/**
 * Logger for the RTMS SDK
 * 
 * This namespace provides logging functionality with support for different
 * formats and levels. It can be configured via environment variables or
 * programmatically using the configure function.
 * 
 * Environment variables:
 * - ZM_RTMS_LOG_LEVEL: Set log level (error, warn, info, debug, trace)
 * - ZM_RTMS_LOG_FORMAT: Set log format (progressive, json)
 * - ZM_RTMS_LOG_ENABLED: Enable/disable logging (true, false)
 */
namespace Logger {
  // Default configuration
  let config: LoggerConfig = {
    level: LogLevel.INFO,
    format: LogFormat.PROGRESSIVE,
    enabled: true
  };
  
  /**
   * Initialize logger from environment variables
   */
  export function init(): void {
    // Read from environment variables
    const envLevel = process.env.ZM_RTMS_LOG_LEVEL;
    const envFormat = process.env.ZM_RTMS_LOG_FORMAT;
    const envEnabled = process.env.ZM_RTMS_LOG_ENABLED;
    
    if (envLevel) {
      switch (envLevel.toLowerCase()) {
        case 'error': config.level = LogLevel.ERROR; break;
        case 'warn': config.level = LogLevel.WARN; break;
        case 'info': config.level = LogLevel.INFO; break;
        case 'debug': config.level = LogLevel.DEBUG; break;
        case 'trace': config.level = LogLevel.TRACE; break;
      }
    }
    
    if (envFormat) {
      switch (envFormat.toLowerCase()) {
        case 'json': config.format = LogFormat.JSON; break;
        case 'progressive': config.format = LogFormat.PROGRESSIVE; break;
      }
    }
    
    if (envEnabled) {
      config.enabled = envEnabled.toLowerCase() !== 'false';
    }
  }
  
  /**
   * Configure logger programmatically
   * 
   * @param options Configuration options to apply
   */
  export function configure(options: Partial<LoggerConfig>): void {
    config = { ...config, ...options };
  }
  
  /**
   * Format and return log message string
   */
  function formatLog(level: LogLevel, component: string, message: string, details?: any): string {
    const timestamp = new Date().toISOString();
    const levelNames = ['ERROR', 'WARN', 'INFO', 'DEBUG', 'TRACE'];
    
    if (config.format === LogFormat.JSON) {
      return JSON.stringify({
        timestamp,
        level: levelNames[level].toLowerCase(),
        component,
        message,
        details: details || undefined
      });
    } else {
      // Progressive format
      const paddedComponent = component.padEnd(8);
      const paddedLevel = levelNames[level].padEnd(5);
      
      return `${paddedComponent} | ${timestamp} | ${paddedLevel} | ${message}`;
    }
  }
  
  /**
   * Log an error message
   * 
   * @param component The component generating the log
   * @param message The log message
   * @param details Optional details object
   */
  export function error(component: string, message: string, details?: any): void {
    if (!config.enabled || config.level < LogLevel.ERROR) return;
    console.error(formatLog(LogLevel.ERROR, component, message, details));
  }
  
  /**
   * Log a warning message
   * 
   * @param component The component generating the log
   * @param message The log message
   * @param details Optional details object
   */
  export function warn(component: string, message: string, details?: any): void {
    if (!config.enabled || config.level < LogLevel.WARN) return;
    console.warn(formatLog(LogLevel.WARN, component, message, details));
  }
  
  /**
   * Log an informational message
   * 
   * @param component The component generating the log
   * @param message The log message
   * @param details Optional details object
   */
  export function info(component: string, message: string, details?: any): void {
    if (!config.enabled || config.level < LogLevel.INFO) return;
    console.log(formatLog(LogLevel.INFO, component, message, details));
  }
  
  /**
   * Log a debug message
   * 
   * @param component The component generating the log
   * @param message The log message
   * @param details Optional details object
   */
  export function debug(component: string, message: string, details?: any): void {
    if (!config.enabled || config.level < LogLevel.DEBUG) return;
    console.log(formatLog(LogLevel.DEBUG, component, message, details));
  }
  
  /**
   * Log a trace message
   * 
   * @param component The component generating the log
   * @param message The log message
   * @param details Optional details object
   */
  export function trace(component: string, message: string, details?: any): void {
    if (!config.enabled || config.level < LogLevel.TRACE) return;
    console.log(formatLog(LogLevel.TRACE, component, message, details));
  }
}

// Initialize logger from environment variables
Logger.init();


/**
 * Expose all constant objects from the native module (dictionaries like AudioCodec, VideoResolution)
 * @param nativeModule the rtms .node module
 * @returns all native constant objects ready for export
 */
function exposeNativeConstants(nativeModule: any): Record<string, any> {
  const constants: Record<string, any> = {};

  // Check if the object has properties and isn't a function or primitive
  function isConstantObject(obj: any): boolean {
    return obj !== null &&
           typeof obj === 'object' &&
           !Array.isArray(obj) &&
           Object.keys(obj).length > 0 &&
           Object.values(obj).every(val => typeof val === 'number' || typeof val === 'string');
  }

  // Identify and collect all constant objects
  for (const key in nativeModule) {
    const value = nativeModule[key];
    if (isConstantObject(value)) {
      constants[key] = value;
    }
  }

  return constants;
}

/**
 * Expose individual number constants from the native module
 * @param nativeModule the rtms .node module
 * @returns all native number constants ready for export
 */
function exposeNumberConstants(nativeModule: any): Record<string, number> {
  const numberConstants: Record<string, number> = {};

  // List of known number constants to export
  const knownConstants = [
    // Media types
    'MEDIA_TYPE_AUDIO', 'MEDIA_TYPE_VIDEO', 'MEDIA_TYPE_DESKSHARE',
    'MEDIA_TYPE_TRANSCRIPT', 'MEDIA_TYPE_CHAT', 'MEDIA_TYPE_ALL',
    // Session events
    'SESSION_EVENT_ADD', 'SESSION_EVENT_STOP', 'SESSION_EVENT_PAUSE', 'SESSION_EVENT_RESUME',
    // User events
    'USER_JOIN', 'USER_LEAVE',
    // Event types (for subscribeEvent/onEventEx)
    'EVENT_UNDEFINED', 'EVENT_FIRST_PACKET_TIMESTAMP', 'EVENT_ACTIVE_SPEAKER_CHANGE',
    'EVENT_PARTICIPANT_JOIN', 'EVENT_PARTICIPANT_LEAVE',
    'EVENT_SHARING_START', 'EVENT_SHARING_STOP', 'EVENT_MEDIA_CONNECTION_INTERRUPTED',
    'EVENT_CONSUMER_ANSWERED', 'EVENT_CONSUMER_END',
    'EVENT_USER_ANSWERED', 'EVENT_USER_END', 'EVENT_USER_HOLD', 'EVENT_USER_UNHOLD',
    // SDK status codes
    'RTMS_SDK_OK', 'RTMS_SDK_FAILURE', 'RTMS_SDK_TIMEOUT',
    'RTMS_SDK_NOT_EXIST', 'RTMS_SDK_WRONG_TYPE', 'RTMS_SDK_INVALID_STATUS', 'RTMS_SDK_INVALID_ARGS',
    // Session status
    'SESS_STATUS_ACTIVE', 'SESS_STATUS_PAUSED'
  ];

  for (const name of knownConstants) {
    if (typeof nativeModule[name] === 'number') {
      numberConstants[name] = nativeModule[name];
    }
  }

  return numberConstants;
}

/**
 * Finds a suitable CA certificate file for SSL verification
 * 
 * This function tries to locate a CA certificate file in the following order:
 * 1. User-specified path
 * 2. ZM_RTMS_CA environment variable
 * 3. Common system locations on Linux and macOS
 * 
 * @private
 * @param specifiedPath Optional explicit path to a CA certificate
 * @returns Path to a CA certificate or empty string if none found
 */
function findCACertificate(specifiedPath?: string): string {
  // Use explicit path if provided
  if (specifiedPath && fs.existsSync(specifiedPath)) {
    Logger.debug('rtms', `Using specified CA certificate: ${specifiedPath}`);
    return specifiedPath;
  }
  
  // Check environment variable
  const envPath = process.env['ZM_RTMS_CA'] || '';
  if (envPath && fs.existsSync(envPath)) {
    Logger.debug('rtms', `Using CA certificate from environment variable: ${envPath}`);
    return envPath;
  }
  
  // Try common system certificate locations
  const commonLocations = [
    // Linux locations
    '/etc/ssl/certs/ca-certificates.crt',  // Debian/Ubuntu
    '/etc/pki/tls/certs/ca-bundle.crt',    // Fedora/RHEL
    '/etc/ssl/ca-bundle.pem',              // OpenSUSE
    '/etc/pki/tls/cacert.pem',             // CentOS
    '/etc/ssl/cert.pem',                   // Alpine
    // macOS locations
    '/etc/ssl/cert.pem',                  // macOS
    '/usr/local/etc/openssl/cert.pem',    // Homebrew OpenSSL
    '/opt/homebrew/etc/openssl/cert.pem'  // Apple Silicon Homebrew
  ];
  
  for (const location of commonLocations) {
    if (fs.existsSync(location)) {
      Logger.debug('rtms', `Found system CA certificate: ${location}`);
      return location;
    }
  }
  
  Logger.warn('rtms', 'No CA certificate found, operation may fail');
  return '';  // No CA certificate found
}

/**
 * Ensures the RTMS SDK is initialized
 * 
 * This function handles initializing the RTMS SDK if it hasn't been already.
 * It automatically locates an appropriate CA certificate file for SSL verification.
 * 
 * @private
 * @param caPath Optional explicit path to a CA certificate
 * @param isVerifyCert Whether to verify TLS certificates (1 = verify, 0 = don't verify)
 * @param agent User agent string to send in requests
 * @returns true if initialization succeeded
 * @throws Error if initialization failed
 */
function ensureInitialized(caPath?: string, isVerifyCert?: number, agent?: string): boolean {
  if (isInitialized) {
    Logger.debug('rtms', 'SDK already initialized');
    return true;
  }
  
  const certPath = findCACertificate(caPath);
  
  try {
    Logger.debug('rtms', `Initializing RTMS SDK with CA certificate: ${certPath || 'none'}`);
    // Handle undefined values by providing defaults
    nativeRtms.Client.initialize(
      certPath, 
      isVerifyCert ?? 1,  // Use nullish coalescing to provide default
      agent ?? undefined
    );
    isInitialized = true;
    Logger.debug('rtms', 'RTMS SDK initialized successfully');
    return true;
  } catch (error: unknown) {
    isInitialized = false;
    if (error instanceof Error) {
      Logger.error('rtms', `Failed to initialize RTMS SDK: ${error.message}`);
      throw new Error(`Failed to initialize RTMS SDK: ${error.message}`);
    } else {
      Logger.error('rtms', 'Failed to initialize RTMS SDK: Unknown error');
      throw new Error("Failed to initialize RTMS SDK: Unknown error");
    }
  }
}

/**
 * Generates an HMAC-SHA256 signature for RTMS authentication
 * 
 * This function creates a signature required for authenticating with Zoom RTMS servers.
 * It uses HMAC-SHA256 with the client secret as the key and a concatenated string of
 * client ID, meeting UUID, and stream ID as the message.
 * 
 * @param params Parameters for signature generation
 * @returns Hex-encoded HMAC-SHA256 signature
 * @throws ReferenceError if client ID or secret is empty
 */
function generateSignature({ client, secret, uuid, streamId }: SignatureParams): string {
  const clientId = process.env['ZM_RTMS_CLIENT'] || client;
  const clientSecret = process.env['ZM_RTMS_SECRET'] || secret;

  if (!clientId) throw new ReferenceError('ZM_RTMS_CLIENT cannot be empty');
  if (!clientSecret) throw new ReferenceError('ZM_RTMS_SECRET cannot be empty');

  Logger.debug('rtms', `Generating signature for client: ${clientId}, uuid: ${uuid}, streamId: ${streamId}`);
  return createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${streamId}`)
    .digest('hex');
}

/**
 * Helper type to detect callback type
 */
type WebhookCallbackUnion = WebhookCallback | RawWebhookCallback;

/**
 * Type guard to check if callback is RawWebhookCallback
 * 
 * @private
 */
function isRawWebhookCallback(callback: WebhookCallbackUnion): callback is RawWebhookCallback {
  return callback.length >= 3;
}

/**
 * Creates a request handler for webhook events that can be mounted on existing HTTP servers
 *
 * This function returns a Node.js request handler compatible with Express, Fastify,
 * and other HTTP frameworks. It allows you to integrate Zoom webhook handling with
 * your existing application routes on a shared port.
 *
 * The handler validates that requests are POST requests to the specified path,
 * parses JSON payloads, and invokes your callback with the webhook data.
 *
 * @param callback Function to call when webhook events are received
 * @param path The URL path to listen on (e.g., '/zoom/webhook')
 * @returns A request handler function compatible with http.Server
 *
 * @example
 * ```typescript
 * import express from 'express';
 * import rtms from '@zoom/rtms';
 *
 * const app = express();
 *
 * // Your application routes
 * app.get('/health', (req, res) => res.json({ status: 'ok' }));
 *
 * // Mount Zoom webhook handler on the same server
 * const webhookHandler = rtms.createWebhookHandler(
 *   (payload) => {
 *     console.log(`Received: ${payload.event}`);
 *   },
 *   '/zoom/webhook'
 * );
 *
 * // Use the handler directly with Express
 * app.post('/zoom/webhook', webhookHandler);
 *
 * // Single port for all routes
 * app.listen(8080);
 * ```
 *
 * @category Common Functions
 */
export function createWebhookHandler(callback: WebhookCallbackUnion, path: string) {
  return (req: IncomingMessage, res: ServerResponse) => {
    const headers = { 'Content-Type': 'application/json' };

    if (req.method !== 'POST' || req.url !== path) {
      Logger.debug('webhook', `Rejected request: ${req.method} ${req.url} (expected: POST ${path})`);
      res.writeHead(404, headers);
      res.end(JSON.stringify({ error: 'Not Found' }));
      return;
    }

    const processPayload = (body: string) => {
      try {
        Logger.debug('webhook', `Received webhook request: ${req.url}`);
        const payload = JSON.parse(body);

        // Validate required webhook fields
        if (!payload.event || typeof payload.event !== 'string') {
          Logger.warn('webhook', 'Received webhook payload missing required "event" field');
          res.writeHead(400, headers);
          res.end(JSON.stringify({ error: 'Invalid webhook payload: missing required "event" field' }));
          return;
        }

        // Log the webhook event
        Logger.info('webhook', `Received event: ${payload.event || 'unknown'}`, {
          eventType: payload.event,
          payloadSize: body.length
        });

        // Check if this is a raw webhook callback
        const isRawCallback = isRawWebhookCallback(callback);

        if (isRawCallback) {
          // For raw callbacks, pass req and res objects
          process.nextTick(() => {
            try {
              (callback as RawWebhookCallback)(payload, req, res);
            } catch (err) {
              Logger.error('webhook', `Error in webhook callback: ${err instanceof Error ? err.message : 'Unknown error'}`);
              // Send error response if not already sent
              if (!res.headersSent) {
                res.writeHead(500, headers);
                res.end(JSON.stringify({ error: 'Internal Server Error' }));
              }
            }
          });
        } else {
          // For basic callbacks, only pass payload and auto-respond
          process.nextTick(() => {
            try {
              (callback as WebhookCallback)(payload);
            } catch (err) {
              Logger.error('webhook', `Error in webhook callback: ${err instanceof Error ? err.message : 'Unknown error'}`);
            }
          });

          res.writeHead(200, headers);
          res.end(JSON.stringify({ status: 'ok' }));
        }
      } catch (e) {
        Logger.error('webhook', `Error parsing webhook JSON: ${e instanceof Error ? e.message : 'Unknown error'}`);
        res.writeHead(400, headers);
        res.end(JSON.stringify({ error: 'Invalid JSON received' }));
      }
    };

    // Check if body was already parsed by middleware (e.g., express.json())
    if ((req as any).body !== undefined) {
      const body = typeof (req as any).body === 'string'
        ? (req as any).body
        : JSON.stringify((req as any).body);
      processPayload(body);
    } else {
      let body = '';
      req.on('data', chunk => body += chunk.toString());
      req.on('end', () => processPayload(body));
    }
  };
}

/**
 * Sets up a webhook server to receive events from Zoom
 * 
 * This function creates an HTTP or HTTPS server that listens for webhook events from Zoom.
 * When a webhook event is received, it parses the JSON payload and passes it to
 * the provided callback function.
 * 
 * For secure HTTPS connections, provide the following environment variables:
 * - ZM_RTMS_CERT: Path to SSL certificate file
 * - ZM_RTMS_KEY: Path to SSL certificate key file
 * - ZM_RTMS_CA_WEBHOOK: (Optional) Path to CA certificate for client verification
 * 
 * @param callback Function to call when webhook events are received
 * 
 */
export function onWebhookEvent(callback: WebhookCallback | RawWebhookCallback): void {
  if (webhookServer?.listening) {
    Logger.warn('webhook', 'Webhook server is already running');
    return;
  }

  const port = parseInt(process.env['ZM_RTMS_PORT'] || '8080');
  const path = process.env['ZM_RTMS_PATH'] || '/';
  
  // Check for TLS certificate configuration
  const certPath = process.env['ZM_RTMS_CERT'] || '';
  const keyPath = process.env['ZM_RTMS_KEY'] || '';
  const caPath = process.env['ZM_RTMS_CA_WEBHOOK'] || '';
  
  // Validate that both cert and key exist if either is provided
  const hasCert = certPath && fs.existsSync(certPath);
  const hasKey = keyPath && fs.existsSync(keyPath);
  
  if ((hasCert && !hasKey) || (!hasCert && hasKey)) {
    Logger.error('webhook', 'Both certificate and key must be provided for HTTPS', {
      certExists: hasCert,
      keyExists: hasKey
    });
    throw new Error('Both certificate and key must be provided for HTTPS');
  }
  
  const useSecureServer = hasCert && hasKey;
  
  // Create the request handler (works with both WebhookCallback and RawWebhookCallback)
  const requestHandler = createWebhookHandler(callback as WebhookCallbackUnion, path);
  
  if (useSecureServer) {
    // Set up HTTPS server with the provided certificates
    Logger.debug('webhook', 'Creating secure HTTPS webhook server', {
      port,
      path,
      cert: certPath,
      clientVerification: !!caPath
    });
    
    try {
      const options: https.ServerOptions = {
        cert: fs.readFileSync(certPath),
        key: fs.readFileSync(keyPath)
      };
      
      // Optional: Add CA certificate for client verification if provided
      if (caPath && fs.existsSync(caPath)) {
        options.ca = fs.readFileSync(caPath);
        options.requestCert = true;
        options.rejectUnauthorized = true;
        Logger.debug('webhook', `Using CA certificate for client verification: ${caPath}`);
      }
      
      webhookServer = https.createServer(options, requestHandler);
    } catch (error) {
      Logger.error('webhook', `Failed to create HTTPS server: ${error instanceof Error ? error.message : 'Unknown error'}`);
      throw error;
    }
  } else {
    // Fall back to regular HTTP server
    Logger.debug('webhook', 'Creating standard HTTP webhook server', { port, path });
    webhookServer = http.createServer(requestHandler);
  }

  webhookServer.on('error', (err) => {
    Logger.error('webhook', `Server error: ${err.message}`);
  });

  webhookServer.listen(port, () => {
    const protocol = useSecureServer ? 'https' : 'http';
    Logger.info('webhook', `Listening for webhook events at ${protocol}://localhost:${port}${path}`);
  });
}

/**
 * Validates audio parameters and throws helpful errors
 * 
 * @private
 * @param params Audio parameters to validate
 */
function validateAudioParams(params: AudioParams): void {
  // Validate contentType
  if (params.contentType !== undefined) {
    const validValues = Object.values(nativeRtms.AudioContentType || {});
    if (!validValues.includes(params.contentType)) {
      throw new Error(
        `Invalid audio contentType: ${params.contentType}. ` +
        `Use rtms.AudioContentType constants (e.g., rtms.AudioContentType.RAW_AUDIO)`
      );
    }
  }
  
  // Validate codec
  if (params.codec !== undefined) {
    const validValues = Object.values(nativeRtms.AudioCodec || {});
    if (!validValues.includes(params.codec)) {
      throw new Error(
        `Invalid audio codec: ${params.codec}. ` +
        `Use rtms.AudioCodec constants (e.g., rtms.AudioCodec.OPUS)`
      );
    }
  }
  
  // Validate sampleRate
  if (params.sampleRate !== undefined) {
    const validValues = Object.values(nativeRtms.AudioSampleRate || {});
    if (!validValues.includes(params.sampleRate)) {
      throw new Error(
        `Invalid audio sampleRate: ${params.sampleRate}. ` +
        `Use rtms.AudioSampleRate constants (e.g., rtms.AudioSampleRate.SR_48K)`
      );
    }
  }
  
  // Validate channel
  if (params.channel !== undefined) {
    const validValues = Object.values(nativeRtms.AudioChannel || {});
    if (!validValues.includes(params.channel)) {
      throw new Error(
        `Invalid audio channel: ${params.channel}. ` +
        `Use rtms.AudioChannel constants (e.g., rtms.AudioChannel.STEREO)`
      );
    }
  }
  
  // Validate dataOpt
  if (params.dataOpt !== undefined) {
    const validValues = Object.values(nativeRtms.AudioDataOption || {});
    if (!validValues.includes(params.dataOpt)) {
      throw new Error(
        `Invalid audio dataOpt: ${params.dataOpt}. ` +
        `Use rtms.AudioDataOption constants (e.g., rtms.AudioDataOption.AUDIO_MIXED_STREAM)`
      );
    }
  }
  
  // Validate numeric ranges
  if (params.duration !== undefined && (params.duration < 0 || params.duration > 10000)) {
    Logger.warn(
      'validation',
      `Audio duration ${params.duration}ms is outside typical range (0-10000ms)`
    );
  }
  
  if (params.frameSize !== undefined && (params.frameSize < 0 || params.frameSize > 100000)) {
    Logger.warn(
      'validation',
      `Audio frameSize ${params.frameSize} is outside typical range (0-100000)`
    );
  }
}

/**
 * Validates video parameters and throws helpful errors
 * 
 * @private
 * @param params Video parameters to validate
 */
function validateVideoParams(params: VideoParams): void {
  // Validate contentType
  if (params.contentType !== undefined) {
    const validValues = Object.values(nativeRtms.VideoContentType || {});
    if (!validValues.includes(params.contentType)) {
      throw new Error(
        `Invalid video contentType: ${params.contentType}. ` +
        `Use rtms.VideoContentType constants (e.g., rtms.VideoContentType.RAW_VIDEO)`
      );
    }
  }
  
  // Validate codec
  if (params.codec !== undefined) {
    const validValues = Object.values(nativeRtms.VideoCodec || {});
    if (!validValues.includes(params.codec)) {
      throw new Error(
        `Invalid video codec: ${params.codec}. ` +
        `Use rtms.VideoCodec constants (e.g., rtms.VideoCodec.H264)`
      );
    }
  }
  
  // Validate resolution
  if (params.resolution !== undefined) {
    const validValues = Object.values(nativeRtms.VideoResolution || {});
    if (!validValues.includes(params.resolution)) {
      throw new Error(
        `Invalid video resolution: ${params.resolution}. ` +
        `Use rtms.VideoResolution constants (e.g., rtms.VideoResolution.HD)`
      );
    }
  }
  
  // Validate dataOpt
  if (params.dataOpt !== undefined) {
    const validValues = Object.values(nativeRtms.VideoDataOption || {});
    if (!validValues.includes(params.dataOpt)) {
      throw new Error(
        `Invalid video dataOpt: ${params.dataOpt}. ` +
        `Use rtms.VideoDataOption constants (e.g., rtms.VideoDataOption.VIDEO_SINGLE_ACTIVE_STREAM)`
      );
    }
  }
  
  // Validate numeric ranges
  if (params.fps !== undefined && (params.fps < 1 || params.fps > 120)) {
    Logger.warn(
      'validation',
      `Video fps ${params.fps} is outside typical range (1-120)`
    );
  }
}

/**
 * Validates deskshare parameters and throws helpful errors
 * 
 * @private
 * @param params Deskshare parameters to validate
 */
function validateDeskshareParams(params: DeskshareParams): void {
  // Validate contentType
  if (params.contentType !== undefined) {
    const validValues = Object.values(nativeRtms.VideoContentType || {});
    if (!validValues.includes(params.contentType)) {
      throw new Error(
        `Invalid deskshare contentType: ${params.contentType}. ` +
        `Use rtms.VideoContentType constants (e.g., rtms.VideoContentType.RAW_VIDEO)`
      );
    }
  }
  
  // Validate codec
  if (params.codec !== undefined) {
    const validValues = Object.values(nativeRtms.VideoCodec || {});
    if (!validValues.includes(params.codec)) {
      throw new Error(
        `Invalid deskshare codec: ${params.codec}. ` +
        `Use rtms.VideoCodec constants (e.g., rtms.VideoCodec.H264)`
      );
    }
  }
  
  // Validate resolution
  if (params.resolution !== undefined) {
    const validValues = Object.values(nativeRtms.VideoResolution || {});
    if (!validValues.includes(params.resolution)) {
      throw new Error(
        `Invalid deskshare resolution: ${params.resolution}. ` +
        `Use rtms.VideoResolution constants (e.g., rtms.VideoResolution.HD)`
      );
    }
  }
  
  // Validate numeric ranges
  if (params.fps !== undefined && (params.fps < 1 || params.fps > 120)) {
    Logger.warn(
      'validation',
      `Deskshare fps ${params.fps} is outside typical range (1-120)`
    );
  }
}

/**
 * Generic function to set media parameters with consistent logging
 * 
 * @param context Context identifier for logging (client/global)
 * @param operation Function name for parameter setting 
 * @param type Parameter type (audio/video/deskshare)
 * @param params Parameter object to pass to the operation
 * @param operation Function to call with the parameters
 * @returns Result of the operation
 */
function setParameters<T>(
  context: string,
  type: string, 
  params: T, 
  operation: (params: T) => boolean
): boolean {
  Logger.debug(context, `Setting ${type} parameters: ${JSON.stringify(params)}`);
  
  // Validate parameters based on type
  try {
    if (type === 'audio' && 'codec' in (params as any)) {
      validateAudioParams(params as unknown as AudioParams);
    } else if (type === 'video' && 'codec' in (params as any)) {
      validateVideoParams(params as unknown as VideoParams);
    } else if (type === 'deskshare' && 'codec' in (params as any)) {
      validateDeskshareParams(params as unknown as DeskshareParams);
    }
  } catch (validationError) {
    Logger.error(context, `Parameter validation failed: ${validationError instanceof Error ? validationError.message : 'Unknown error'}`);
    throw validationError;
  }
  
  try {
    const result = operation(params);
    if (result) {
      Logger.debug(context, `${type} parameters set successfully`);
    } else {
      Logger.warn(context, `Setting ${type} parameters returned false`);
    }
    return result;
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : 'Unknown error';
    Logger.error(context, `Error setting ${type} parameters: ${errorMessage}`);
    throw error;
  }
}

/**
 * Client class for connecting to Zoom RTMS streams
 * 
 * This class provides an object-oriented interface for connecting to and
 * processing Zoom RTMS streams. Use this approach when you need to connect
 * to multiple meetings simultaneously.
 * 
 */
class Client extends nativeRtms.Client {
  private pollingInterval: NodeJS.Timeout | null = null;
  private pollRate: number = 0;
  private participantEventCallback: ((event: 'join' | 'leave', timestamp: number, participants: Array<{ userId: number; userName?: string }>) => void) | null = null;
  private activeSpeakerEventCallback: ((timestamp: number, userId: number, userName: string) => void) | null = null;
  private sharingEventCallback: ((event: 'start' | 'stop', timestamp: number, userId?: number, userName?: string) => void) | null = null;
  private rawEventCallback: ((eventData: string) => void) | null = null;
  private mediaConnectionInterruptedCallback: ((timestamp: number) => void) | null = null;
  private eventHandlerRegistered: boolean = false;

  constructor() {
    super();
  }

  /**
   * Internal event dispatcher that routes events to typed callbacks
   * @private
   */
  private setupEventHandler(): void {
    if (this.eventHandlerRegistered) return;
    this.eventHandlerRegistered = true;

    super.onEventEx((eventData: string) => {
      // Call raw callback if set
      if (this.rawEventCallback) {
        this.rawEventCallback(eventData);
      }

      // Parse and dispatch to typed callbacks
      try {
        const data = JSON.parse(eventData);
        const eventType = data.event_type;

        switch (eventType) {
          case nativeRtms.EVENT_PARTICIPANT_JOIN:
            if (this.participantEventCallback) {
              const participants = (data.participants || []).map((p: any) => ({
                userId: p.user_id,
                userName: p.user_name
              }));
              this.participantEventCallback('join', data.timestamp || 0, participants);
            }
            break;

          case nativeRtms.EVENT_PARTICIPANT_LEAVE:
            if (this.participantEventCallback) {
              const participants = (data.participants || []).map((p: any) => ({
                userId: p.user_id,
                userName: p.user_name
              }));
              this.participantEventCallback('leave', data.timestamp || 0, participants);
            }
            break;

          case nativeRtms.EVENT_ACTIVE_SPEAKER_CHANGE:
            if (this.activeSpeakerEventCallback) {
              this.activeSpeakerEventCallback(
                data.timestamp || 0,
                data.user_id || 0,
                data.user_name || ''
              );
            }
            break;

          case nativeRtms.EVENT_SHARING_START:
            if (this.sharingEventCallback) {
              this.sharingEventCallback(
                'start',
                data.timestamp || 0,
                data.user_id,
                data.user_name
              );
            }
            break;

          case nativeRtms.EVENT_SHARING_STOP:
            if (this.sharingEventCallback) {
              this.sharingEventCallback(
                'stop',
                data.timestamp || 0
              );
            }
            break;

          case nativeRtms.EVENT_MEDIA_CONNECTION_INTERRUPTED:
            if (this.mediaConnectionInterruptedCallback) {
              this.mediaConnectionInterruptedCallback(data.timestamp || 0);
            }
            break;
        }
      } catch (e) {
        Logger.error('client', `Failed to parse event: ${e}`);
      }
    });
  }
  
  /**
   * Join a Zoom RTMS session
   * 
   * This method establishes a connection to a Zoom RTMS stream.
   * It automatically handles initialization, signature generation,
   * and starts background polling for events.
   * 
   * @param options Object containing join parameters
   * @returns true if the join operation succeeds

   */
  join(options: JoinParams): boolean {
    let ret = false;
    const caPath = options.ca || process.env['ZM_RTMS_CA'];
    const isVerifyCert = options.is_verify_cert !== undefined ? options.is_verify_cert : 1;
    const agent = options.agent;
    ret = ensureInitialized(caPath, isVerifyCert, agent);

    const {
      meeting_uuid,
      webinar_uuid,
      session_id,
      rtms_stream_id,
      server_urls,
      signature: providedSignature,
      client = process.env['ZM_RTMS_CLIENT'] || "",
      secret = process.env['ZM_RTMS_SECRET'] || "",
      timeout: providedTimeout = -1,
      pollInterval = 0
    } = options;

    // Use meeting_uuid for Meeting SDK, webinar_uuid for Webinar, session_id for Video SDK
    const instance_id = meeting_uuid || webinar_uuid || session_id;

    this.pollRate = pollInterval;

    Logger.info('client', `Joining ${meeting_uuid ? 'meeting' : webinar_uuid ? 'webinar' : 'session'}: ${instance_id}`, {
      streamId: rtms_stream_id,
      serverUrls: server_urls,
      timeout: providedTimeout,
      pollInterval
    });

    if (!instance_id) {
      throw new Error('Either meeting_uuid, webinar_uuid, or session_id must be provided');
    }

    const finalSignature = providedSignature || generateSignature({
      client,
      secret,
      uuid: instance_id,
      streamId: rtms_stream_id
    });

    try {
      ret = super.join(instance_id, rtms_stream_id, finalSignature, server_urls, providedTimeout);

      if (ret) {
        Logger.info('client', `Successfully joined: ${instance_id}`);
      } else {
        Logger.error('client', `Failed to join: ${instance_id}`);
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error';
      Logger.error('client', `Error joining: ${errorMessage}`, { instance_id });
      throw error;
    }

    if (ret)
      this.startPolling();


    return ret;
  }

  /**
   * Sets audio parameters for the client
   */
  setAudioParams(params: AudioParams): boolean {
    return setParameters<AudioParams>(
      'client',
      'audio',
      params,
      (p) => super.setAudioParams(p)
    );
  }

  /**
   * Sets video parameters for the client
   */
  setVideoParams(params: VideoParams): boolean {
    return setParameters<VideoParams>(
      'client',
      'video',
      params,
      (p) => super.setVideoParams(p)
    );
  }

  /** 
  * Sets deskshare parameters for the client
  */
 setDeskshareParams(params: DeskshareParams): boolean {
   return setParameters<DeskshareParams>(
     'client',
     'deskshare',
     params,
     (p) => super.setDeskshareParams(p)
   );
 }

  /**
   * Register a callback for participant join/leave events
   *
   * This automatically subscribes to EVENT_PARTICIPANT_JOIN and EVENT_PARTICIPANT_LEAVE.
   * Events are delivered as parsed objects, not raw JSON.
   *
   * @param callback Function called when participants join or leave
   * @returns true if registration succeeds
   *
   * @example
   * ```typescript
   * client.onParticipantEvent((event, timestamp, participants) => {
   *   console.log(`Participant ${event}:`, participants);
   * });
   * ```
   */
  onParticipantEvent(callback: (event: 'join' | 'leave', timestamp: number, participants: Array<{ userId: number; userName?: string }>) => void): boolean {
    this.participantEventCallback = callback;
    this.setupEventHandler();

    // Auto-subscribe to participant events
    try {
      super.subscribeEvent([nativeRtms.EVENT_PARTICIPANT_JOIN, nativeRtms.EVENT_PARTICIPANT_LEAVE]);
    } catch (e) {
      Logger.warn('client', `Failed to auto-subscribe to participant events: ${e}`);
    }

    return true;
  }

  /**
   * Register a callback for active speaker change events
   *
   * This automatically subscribes to EVENT_ACTIVE_SPEAKER_CHANGE.
   *
   * @param callback Function called when the active speaker changes
   * @returns true if registration succeeds
   *
   * @example
   * ```typescript
   * client.onActiveSpeakerEvent((timestamp, userId, userName) => {
   *   console.log(`Active speaker: ${userName} (${userId})`);
   * });
   * ```
   */
  onActiveSpeakerEvent(callback: (timestamp: number, userId: number, userName: string) => void): boolean {
    this.activeSpeakerEventCallback = callback;
    this.setupEventHandler();

    try {
      super.subscribeEvent([nativeRtms.EVENT_ACTIVE_SPEAKER_CHANGE]);
    } catch (e) {
      Logger.warn('client', `Failed to auto-subscribe to active speaker events: ${e}`);
    }

    return true;
  }

  /**
   * Register a callback for sharing start/stop events
   *
   * This automatically subscribes to EVENT_SHARING_START and EVENT_SHARING_STOP.
   * Note: These events only work when the RTMS app has DESKSHARE scope permission.
   *
   * @param callback Function called when sharing starts or stops
   * @returns true if registration succeeds
   *
   * @example
   * ```typescript
   * client.onSharingEvent((event, timestamp, userId, userName) => {
   *   console.log(`Sharing ${event} by ${userName}`);
   * });
   * ```
   */
  onSharingEvent(callback: (event: 'start' | 'stop', timestamp: number, userId?: number, userName?: string) => void): boolean {
    this.sharingEventCallback = callback;
    this.setupEventHandler();

    try {
      super.subscribeEvent([nativeRtms.EVENT_SHARING_START, nativeRtms.EVENT_SHARING_STOP]);
    } catch (e) {
      Logger.warn('client', `Failed to auto-subscribe to sharing events: ${e}`);
    }

    return true;
  }

  /**
   * Register a callback for media connection interrupted events
   *
   * This automatically subscribes to EVENT_MEDIA_CONNECTION_INTERRUPTED.
   *
   * @param callback Function called when the media connection is interrupted
   * @returns true if registration succeeds
   *
   * @example
   * ```typescript
   * client.onMediaConnectionInterrupted((timestamp) => {
   *   console.log(`Media connection interrupted at ${timestamp}`);
   * });
   * ```
   */
  onMediaConnectionInterrupted(callback: (timestamp: number) => void): boolean {
    this.mediaConnectionInterruptedCallback = callback;
    this.setupEventHandler();

    try {
      super.subscribeEvent([nativeRtms.EVENT_MEDIA_CONNECTION_INTERRUPTED]);
    } catch (e) {
      Logger.warn('client', `Failed to auto-subscribe to media connection interrupted events: ${e}`);
    }

    return true;
  }

  /**
   * Register a callback for raw event data
   *
   * This provides access to the raw JSON event data from the SDK.
   * Use this when you need custom event handling or access to all event types.
   * This callback is called IN ADDITION to typed callbacks, not instead of.
   *
   * @param callback Function called with raw JSON event data
   * @returns true if registration succeeds
   *
   * @example
   * ```typescript
   * client.onEventEx((eventData) => {
   *   const data = JSON.parse(eventData);
   *   console.log('Raw event:', data);
   * });
   * ```
   */
  onEventEx(callback: (eventData: string) => void): boolean {
    this.rawEventCallback = callback;
    this.setupEventHandler();
    return true;
  }

  /**
   * Start background polling for events
   * 
   * @private
   */
  private startPolling(): void {
    if (this.pollingInterval) {
      clearInterval(this.pollingInterval);
    }
    
    Logger.debug('client', `Starting polling with interval: ${this.pollRate}ms`);
    
    this.pollingInterval = setInterval(() => {
      try {
        super.poll();
      } catch (error) {
        const errorMessage = error instanceof Error ? error.message : 'Unknown error';
        Logger.error('client', `Error during polling: ${errorMessage}`);
        this.stopPolling();
      }
    }, this.pollRate);
  }
  
  /**
   * Stop background polling
   * 
   * @private
   */
  private stopPolling(): void {
    if (this.pollingInterval) {
      Logger.debug('client', 'Stopping polling');
      clearInterval(this.pollingInterval);
      this.pollingInterval = null;
    }
  }
  
  /**
   * Leave the current RTMS session and clean up resources
   * 
   * This method disconnects from the Zoom RTMS stream, stops
   * background polling, and releases resources.
   * 
   * @returns true if the leave operation succeeds
   */
  leave(): boolean {
    Logger.info('client', `Leaving meeting: ${this.uuid()}`);
    
    try {
      this.stopPolling();
      const result = super.release();
      
      if (result) {
        Logger.info('client', 'Successfully left meeting');
      } else {
        Logger.warn('client', 'Left meeting with warnings');
      }
      
      return result;
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error';
      Logger.error('client', `Error during leave: ${errorMessage}`);
      return false;
    }
  }
}

/**
 * Configure the RTMS logger
 * 
 * This function allows you to configure the logging behavior of the RTMS SDK.
 * You can change the log level, format, and enable/disable logging.
 * 
 * @param options Configuration options for the logger
 */
function configureLogger(options: Partial<LoggerConfig>): void {
  Logger.info('rtms', 'Configuring logger', options);
  Logger.configure(options);
}

const nativeConstants = exposeNativeConstants(nativeRtms);
const numberConstants = exposeNumberConstants(nativeRtms);

/**
 * Default export object for the RTMS module
 */
export default {
  // Class-based API
  Client,
  onWebhookEvent,
  createWebhookHandler,

  // Utility functions
  generateSignature,
  isInitialized: () => isInitialized,

  // Logger configuration
  configureLogger,
  LogLevel,
  LogFormat,

  // Native constant objects (AudioCodec, VideoResolution, etc.)
  ...nativeConstants,

  // Individual number constants (EVENT_PARTICIPANT_JOIN, RTMS_SDK_OK, etc.)
  ...numberConstants,
};