import { createHmac } from 'crypto';
import { createServer, IncomingMessage, Server, ServerResponse } from 'http';
import { createRequire } from 'module';

import type { 
  JoinParams, SignatureParams, MediaTypes, 
  WebhookCallback, JoinConfirmCallback, 
  SessionUpdateCallback, UserUpdateCallback, 
  MediaDataCallback, LeaveCallback
} from './rtms.d.ts';

const req = createRequire(import.meta.url);
const rtms = req('./build/Release/rtms.node');

let server: Server | undefined, port: number | string, path: string;

/**
 * Joins an RTMS stream using the provided parameters.
 *
 * @param opts - The options for joining the RTMS stream.
 */
export function join({meeting_uuid, rtms_stream_id, server_urls, ca="ca.pem", timeout=-1, signature='', client='', secret='', }: JoinParams): void {
  const caCert = process.env['ZM_RTMS_CA'] || ca;

  if (!rtms._isInit())
    rtms._init(caCert);
  
  const sign = signature || generateSignature({ client, secret, uuid: meeting_uuid, streamId: rtms_stream_id });
  rtms._join(meeting_uuid, rtms_stream_id, sign, server_urls, timeout);
}

/**
 * Enables or disables audio.
 * @param hasAudio - True to enable, false to disable.
 */
export function useAudio(hasAudio: boolean = true): void {
  rtms.useAudio(hasAudio);
}

/**
 * Enables or disables video.
 * @param hasVideo - True to enable, false to disable.
 */
export function useVideo(hasVideo: boolean = true): void {
  rtms.useVideo(hasVideo);
}

/**
 * Enables or disables transcript.
 * @param hasTranscript - True to enable, false to disable.
 */
export function useTranscript(hasTranscript: boolean = true): void {
  rtms.useTranscript(hasTranscript);
}

/**
 * Sets media types.
 * @param mediaTypes - Object containing media type flags.
 */
export function setMediaTypes({ audio, video = false, transcript = false }: MediaTypes): void {
  rtms.setMediaTypes(audio, video, transcript);
}

/**
 * Generates HMAC SHA256 signature.
 * @param params - Signature parameters.
 * @returns Generated signature.
 */
export function generateSignature({ client, secret, uuid, streamId }: SignatureParams): string {
  const clientId = process.env['ZM_RTMS_CLIENT'] || client;
  const clientSecret = process.env['ZM_RTMS_SECRET'] || secret;

  if (!clientId) throw new ReferenceError('Client ID cannot be blank');
  if (!clientSecret) throw new ReferenceError('Client Secret cannot be blank');

  return createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${streamId}`)
    .digest('hex');
}

/**
 * Sets up a webhook server to listen for RTMS events.
 * @param callback - Callback for incoming webhook events.
 */
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

/**
 * Sets the callback for join confirmation.
 * @param callback - Callback receiving the reason code.
 */
export function onJoinConfirm(callback: JoinConfirmCallback): void {
  rtms.onJoinConfirm(callback);
}

/**
 * Sets the callback for session updates.
 * @param callback - Callback receiving operation code and session info.
 */
export function onSessionUpdate(callback: SessionUpdateCallback): void {
  rtms.onSessionUpdate(callback);
}

/**
 * Sets the callback for user updates.
 * @param callback - Callback receiving operation code and participant info.
 */
export function onUserUpdate(callback: UserUpdateCallback): void {
  rtms.onUserUpdate(callback);
}

/**
 * Sets the callback for audio data.
 * @param callback - Callback receiving audio buffer, size, timestamp, and metadata.
 */
export function onAudioData(callback: MediaDataCallback): void {
  rtms.onAudioData(callback);
}

/**
 * Sets the callback for video data.
 * @param callback - Callback receiving video buffer, size, timestamp, and metadata.
 */
export function onVideoData(callback: MediaDataCallback): void {
  rtms.onVideoData(callback);
}

/**
 * Sets the callback for transcript data.
 * @param callback - Callback receiving transcript buffer, size, timestamp, and metadata.
 */
export function onTranscriptData(callback: MediaDataCallback): void {
  rtms.onTranscriptData(callback);
}

/**
 * Sets the callback for leave events.
 * @param callback - Callback receiving the reason code for leaving.
 */
export function onLeave(callback: LeaveCallback): void {
  rtms.onLeave(callback);
}

export default {
  join,
  useAudio,
  useVideo,
  useTranscript,
  setMediaTypes,
  generateSignature,
  onWebhookEvent,
  onJoinConfirm,
  onSessionUpdate,
  onUserUpdate,
  onAudioData,
  onVideoData,
  onTranscriptData,
  onLeave,
};
