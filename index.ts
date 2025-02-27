import { createHmac } from 'crypto';
import { createRequire } from 'module';
import * as fs from 'fs';

import type {JoinParams, SignatureParams} from "./rtms.d.ts"

const require = createRequire(import.meta.url);
const nativeRtms = require('../../build/Release/rtms.node');

let isInitialized = false;

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
        pollInterval = 10
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
  Client,
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