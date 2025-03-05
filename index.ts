import * as fs from 'fs';
import { createHmac } from 'crypto';
import { createRequire } from 'module';
import { createServer, IncomingMessage, Server, ServerResponse } from 'http';

import type {JoinParams, SignatureParams, WebhookCallback} from "./rtms.d.ts"

const require = createRequire(import.meta.url);
const nativeRtms = require('bindings')('rtms');

let isInitialized = false;

let server: Server | undefined, port: number | string, path: string;


export function onWebhookEvent(callback: WebhookCallback): void {
  if (server?.listening) return;

  port = process.env['ZM_RTMS_PORT'] || 8080;
  path = process.env['ZM_RTMS_PATH'] || '/';

  server = createServer((req: IncomingMessage, res: ServerResponse) => {
    const headers = { 'Content-Type': 'application/json' };

    if (req.method !== 'POST' || req.url !== path) {
      res.writeHead(404, headers);
      res.end('Not Found');
      return;
    }

    let body = '';
    req.on('data', chunk => body += chunk.toString());

    req.on('end', () => {
      try {
        const payload = JSON.parse(body);
        callback(payload);
        res.writeHead(200, headers);
        res.end();
      } catch (e) {
        console.error('Error parsing webhook JSON:', e);
        res.writeHead(400, headers);
        res.end('Invalid JSON received');
      }
    });
  });

  server.listen(port, () => {
    console.log(`Listening for events at http://localhost:${port}${path}`);
  });
}

class Client extends nativeRtms.Client {
  private pollingInterval: NodeJS.Timeout | null = null;
  private pollRate: number = 0;
  
  constructor() {
    super();
  }
  
  join(options: JoinParams): boolean;
  join(meetingUuid: string, rtmsStreamId: string, signature: string, serverUrls: string, timeout?: number): boolean;
  join(optionsOrMeetingUuid: JoinParams | string, rtmsStreamId?: string, signature?: string, serverUrls?: string, timeout: number = -1): boolean {
    if (!isInitialized) {
      let caPath = '';
      
      if (typeof optionsOrMeetingUuid === 'object' && optionsOrMeetingUuid.ca) {
        caPath = optionsOrMeetingUuid.ca;
      } else {
        caPath = process.env['ZM_RTMS_CA'] || '';
        
        if (!caPath) {
          const commonLocations = [
            '/etc/ssl/certs/ca-certificates.crt',
            '/etc/pki/tls/certs/ca-bundle.crt',
            '/etc/ssl/ca-bundle.pem',
            '/etc/pki/tls/cacert.pem',
            '/etc/ssl/cert.pem'
          ];
          
          for (const location of commonLocations) {
            if (fs.existsSync(location)) {
              caPath = location;
              break;
            }
          }
        }
      }
      
      try {
        Client.initialize(caPath);
        isInitialized = true;
      } catch (error: unknown) {
        if (error instanceof Error) 
          throw new Error(`Failed to initialize RTMS SDK: ${error.message}`);
        else 
          console.error("An unknown error occurred:", error);
      }
    }
    
    let joinResult: boolean;
    
    if (typeof optionsOrMeetingUuid === 'object') {
      const options = optionsOrMeetingUuid;
      
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
      
      const finalSignature = providedSignature || generateSignature({
        client,
        secret,
        uuid: meeting_uuid,
        streamId: rtms_stream_id
      });
      
      joinResult = super.join(meeting_uuid, rtms_stream_id, finalSignature, server_urls, providedTimeout);
    } else {
      joinResult = super.join(optionsOrMeetingUuid, rtmsStreamId, signature, serverUrls, timeout);
    }
    
    if (joinResult) {
      this.startPolling();
    }
    
    return joinResult;
  }
  
  startPolling(): void {
    if (this.pollingInterval) {
      clearInterval(this.pollingInterval);
    }
    
    this.pollingInterval = setInterval(() => {
      try {
        super.poll();
      } catch (error) {
        console.error('Error during polling:', error);
        this.stopPolling();
      }
    }, this.pollRate);
  }
  
  stopPolling(): void {
    if (this.pollingInterval) {
      clearInterval(this.pollingInterval);
      this.pollingInterval = null;
    }
  }
  
  leave(): boolean {
    try {
      this.stopPolling();
      return super.release();
    } catch (error) {
      console.error('Error during leave:', error);
      return false;
    }
  }
}

// Global client functions
let globalPollingInterval: NodeJS.Timeout | null = null;
let globalPollRate: number = 0;

function join(options: JoinParams): boolean;
function join(meetingUuid: string, rtmsStreamId: string, signature: string, serverUrls: string, timeout?: number): boolean;
function join(optionsOrMeetingUuid: JoinParams | string, rtmsStreamId?: string, signature?: string, serverUrls?: string, timeout: number = -1): boolean {
  if (!isInitialized) {
    let caPath = '';
    
    if (typeof optionsOrMeetingUuid === 'object' && optionsOrMeetingUuid.ca) {
      caPath = optionsOrMeetingUuid.ca;
    } else {
      caPath = process.env['ZM_RTMS_CA'] || '';
      
      if (!caPath) {
        const commonLocations = [
          '/etc/ssl/certs/ca-certificates.crt',
          '/etc/pki/tls/certs/ca-bundle.crt',
          '/etc/ssl/ca-bundle.pem',
          '/etc/pki/tls/cacert.pem',
          '/etc/ssl/cert.pem'
        ];
        
        for (const location of commonLocations) {
          if (fs.existsSync(location)) {
            caPath = location;
            break;
          }
        }
      }
    }
    
    try {
      nativeRtms.Client.initialize(caPath);
      isInitialized = true;
    } catch (error: unknown) {
      if (error instanceof Error) 
        throw new Error(`Failed to initialize RTMS SDK: ${error.message}`);
      else 
        console.error("An unknown error occurred:", error);
    }
  }
  
  let joinResult: boolean;
  
  if (typeof optionsOrMeetingUuid === 'object') {
    const options = optionsOrMeetingUuid;
    
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
    
    const finalSignature = providedSignature || generateSignature({
      client,
      secret,
      uuid: meeting_uuid,
      streamId: rtms_stream_id
    });
    
    joinResult = nativeRtms.join({
      meeting_uuid,
      rtms_stream_id,
      signature: finalSignature,
      server_urls,
      timeout: providedTimeout
    });
  } else {
    joinResult = nativeRtms.join(optionsOrMeetingUuid, rtmsStreamId, signature, serverUrls, timeout);
  }
  
  if (joinResult) {
    startGlobalPolling();
  }
  
  return joinResult;
}

function startGlobalPolling(): void {
  if (globalPollingInterval) {
    clearInterval(globalPollingInterval);
  }
  
  globalPollingInterval = setInterval(() => {
    try {
      nativeRtms.poll();
    } catch (error) {
      console.error('Error during global polling:', error);
      stopGlobalPolling();
    }
  }, globalPollRate);
}

function stopGlobalPolling(): void {
  if (globalPollingInterval) {
    clearInterval(globalPollingInterval);
    globalPollingInterval = null;
  }
}

function leave(): boolean {
  try {
    stopGlobalPolling();
    
    // If the native module has a leave function, use that
    if (typeof nativeRtms.leave === 'function') {
      return nativeRtms.leave();
    }
    
    // Otherwise, fall back to release (for backward compatibility)
    return nativeRtms.release();
  } catch (error) {
    console.error('Error during global leave:', error);
    return false;
  }
}

function generateSignature({ client, secret, uuid, streamId }: SignatureParams): string {
  const clientId = process.env['ZM_RTMS_CLIENT'] || client;
  const clientSecret = process.env['ZM_RTMS_SECRET'] || secret;

  if (!clientId) throw new ReferenceError('Client ID cannot be blank');
  if (!clientSecret) throw new ReferenceError('Client Secret cannot be blank');

  return createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${streamId}`)
    .digest('hex');
}

export default {
  // Class-based API
  Client,
  onWebhookEvent,
  // Global singleton API
  join,
  leave,
  poll: nativeRtms.poll,
  uuid: nativeRtms.uuid,
  streamId: nativeRtms.streamId,
  onJoinConfirm: nativeRtms.onJoinConfirm,
  onSessionUpdate: nativeRtms.onSessionUpdate,
  onUserUpdate: nativeRtms.onUserUpdate,
  onAudioData: nativeRtms.onAudioData,
  onVideoData: nativeRtms.onVideoData,
  onTranscriptData: nativeRtms.onTranscriptData,
  onLeave: nativeRtms.onLeave,
  
  // Utility functions
  initialize: function(caPath?: string) {
    try {
      nativeRtms.Client.initialize(caPath);
      isInitialized = true;
      return true;
    } catch (error) {
      isInitialized = false;
      throw error;
    }
  },
  uninitialize: function() {
    try {
      nativeRtms.Client.uninitialize();
      isInitialized = false;
      return true;
    } catch (error) {
      throw error;
    }
  },
  generateSignature,
  isInitialized: () => isInitialized
};