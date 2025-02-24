import {createHmac} from 'crypto'
import {createServer} from 'http'

import { createRequire } from "module";
const req = createRequire(import.meta.url);
const rtms = req('./_rtms.node');

let server, port, path;

export const onWebhookEvent = (callback) => {

    if (server?.listening()) return;

    port = process.env['ZM_RTMS_PORT'] || 8080;
    path = process.env['ZM_RTMS_PATH'] || '/'

    server = createServer((req, res) => {
        if (req.method !== 'POST' || req.url !== path) {
            res.writeHead(404,  headers);
            res.end("Not Found")
            return
        }

        let body = '';
        const headers = {
            'Content-Type': 'text/plain'
        }
        

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

    server.listen(port, () =>
        console.log(`listening for events at http://localhost:${port}${path}`)
    )
}

export const generateSignature = ({client, secret, uuid, streamId}) => {
    const clientId = process.env['ZM_RTMS_CLIENT'] || client
    const clientSecret = process.env['ZM_RTMS_SECRET'] || secret

    if (!clientId)
        throw new ReferenceError("Client Id cannot be blank")

    if (!clientSecret)
        throw new ReferenceError("Client Secret cannot be blank")

    return createHmac('sha256', clientSecret)
        .update(`${clientId},${uuid},${streamId}`)
        .digest('hex')
}


export const join = ({meeting_uuid, rtms_stream_id, server_urls, timeout= -1, ca="ca.pem", client="", secret="", signature=""}) => {
    const caCert = process.env['ZM_RTMS_CA'] || ca

    if (!rtms._isInit())
        rtms._init(caCert)

    const sign = signature || generateSignature(client, secret, meeting_uuid, rtms_stream_id);
    rtms._join(meeting_uuid, rtms_stream_id, sign, server_urls, timeout)
}

export default {
    generateSignature,
    onWebhookEvent,
    join,
    ...rtms
}
