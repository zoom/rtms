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

export function join({meeting_uuid, rtms_stream_id, server_urls, ca="ca.pem", timeout=-1, signature='', client='', secret='', }: JoinParams): void {
  const caCert = process.env['ZM_RTMS_CA'] || ca;

  if (!rtms._isInit())
    rtms._init(caCert);
  
  const sign = signature || generateSignature({ client, secret, uuid: meeting_uuid, streamId: rtms_stream_id });
  rtms._join(meeting_uuid, rtms_stream_id, sign, server_urls, timeout);
}

export function useAudio(hasAudio: boolean = true): void {
  rtms.useAudio(hasAudio);
}

export function useVideo(hasVideo: boolean = true): void {
  rtms.useVideo(hasVideo);
}

export function useTranscript(hasTranscript: boolean = true): void {
  rtms.useTranscript(hasTranscript);
}

export function setMediaTypes({ audio, video = false, transcript = false }: MediaTypes): void {
  rtms.setMediaTypes(audio, video, transcript);
}

export function generateSignature({ client, secret, uuid, streamId }: SignatureParams): string {
  const clientId = process.env['ZM_RTMS_CLIENT'] || client;
  const clientSecret = process.env['ZM_RTMS_SECRET'] || secret;

  if (!clientId) throw new ReferenceError('Client ID cannot be blank');
  if (!clientSecret) throw new ReferenceError('Client Secret cannot be blank');

  return createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${streamId}`)
    .digest('hex');
}

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

export function onJoinConfirm(callback: JoinConfirmCallback): void {
  rtms.onJoinConfirm(callback);
}

export function onSessionUpdate(callback: SessionUpdateCallback): void {
  rtms.onSessionUpdate(callback);
}

export function onUserUpdate(callback: UserUpdateCallback): void {
  rtms.onUserUpdate(callback);
}

export function onAudioData(callback: MediaDataCallback): void {
  rtms.onAudioData(callback);
}

export function onVideoData(callback: MediaDataCallback): void {
  rtms.onVideoData(callback);
}

export function onTranscriptData(callback: MediaDataCallback): void {
  rtms.onTranscriptData(callback);
}

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
