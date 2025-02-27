export interface MediaTypes {
  audio: boolean;
  video?: boolean;
  transcript?: boolean;
}

export interface Metadata {
  userName: string;
  userId: number;
}

export interface ParticipantInfo {
  participantId: number;
  participantName: string;
}

export interface SessionInfo {
  sessionId: string;
  statTime: number;
  status: number;
}

export type WebhookCallback = (payload: object) => void;
export type JoinConfirmCallback = (reason: number) => void;
export type SessionUpdateCallback = (op: number, sessionInfo: SessionInfo) => void;
export type UserUpdateCallback = (op: number, participantInfo: ParticipantInfo) => void;
export type MediaDataCallback = (buffer: Buffer, size: number, timestamp: number, metadata: Metadata) => void;
export type LeaveCallback = (reason: number) => void;

export declare function join(params: JoinParams): void;
export declare function useAudio(hasAudio?: boolean): void;
export declare function useVideo(hasVideo?: boolean): void;
export declare function useTranscript(hasTranscript?: boolean): void;
export declare function setMediaTypes(mediaTypes: MediaTypes): void;
export declare function generateSignature(params: SignatureParams): string;
export declare function onWebhookEvent(callback: WebhookCallback): void;
export declare function onJoinConfirm(callback: JoinConfirmCallback): void;
export declare function onSessionUpdate(callback: SessionUpdateCallback): void;
export declare function onUserUpdate(callback: UserUpdateCallback): void;
export declare function onAudioData(callback: MediaDataCallback): void;
export declare function onVideoData(callback: MediaDataCallback): void;
export declare function onTranscriptData(callback: MediaDataCallback): void;
export declare function onLeave(callback: LeaveCallback): void;


interface JoinParams {
  meeting_uuid: string;
  rtms_stream_id: string;
  server_urls: string;
  signature?: string;
  client?: string;
  secret?: string;
  ca?: string;
  timeout?: number;
  pollInterval?: number;
}

interface SignatureParams {
  client: string;
  secret: string;
  uuid: string;
  streamId: string;
}

class Client extends nativeRtms.Client {
  join(options: JoinParams): boolean;
  join(meetingUuid: string, rtmsStreamId: string, signature: string, serverUrls: string, timeout?: number): boolean;
  join(optionsOrMeetingUuid: JoinParams | string, rtmsStreamId?: string, signature?: string, serverUrls?: string, timeout?: number): boolean;
  
  startPolling(): void;
  stopPolling(): void;
  leave(): boolean;
}