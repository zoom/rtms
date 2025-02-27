// Type definitions for Zoom RTMS SDK

// Constants
export const MEDIA_TYPE_AUDIO: number;
export const MEDIA_TYPE_VIDEO: number;
export const MEDIA_TYPE_DESKSHARE: number;
export const MEDIA_TYPE_TRANSCRIPT: number;
export const MEDIA_TYPE_CHAT: number;
export const MEDIA_TYPE_ALL: number;

export const SESSION_EVENT_ADD: number;
export const SESSION_EVENT_STOP: number;
export const SESSION_EVENT_PAUSE: number;
export const SESSION_EVENT_RESUME: number;

export const USER_EVENT_JOIN: number;
export const USER_EVENT_LEAVE: number;

export const RTMS_SDK_FAILURE: number;
export const RTMS_SDK_OK: number;
export const RTMS_SDK_TIMEOUT: number;
export const RTMS_SDK_NOT_EXIST: number;
export const RTMS_SDK_WRONG_TYPE: number;
export const RTMS_SDK_INVALID_STATUS: number;
export const RTMS_SDK_INVALID_ARGS: number;

export const SESS_STATUS_ACTIVE: number;
export const SESS_STATUS_PAUSED: number;

// Interfaces for data types
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
  id: number;
  name: string;
}

export interface SessionInfo {
  sessionId: string;
  statTime: number;
  status: number;
  isActive: boolean;
  isPaused: boolean;
}

export interface AudioParameters {
  contentType?: number;
  codec?: number;
  sampleRate?: number;
  channel?: number;
  dataOpt?: number;
  duration?: number;
  frameSize?: number;
}

export interface VideoParameters {
  contentType?: number;
  codec?: number;
  resolution?: number;
  dataOpt?: number;
  fps?: number;
}

// Callback types
export type WebhookCallback = (payload: Record<string, any>) => void;
export type JoinConfirmCallback = (reason: number) => void;
export type SessionUpdateCallback = (op: number, sessionInfo: SessionInfo) => void;
export type UserUpdateCallback = (op: number, participantInfo: ParticipantInfo) => void;
export type AudioDataCallback = (buffer: Buffer, size: number, timestamp: number, metadata: Metadata) => void;
export type VideoDataCallback = (buffer: Buffer, size: number, timestamp: number, trackId: string, metadata: Metadata) => void;
export type TranscriptDataCallback = (buffer: Buffer, size: number, timestamp: number, metadata: Metadata) => void;
export type LeaveCallback = (reason: number) => void;

// Parameter interfaces
export interface JoinParams {
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

export interface SignatureParams {
  client: string;
  secret: string;
  uuid: string;
  streamId: string;
}

// Client class
export class Client {
  constructor();
  
  // Static methods
  static initialize(caPath?: string): boolean;
  static uninitialize(): boolean;
  
  // Instance methods
  join(options: JoinParams): boolean;
  join(meetingUuid: string, rtmsStreamId: string, signature: string, serverUrls: string, timeout?: number): boolean;
  
  configure(mediaTypes: number, enableEncryption?: boolean): boolean;
  poll(): boolean;
  release(): boolean;
  leave(): boolean;
  uuid(): string;
  streamId(): string;
  
  setAudioParameters(params: AudioParameters): boolean;
  setVideoParameters(params: VideoParameters): boolean;
  
  onJoinConfirm(callback: JoinConfirmCallback): boolean;
  onSessionUpdate(callback: SessionUpdateCallback): boolean;
  onUserUpdate(callback: UserUpdateCallback): boolean;
  onAudioData(callback: AudioDataCallback): boolean;
  onVideoData(callback: VideoDataCallback): boolean;
  onTranscriptData(callback: TranscriptDataCallback): boolean;
  onLeave(callback: LeaveCallback): boolean;
}

// Global singleton functions
export function join(options: JoinParams): boolean;
export function join(meetingUuid: string, rtmsStreamId: string, signature: string, serverUrls: string, timeout?: number): boolean;
export function leave(): boolean;
export function poll(): boolean;
export function uuid(): string;
export function streamId(): string;
export function onJoinConfirm(callback: JoinConfirmCallback): boolean;
export function onSessionUpdate(callback: SessionUpdateCallback): boolean;
export function onUserUpdate(callback: UserUpdateCallback): boolean;
export function onAudioData(callback: AudioDataCallback): boolean;
export function onVideoData(callback: VideoDataCallback): boolean;
export function onTranscriptData(callback: TranscriptDataCallback): boolean;
export function onLeave(callback: LeaveCallback): boolean;

// Module exports
export function initialize(caPath?: string): boolean;
export function uninitialize(): boolean;
export function generateSignature(params: SignatureParams): string;
export function isInitialized(): boolean;

// Default export
declare const rtms: {
  // Class-based API
  Client: typeof Client;
  
  // Global singleton API
  join: typeof join;
  leave: typeof leave;
  poll: typeof poll;
  uuid: typeof uuid;
  streamId: typeof streamId;
  onJoinConfirm: typeof onJoinConfirm;
  onSessionUpdate: typeof onSessionUpdate;
  onUserUpdate: typeof onUserUpdate;
  onAudioData: typeof onAudioData;
  onVideoData: typeof onVideoData;
  onTranscriptData: typeof onTranscriptData;
  onLeave: typeof onLeave;
  
  // Utility functions
  initialize: typeof initialize;
  uninitialize: typeof uninitialize;
  generateSignature: typeof generateSignature;
  isInitialized: typeof isInitialized;
  
  // Constants
  MEDIA_TYPE_AUDIO: number;
  MEDIA_TYPE_VIDEO: number;
  MEDIA_TYPE_DESKSHARE: number;
  MEDIA_TYPE_TRANSCRIPT: number;
  MEDIA_TYPE_CHAT: number;
  MEDIA_TYPE_ALL: number;
  
  SESSION_EVENT_ADD: number;
  SESSION_EVENT_STOP: number;
  SESSION_EVENT_PAUSE: number;
  SESSION_EVENT_RESUME: number;
  
  USER_EVENT_JOIN: number;
  USER_EVENT_LEAVE: number;
  
  RTMS_SDK_FAILURE: number;
  RTMS_SDK_OK: number;
  RTMS_SDK_TIMEOUT: number;
  RTMS_SDK_NOT_EXIST: number;
  RTMS_SDK_WRONG_TYPE: number;
  RTMS_SDK_INVALID_STATUS: number;
  RTMS_SDK_INVALID_ARGS: number;
  
  SESS_STATUS_ACTIVE: number;
  SESS_STATUS_PAUSED: number;
};

export default rtms;