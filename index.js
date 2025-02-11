import rtms from './rtms.cjs'
import {createHmac} from 'crypto'
import { WebSocketServer } from 'ws';

const socketMap = new Map();
const wss = new WebSocketServer({ port: process.env.WS_PORT || 8080 });

export const generateSignature = (clientId, clientSecret, uuid, sessionId) =>
    createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${sessionId}`)
    .digest('hex')


export const join = (uuid, sessionId, signature, timeout=0) => {
    wss.on('connection', ws => {
        ws.on('message', message => {

            const expected = "SIGNALING_HAND_SHAKE_RESP";
            if (message.msg_type !== expected)
                ws.close(1002, `Expected ${expected} msg_type`)

            const code = message.status_code;
            if (!code) {
                const reason = message.reason
                ws.close(1002, `failed with status_code ${code}: ${reason}`)
            }

            socketMap.set(uuid, {
                ws,
                mediaServer: message.media_server
            });

            ws.send({
                msg_type: "CLIENT_READY_ACK",
                rtms_stream_id: sessionId
            })

            rtms._join(uuid, sessionId, signature, ws.url, timeout);

        });

        ws.on('close', () => socketMap.delete(uuid));

        ws.send({
            msg_type: "SIGNALING_HAND_SHAKE_REQ",
            protocol_version: 1,
            sequence: 0,
            meeting_uuid: uuid,
            rtms_stream_id: sessionId,
            signature: signature
        });
    });
}

export default {
    generateSignature,
    join,
    ...rtms
}
