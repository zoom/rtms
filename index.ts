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
const rtms = req('../../build/Release/rtms.node');



export function generateSignature({ client, secret, uuid, streamId }: SignatureParams): string {
  const clientId = process.env['ZM_RTMS_CLIENT'] || client;
  const clientSecret = process.env['ZM_RTMS_SECRET'] || secret;

  if (!clientId) throw new ReferenceError('Client ID cannot be blank');
  if (!clientSecret) throw new ReferenceError('Client Secret cannot be blank');

  return createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${streamId}`)
    .digest('hex');
}



export default {
  ...rtms,
  generateSignature
};
