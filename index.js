import rtms from './rtms.cjs'

export const generateSignature = (clientId, clientSecret, uuid, sessionId) =>
    crypto
    .createHmac('sha256', clientSecret)
    .update(`${clientId},${uuid},${sessionId}`)
    .digest('hex')

export default {
    generateSignature,
    ...rtms
}
