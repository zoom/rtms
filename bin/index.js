import {createHmac} from 'crypto'
import {createServer} from 'http'

import { createRequire } from "module";
const req = createRequire(import.meta.url);

const rtms = req('./rtms.node');

const port = process.env['ZM_RTMS_PORT'] || 8080;
const path = process.env['ZM_RTMS_PATH'] || '/'

const clientId = process.env['ZM_RTMS_CLIENT']
const clientSecret = process.env['ZM_RTMS_SECRET']
const caCert = process.env['ZM_RTMS_CA']

const headers = {
    'Content-Type': 'text/plain'
}

let server;

export const  onWebhookEvent = (callback) => {

    if (server?.listening()) return;

    server = createServer((req, res) => {
        if (req.method !== 'POST' || req.url !== path) {
            res.writeHead(404,  headers);
            res.end("Not Found")
            return
        }

        let body = '';

        req.on('data', chunk => {
            body += chunk.toString();
        })

        req.on('end', () => {
            try {
                const payload = JSON.parse(body);
                
                callback(payload);
                
                res.writeHead(200, headers)
                res.end()
            } catch (e) {
                console.error("parsing webhook JSON", e);
                res.writeHead(400, headers);
                res.end("invalid JSON received")

            }
        })
    })

    server.listen(port, () => {
        console.log(`listening for events at http://localhost:${port}${path}`)
    })
}

export const generateSignature = (clientId, clientSecret, uuid, sessionId) =>
    createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${sessionId}`)
    .digest('hex')


export const join = ({meeting_uuid, rtms_stream_id, server_url}, ca=caCert, client=clientId, secret=clientSecret) => {
    if (!rtms._isInit())
        rtms._init(ca || "ca.pem")

    if (!client)
        throw new ReferenceError("Zoom Client Id cannot be empty")

    if (!secret)
        throw new ReferenceError("Zooom Client Secret cannot be empty")

    const signature = generateSignature(client, secret, meeting_uuid, rtms_stream_id);

    rtms._join(meeting_uuid, rtms_stream_id, signature, server_url)
}

export default {
    generateSignature,
    onWebhookEvent,
    join,
    ...rtms
}
