import {createHmac} from 'crypto'
import {createServer} from 'http'

import { createRequire } from "module";
const req = createRequire(import.meta.url);

const rtms = req('./rtms_sdk.node');

const port = process.env['ZOOM_WEBHOOK_PORT'] || 8080;
const path = process.env['ZOOM_WEBHOOK_PATH'] || '/'

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

    
export default {
    generateSignature,
    onWebhookEvent,
    ...rtms
}
