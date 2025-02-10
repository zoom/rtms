import jwt from 'jsonwebtoken'
import rtms from './rtms.cjs'

export const generateSignature = (clientId, clientSecret, uuid, sessionId) =>
    jwt.sign(`${clientId},${uuid},${sessionId}`, clientSecret)


export default {
    generateSignature,
    ...rtms
}
