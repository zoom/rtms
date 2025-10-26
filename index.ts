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
    level: LogLevel.DEBUG,
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
 * Expose all constants from the native module
 * @param nativeModule the rtms .node module
 * @returns all native constants ready for export
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
  
  // Identify and collect all constants
  for (const key in nativeModule) {
    const value = nativeModule[key];
    if (isConstantObject(value)) {
      constants[key] = value;
    }
  }
  
  return constants;
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
 * @returns true if initialization succeeded
 * @throws Error if initialization failed
 */
function ensureInitialized(caPath?: string): boolean {
  if (isInitialized) {
    Logger.debug('rtms', 'SDK already initialized');
    return true;
  }
  
  const certPath = findCACertificate(caPath);
  
  try {
    Logger.info('rtms', `Initializing RTMS SDK with CA certificate: ${certPath || 'none'}`);
    nativeRtms.Client.initialize(certPath);
    isInitialized = true;
    Logger.info('rtms', 'RTMS SDK initialized successfully');
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
 * Creates a request handler for webhook events
 * 
 * @private
 * @param callback The webhook callback function
 * @param path The URL path to listen on
 * @returns A request handler function
 */
function createWebhookHandler(callback: WebhookCallbackUnion, path: string) {
  return (req: IncomingMessage, res: ServerResponse) => {
    const headers = { 'Content-Type': 'application/json' };

    if (req.method !== 'POST' || req.url !== path) {
      Logger.debug('webhook', `Rejected request: ${req.method} ${req.url} (expected: POST ${path})`);
      res.writeHead(404, headers);
      res.end(JSON.stringify({ error: 'Not Found' }));
      return;
    }

    let body = '';
    req.on('data', chunk => body += chunk.toString());

    req.on('end', () => {
      try {
        Logger.debug('webhook', `Received webhook request: ${req.url}`);
        const payload = JSON.parse(body);
        
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
    });
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
    Logger.info('webhook', 'Creating secure HTTPS webhook server', {
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
    Logger.info('webhook', 'Creating standard HTTP webhook server', { port, path });
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
  
  constructor() {
    super();
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
    ret = ensureInitialized(caPath);

    const {
      meeting_uuid,
      rtms_stream_id,
      server_urls,
      signature: providedSignature,
      client = process.env['ZM_RTMS_CLIENT'] || "",
      secret = process.env['ZM_RTMS_SECRET'] || "",
      timeout: providedTimeout = -1,
      pollInterval = 0
    } = options;

    this.pollRate = pollInterval;

    Logger.info('client', `Joining meeting: ${meeting_uuid}`, {
      streamId: rtms_stream_id,
      serverUrls: server_urls,
      timeout: providedTimeout,
      pollInterval
    });
    

    const finalSignature = providedSignature || generateSignature({
      client,
      secret,
      uuid: meeting_uuid,
      streamId: rtms_stream_id
    });

    try {
      ret = super.join(meeting_uuid, rtms_stream_id, finalSignature, server_urls, providedTimeout);
      
      if (ret) {
        Logger.info('client', `Successfully joined meeting: ${meeting_uuid}`);
      } else {
        Logger.error('client', `Failed to join meeting: ${meeting_uuid}`);
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error';
      Logger.error('client', `Error joining meeting: ${errorMessage}`, { meeting_uuid });
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

// Global client polling state
let globalPollingInterval: NodeJS.Timeout | null = null;
let globalPollRate: number = 0;

/**
 * Start global client polling
 * 
 * @private
 */
function startGlobalPolling(): void {
  if (globalPollingInterval) {
    clearInterval(globalPollingInterval);
  }
  
  Logger.debug('global', `Starting global polling with interval: ${globalPollRate}ms`);
  
  globalPollingInterval = setInterval(() => {
    try {
      nativeRtms.poll();
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error';
      Logger.error('global', `Error during global polling: ${errorMessage}`);
      stopGlobalPolling();
    }
  }, globalPollRate);
}

/**
 * Stop global client polling
 * 
 * @private
 */
function stopGlobalPolling(): void {
  if (globalPollingInterval) {
    Logger.debug('global', 'Stopping global polling');
    clearInterval(globalPollingInterval);
    globalPollingInterval = null;
  }
}

/**
 * Join a Zoom RTMS session using the global client
 * 
 * This function establishes a connection to a Zoom RTMS stream using the
 * global client singleton. It automatically handles initialization,
 * signature generation, and starts background polling for events.
 * 
 * @param options Object containing join parameters
 * @returns true if the join operation succeeds
 */
function join(options: JoinParams): boolean {
 
  let ret = false;
  const caPath = options.ca || process.env['ZM_RTMS_CA'];
  ret = ensureInitialized(caPath);

  const {
    meeting_uuid,
    rtms_stream_id,
    server_urls,
    signature: providedSignature,
    client = process.env['ZM_RTMS_CLIENT'] || "",
    secret = process.env['ZM_RTMS_SECRET'] || "",
    timeout: providedTimeout = -1,
    pollInterval = 0
  } = options;

  globalPollRate = pollInterval;

  Logger.info('global', `Joining meeting: ${meeting_uuid}`, {
    streamId: rtms_stream_id,
    serverUrls: server_urls,
    timeout: providedTimeout,
    globalPollingInterval
  });
  

  const finalSignature = providedSignature || generateSignature({
    client,
    secret,
    uuid: meeting_uuid,
    streamId: rtms_stream_id
  });

  try {
    ret = nativeRtms.join(meeting_uuid, rtms_stream_id, finalSignature, server_urls, providedTimeout);
    
    if (ret) {
      Logger.info('global', `Successfully joined meeting: ${meeting_uuid}`);
    } else {
      Logger.error('global', `Failed to join meeting: ${meeting_uuid}`);
    }
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : 'Unknown error';
    Logger.error('global', `Error joining meeting: ${errorMessage}`, { meeting_uuid });
    throw error;
  }

  if (ret)
    startGlobalPolling();


  return ret;
}

function setAudioParams(params: AudioParams): boolean {
  return setParameters<AudioParams>(
    'global',
    'audio',
    params,
    (p) => nativeRtms.setAudioParams(p)
  );
}

function setVideoParams(params: VideoParams): boolean {
  return setParameters<VideoParams>(
    'global',
    'video',
    params,
    (p) => nativeRtms.setVideoParams(p)
  )
}
  
  function setDeskshareParams(params: DeskshareParams): boolean {
    return setParameters<DeskshareParams>(
      'global',
      'deskshare',
      params,
      (p) => nativeRtms.setDeskshareParams(p)
    );
  }


/**
 * Leave the current session and clean up global client resources
 * 
 * This function disconnects from the Zoom RTMS stream, stops
 * background polling, and releases resources for the global client.
 * 
 * @returns true if the leave operation succeeds
 *
 */
function leave(): boolean {
  Logger.info('global', 'Leaving meeting');
  
  try {
    stopGlobalPolling();
    
    // If the native module has a leave function, use that
    if (typeof nativeRtms.leave === 'function') {
      const result = nativeRtms.leave();
      
      if (result) {
        Logger.info('global', 'Successfully left meeting');
      } else {
        Logger.warn('global', 'Left meeting with warnings');
      }
      
      return result;
    }
    
    // Otherwise, fall back to release (for backward compatibility)
    const result = nativeRtms.release();
    
    if (result) {
      Logger.info('global', 'Successfully released resources');
    } else {
      Logger.warn('global', 'Released resources with warnings');
    }
    
    return result;
  } catch (error) {
    const errorMessage = error instanceof Error ? error.message : 'Unknown error';
    Logger.error('global', `Error during leave: ${errorMessage}`);
    return false;
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

/**
 * Default export object for the RTMS module
 */
export default {
  // Class-based API
  Client,
  onWebhookEvent,
  
  // Global singleton API
  join,
  leave,
  setDeskshareParams: setDeskshareParams,
  setAudioParams: setAudioParams,
  setVideoParams: setVideoParams,
  poll: nativeRtms.poll,
  uuid: nativeRtms.uuid,
  streamId: nativeRtms.streamId,
  onJoinConfirm: nativeRtms.onJoinConfirm,
  onSessionUpdate: nativeRtms.onSessionUpdate,
  onUserUpdate: nativeRtms.onUserUpdate,
  onDeskshareData: nativeRtms.onDeskshareData,
  onAudioData: nativeRtms.onAudioData,
  onVideoData: nativeRtms.onVideoData,
  onTranscriptData: nativeRtms.onTranscriptData,
  onLeave: nativeRtms.onLeave,

  ...nativeConstants,
  
  // Utility functions
  generateSignature,
  isInitialized: () => isInitialized,
  
  // Logger configuration
  configureLogger,
  LogLevel,
  LogFormat
};