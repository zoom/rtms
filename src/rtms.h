#ifndef RTMS_H
#define RTMS_H

#include "rtms_sdk.h"
#include <functional>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>

using namespace std;
namespace rtms {

class Exception : public runtime_error {
public:
    Exception(int error_code, const string& message);

    int code() const noexcept;

private:
    int error_code_;
};

class Session {
public:
    explicit Session(const session_info& info);
    string sessionId() const;
    string streamId() const;
    string meetingId() const;

    int statTime() const;
    int status() const;
    bool isActive() const;
    bool isPaused() const;

private:
    string session_id_;
    string stream_id_;
    string meeting_id_;
    int stat_time_;
    int status_;
};

class Participant {
public:
    explicit Participant(const participant_info& info);

    int id() const;
    string name() const;

private:
    int id_;
    string name_;
};

class Metadata {
public:
    explicit Metadata(const rtms_metadata& metadata);

    string userName() const;
    int userId() const;

private:
    string user_name_;
    int user_id_;
};

class BaseMediaParams {
public:
    BaseMediaParams();
    virtual ~BaseMediaParams() = default;

    void setContentType(int content_type);
    void setCodec(int codec);
    void setDataOpt(int data_opt);
    int contentType() const;
    int codec() const;
    int dataOpt() const;

protected:
    int content_type_;
    int codec_;
    int data_opt_;
};

// ============================================================================
// Protocol-level enums — single source of truth for all bindings.
// Values sourced from https://developers.zoom.us/docs/rtms/data-types/
// and verified against rtms_common.h where applicable.
// ============================================================================

// MEDIA_CONTENT_TYPE — shared by audio, video, deskshare, transcript
enum MediaContentType {
    CONTENT_TYPE_UNDEFINED   = 0,
    CONTENT_TYPE_RTP         = 1,
    CONTENT_TYPE_RAW_AUDIO   = 2,
    CONTENT_TYPE_RAW_VIDEO   = 3,
    CONTENT_TYPE_FILE_STREAM = 4,
    CONTENT_TYPE_TEXT        = 5,
};

// MEDIA_PAYLOAD_TYPE — codec identifiers (shared by audio and video)
enum MediaPayloadType {
    CODEC_UNDEFINED = 0,
    CODEC_L16       = 1,
    CODEC_G711      = 2,
    CODEC_G722      = 3,
    CODEC_OPUS      = 4,
    CODEC_JPG       = 5,
    CODEC_PNG       = 6,
    CODEC_H264      = 7,
};

// AUDIO_SAMPLE_RATE
enum AudioSampleRateEnum {
    SAMPLE_RATE_8K  = 0,
    SAMPLE_RATE_16K = 1,
    SAMPLE_RATE_32K = 2,
    SAMPLE_RATE_48K = 3,
};

// AUDIO_CHANNEL
enum AudioChannelEnum {
    CHANNEL_MONO   = 1,
    CHANNEL_STEREO = 2,
};

// MEDIA_RESOLUTION — for video and deskshare
enum MediaResolutionEnum {
    RESOLUTION_SD  = 1,
    RESOLUTION_HD  = 2,
    RESOLUTION_FHD = 3,
    RESOLUTION_QHD = 4,
};

// MEDIA_DATA_OPTION — audio and video stream delivery modes
// Note: VIDEO_MIXED_SPEAKER_VIEW and VIDEO_MIXED_GALLERY_VIEW are SDK-specific
// extensions not in the public MEDIA_DATA_OPTION docs but supported by the SDK.
enum MediaDataOptionEnum {
    DATA_OPT_UNDEFINED                      = 0,
    DATA_OPT_AUDIO_MIXED_STREAM             = 1,
    DATA_OPT_AUDIO_MULTI_STREAMS            = 2,
    DATA_OPT_VIDEO_SINGLE_ACTIVE_STREAM     = 3,
    DATA_OPT_VIDEO_SINGLE_INDIVIDUAL_STREAM = 4,  // also known as VIDEO_MIXED_SPEAKER_VIEW
    DATA_OPT_VIDEO_MIXED_GALLERY_VIEW       = 5,
};

// MEDIA_DATA_TYPE — bitmask of media stream types
// SDK_ALL = 0x1<<5 = 32 (matches rtms_common.h sdk_type::SDK_ALL)
enum MediaDataTypeEnum {
    MEDIA_DATA_UNDEFINED  = 0,
    MEDIA_DATA_AUDIO      = 1,
    MEDIA_DATA_VIDEO      = 2,
    MEDIA_DATA_DESKSHARE  = 4,
    MEDIA_DATA_TRANSCRIPT = 8,
    MEDIA_DATA_CHAT       = 16,
    MEDIA_DATA_ALL        = 32,
};

// RTMS_SESSION_STATE
enum SessionStateEnum {
    SESSION_STATE_INACTIVE   = 0,
    SESSION_STATE_INITIALIZE = 1,
    SESSION_STATE_STARTED    = 2,
    SESSION_STATE_PAUSED     = 3,
    SESSION_STATE_RESUMED    = 4,
    SESSION_STATE_STOPPED    = 5,
};

// RTMS_STREAM_STATE
enum StreamStateEnum {
    STREAM_STATE_INACTIVE    = 0,
    STREAM_STATE_ACTIVE      = 1,
    STREAM_STATE_INTERRUPTED = 2,
    STREAM_STATE_TERMINATING = 3,
    STREAM_STATE_TERMINATED  = 4,
    STREAM_STATE_PAUSED      = 5,
    STREAM_STATE_RESUMED     = 6,
};

// RTMS_EVENT_TYPE — standard meeting events for subscribeEvent
enum RtmsEventType {
    RTMS_EVENT_UNDEFINED                    = 0,
    RTMS_EVENT_FIRST_PACKET_TIMESTAMP       = 1,
    RTMS_EVENT_ACTIVE_SPEAKER_CHANGE        = 2,
    RTMS_EVENT_PARTICIPANT_JOIN             = 3,
    RTMS_EVENT_PARTICIPANT_LEAVE            = 4,
    RTMS_EVENT_SHARING_START               = 5,
    RTMS_EVENT_SHARING_STOP                = 6,
    RTMS_EVENT_MEDIA_CONNECTION_INTERRUPTED = 7,
    RTMS_EVENT_PARTICIPANT_VIDEO_ON         = 8,
    RTMS_EVENT_PARTICIPANT_VIDEO_OFF        = 9,
};

// RTMS_ZCC_VOICE_EVENT_TYPE — Zoom Contact Center voice events
enum ZccVoiceEventType {
    ZCC_EVENT_UNDEFINED                      = 0,
    ZCC_EVENT_CONSUMER_ANSWERED              = 8,
    ZCC_EVENT_CONSUMER_END                   = 9,
    ZCC_EVENT_USER_ANSWERED                  = 10,
    ZCC_EVENT_USER_END                       = 11,
    ZCC_EVENT_USER_HOLD                      = 12,
    ZCC_EVENT_USER_UNHOLD                    = 13,
    ZCC_EVENT_MONITOR_STARTED                = 14,
    ZCC_EVENT_MONITOR_TRANSITIONED           = 15,
    ZCC_EVENT_MONITOR_ENDED                  = 16,
    ZCC_EVENT_TAKEOVER_STARTED               = 17,
    ZCC_EVENT_TRANSFER_INITIATED             = 18,
    ZCC_EVENT_TRANSFER_CANCELED              = 19,
    ZCC_EVENT_TRANSFER_ACCEPTED              = 20,
    ZCC_EVENT_TRANSFER_COMPLETED             = 21,
    ZCC_EVENT_TRANSFER_REJECTED              = 22,
    ZCC_EVENT_TRANSFER_TIMEOUT               = 23,
    ZCC_EVENT_CONFERENCE_CANCELED            = 24,
    ZCC_EVENT_CONFERENCE_PARTICIPANT_CANCELED = 25,
    ZCC_EVENT_CONFERENCE_PARTICIPANT_INVITED = 26,
    ZCC_EVENT_CONFERENCE_PARTICIPANT_REJECTED = 27,
    ZCC_EVENT_CONFERENCE_PARTICIPANT_TIMEOUT = 28,
};

// RTMS_MESSAGE_TYPE — WebSocket protocol message types
enum MessageTypeEnum {
    MSG_UNDEFINED                = 0,
    MSG_SIGNALING_HAND_SHAKE_REQ = 1,
    MSG_SIGNALING_HAND_SHAKE_RESP = 2,
    MSG_DATA_HAND_SHAKE_REQ      = 3,
    MSG_DATA_HAND_SHAKE_RESP     = 4,
    MSG_EVENT_SUBSCRIPTION       = 5,
    MSG_EVENT_UPDATE             = 6,
    MSG_CLIENT_READY_ACK         = 7,
    MSG_STREAM_STATE_UPDATE      = 8,
    MSG_SESSION_STATE_UPDATE     = 9,
    MSG_SESSION_STATE_REQ        = 10,
    MSG_SESSION_STATE_RESP       = 11,
    MSG_KEEP_ALIVE_REQ           = 12,
    MSG_KEEP_ALIVE_RESP          = 13,
    MSG_MEDIA_DATA_AUDIO         = 14,
    MSG_MEDIA_DATA_VIDEO         = 15,
    MSG_MEDIA_DATA_SHARE         = 16,
    MSG_MEDIA_DATA_TRANSCRIPT    = 17,
    MSG_MEDIA_DATA_CHAT          = 18,
    MSG_STREAM_STATE_REQ         = 19,
    MSG_STREAM_STATE_RESP        = 20,
    MSG_STREAM_CLOSE_REQ         = 21,
    MSG_STREAM_CLOSE_RESP        = 22,
    MSG_META_DATA_AUDIO          = 23,
    MSG_META_DATA_VIDEO          = 24,
    MSG_META_DATA_SHARE          = 25,
    MSG_META_DATA_TRANSCRIPT     = 26,
    MSG_META_DATA_CHAT           = 27,
    MSG_VIDEO_SUBSCRIPTION_REQ   = 28,
    MSG_VIDEO_SUBSCRIPTION_RESP  = 29,
};

// RTMS_STOP_REASON — reasons for stream termination
enum StopReasonEnum {
    STOP_UNDEFINED                              = 0,
    STOP_BC_HOST_TRIGGERED                      = 1,
    STOP_BC_USER_TRIGGERED                      = 2,
    STOP_BC_USER_LEFT                           = 3,
    STOP_BC_USER_EJECTED                        = 4,
    STOP_BC_HOST_DISABLED_APP                   = 5,
    STOP_BC_MEETING_ENDED                       = 6,
    STOP_BC_STREAM_CANCELED                     = 7,
    STOP_BC_STREAM_REVOKED                      = 8,
    STOP_BC_ALL_APPS_DISABLED                   = 9,
    STOP_BC_INTERNAL_EXCEPTION                  = 10,
    STOP_BC_CONNECTION_TIMEOUT                  = 11,
    STOP_BC_INSTANCE_CONNECTION_INTERRUPTED     = 12,
    STOP_BC_SIGNAL_CONNECTION_INTERRUPTED       = 13,
    STOP_BC_DATA_CONNECTION_INTERRUPTED         = 14,
    STOP_BC_SIGNAL_CONNECTION_CLOSED_ABNORMALLY = 15,
    STOP_BC_DATA_CONNECTION_CLOSED_ABNORMALLY   = 16,
    STOP_BC_EXIT_SIGNAL                         = 17,
    STOP_BC_AUTHENTICATION_FAILURE              = 18,
    STOP_BC_AWAIT_RECONNECTION_TIMEOUT          = 19,
    STOP_BC_RECEIVER_REQUEST_CLOSE              = 20,
    STOP_BC_CUSTOMER_DISCONNECTED               = 21,
    STOP_BC_AGENT_DISCONNECTED                  = 22,
    STOP_BC_ADMIN_DISABLED_APP                  = 23,
    STOP_BC_KEEP_ALIVE_TIMEOUT                  = 24,
    STOP_BC_MANUAL_API_TRIGGERED                = 25,
    STOP_BC_STREAMING_NOT_SUPPORTED             = 26,
};

// RTMS_TRANSCRIPT_LANGUAGE — source language IDs for transcript configuration.
// Use LANGUAGE_ID_NONE (-1) to enable automatic language detection (LID).
enum TranscriptLanguageId {
    LANGUAGE_ID_NONE                = -1,
    LANGUAGE_ID_ARABIC              = 0,
    LANGUAGE_ID_BENGALI             = 1,
    LANGUAGE_ID_CANTONESE           = 2,
    LANGUAGE_ID_CATALAN             = 3,
    LANGUAGE_ID_CHINESE_SIMPLIFIED  = 4,
    LANGUAGE_ID_CHINESE_TRADITIONAL = 5,
    LANGUAGE_ID_CZECH               = 6,
    LANGUAGE_ID_DANISH              = 7,
    LANGUAGE_ID_DUTCH               = 8,
    LANGUAGE_ID_ENGLISH             = 9,
    LANGUAGE_ID_ESTONIAN            = 10,
    LANGUAGE_ID_FINNISH             = 11,
    LANGUAGE_ID_FRENCH_CANADA       = 12,
    LANGUAGE_ID_FRENCH_FRANCE       = 13,
    LANGUAGE_ID_GERMAN              = 14,
    LANGUAGE_ID_HEBREW              = 15,
    LANGUAGE_ID_HINDI               = 16,
    LANGUAGE_ID_HUNGARIAN           = 17,
    LANGUAGE_ID_INDONESIAN          = 18,
    LANGUAGE_ID_ITALIAN             = 19,
    LANGUAGE_ID_JAPANESE            = 20,
    LANGUAGE_ID_KOREAN              = 21,
    LANGUAGE_ID_MALAY               = 22,
    LANGUAGE_ID_PERSIAN             = 23,
    LANGUAGE_ID_POLISH              = 24,
    LANGUAGE_ID_PORTUGUESE          = 25,
    LANGUAGE_ID_ROMANIAN            = 26,
    LANGUAGE_ID_RUSSIAN             = 27,
    LANGUAGE_ID_SPANISH             = 28,
    LANGUAGE_ID_SWEDISH             = 29,
    LANGUAGE_ID_TAGALOG             = 30,
    LANGUAGE_ID_TAMIL               = 31,
    LANGUAGE_ID_TELUGU              = 32,
    LANGUAGE_ID_THAI                = 33,
    LANGUAGE_ID_TURKISH             = 34,
    LANGUAGE_ID_UKRAINIAN           = 35,
    LANGUAGE_ID_VIETNAMESE          = 36,
};

class TranscriptParams : public BaseMediaParams {
public:
    TranscriptParams();

    void setSrcLanguage(int src_language);
    void setEnableLid(bool enable_lid);
    int srcLanguage() const;
    bool enableLid() const;

    transcript_parameters toNative() const;

private:
    int src_language_;
    bool enable_lid_;
};

class DeskshareParams : public BaseMediaParams {
public:
        DeskshareParams();
        DeskshareParams(int content_type, int codec, int resolution, int fps);
        void setResolution(int resolution);
        void setFps(int fps);
        int resolution() const;
        int fps() const;

        /**
         * Validate deskshare parameters for consistency
         * Throws std::invalid_argument if parameters are invalid or incompatible
         */
        void validate() const;

        ds_parameters toNative() const;

private:
        int resolution_;
        int fps_;
};


class AudioParams : public BaseMediaParams {
public:
    /**
     * Default constructor with sensible defaults:
     * - contentType: RAW_AUDIO (2)
     * - codec: OPUS (4)
     * - sampleRate: SR_48K (3)
     * - channel: STEREO (2)
     * - dataOpt: AUDIO_MULTI_STREAMS (2) - enables per-participant audio with userId
     * - duration: 20 ms
     * - frameSize: 960 samples (48kHz × 20ms)
     */
    AudioParams();
    AudioParams(int content_type, int codec, int sample_rate, int channel, int data_opt, int duration, int frame_size);

    void setSampleRate(int sample_rate);
    void setChannel(int channel);
    void setDuration(int duration);
    void setFrameSize(int frame_size);
    int sampleRate() const;
    int channel() const;
    int duration() const;
    int frameSize() const;

    /**
     * Validate audio parameters for consistency
     * Throws std::invalid_argument if parameters are invalid or incompatible
     */
    void validate() const;

    audio_parameters toNative() const;

private:
    int sample_rate_;
    int channel_;
    int duration_;
    int frame_size_;
};

class VideoParams : public BaseMediaParams {
public:
    VideoParams();
    VideoParams(int content_type, int codec, int resolution, int data_opt, int fps);
    void setResolution(int resolution);
    void setFps(int fps);
    int resolution() const;
    int fps() const;

    /**
     * Validate video parameters for consistency
     * Throws std::invalid_argument if parameters are invalid or incompatible
     * - JPG/PNG codecs require fps <= 5
     * - H264 codec allows fps up to 30
     */
    void validate() const;

    video_parameters toNative() const;

private:
    int resolution_;
    int fps_;
};

class MediaParams {
    public:
        MediaParams();
        ~MediaParams();

        MediaParams& operator=(const MediaParams& other) {
            if (this != &other) {
                if (other.hasAudioParams()) {
                    audio_params_ = std::make_unique<AudioParams>(other.audioParams());
                } else {
                    audio_params_.reset();
                }

                if (other.hasVideoParams()) {
                    video_params_ = std::make_unique<VideoParams>(other.videoParams());
                } else {
                    video_params_.reset();
                }

                if (other.hasDeskshareParams()) {
                    ds_params_ = std::make_unique<DeskshareParams>(other.deskshareParams());
                } else {
                    ds_params_.reset();
                }

                if (other.hasTranscriptParams()) {
                    transcript_params_ = std::make_unique<TranscriptParams>(other.transcriptParams());
                } else {
                    transcript_params_.reset();
                }
            }
            return *this;
        }

        void setDeskshareParams(const DeskshareParams& ds_params);
        void setAudioParams(const AudioParams& audio_params);
        void setVideoParams(const VideoParams& video_params);
        void setTranscriptParams(const TranscriptParams& transcript_params);

        const DeskshareParams& deskshareParams() const;
        const AudioParams& audioParams() const;
        const VideoParams& videoParams() const;
        const TranscriptParams& transcriptParams() const;

        bool hasDeskshareParams() const;
        bool hasAudioParams() const;
        bool hasVideoParams() const;
        bool hasTranscriptParams() const;

        media_parameters toNative() const;

    private:
        std::unique_ptr<DeskshareParams> ds_params_;
        std::unique_ptr<AudioParams> audio_params_;
        std::unique_ptr<VideoParams> video_params_;
        std::unique_ptr<TranscriptParams> transcript_params_;
    };

class Client : public rtms_sdk_sink {

public:
    using JoinConfirmFn = function<void(int)>;
    using SessionUpdateFn = function<void(int, const Session&)>;
    using UserUpdateFn = function<void(int, const Participant&)>;
    using DsDataFn = function<void(const vector<uint8_t>&, uint64_t,  const Metadata&)>;
    using AudioDataFn = function<void(const vector<uint8_t>&, uint64_t, const Metadata&)>;
    using VideoDataFn = function<void(const vector<uint8_t>&, uint64_t,  const Metadata&)>;
    using TranscriptDataFn = function<void(const vector<uint8_t>&, uint64_t, const Metadata&)>;
    using LeaveFn = function<void(int)>;
    using EventExFn = function<void(const string&)>;
    using ParticipantVideoFn = function<void(const vector<int>&, bool)>;
    using VideoSubscribedFn = function<void(int, int, const string&)>;

    // Media type bitmask constants (matches SDK media_type enum in rtms_common.h)
    // ALL = SDK_ALL = 0x1<<5 = 32
    enum MediaType {
        AUDIO      = MEDIA_DATA_AUDIO,
        VIDEO      = MEDIA_DATA_VIDEO,
        DESKSHARE  = MEDIA_DATA_DESKSHARE,
        TRANSCRIPT = MEDIA_DATA_TRANSCRIPT,
        CHAT       = MEDIA_DATA_CHAT,
        ALL        = MEDIA_DATA_ALL,
    };

    // RTMS event types (RTMS_EVENT_TYPE) for subscribeEvent
    // Values 0–7 are standard meeting events; 8–9 are video presence events.
    // ZCC voice events (RTMS_ZCC_VOICE_EVENT_TYPE) use ZccVoiceEventType above.
    enum EventType {
        EVENT_UNDEFINED                    = RTMS_EVENT_UNDEFINED,
        EVENT_FIRST_PACKET_TIMESTAMP       = RTMS_EVENT_FIRST_PACKET_TIMESTAMP,
        EVENT_ACTIVE_SPEAKER_CHANGE        = RTMS_EVENT_ACTIVE_SPEAKER_CHANGE,
        EVENT_PARTICIPANT_JOIN             = RTMS_EVENT_PARTICIPANT_JOIN,
        EVENT_PARTICIPANT_LEAVE            = RTMS_EVENT_PARTICIPANT_LEAVE,
        EVENT_SHARING_START                = RTMS_EVENT_SHARING_START,
        EVENT_SHARING_STOP                 = RTMS_EVENT_SHARING_STOP,
        EVENT_MEDIA_CONNECTION_INTERRUPTED = RTMS_EVENT_MEDIA_CONNECTION_INTERRUPTED,
        EVENT_PARTICIPANT_VIDEO_ON         = RTMS_EVENT_PARTICIPANT_VIDEO_ON,
        EVENT_PARTICIPANT_VIDEO_OFF        = RTMS_EVENT_PARTICIPANT_VIDEO_OFF,
        // ZCC voice events (kept here for backward compatibility)
        EVENT_CONSUMER_ANSWERED            = ZCC_EVENT_CONSUMER_ANSWERED,
        EVENT_CONSUMER_END                 = ZCC_EVENT_CONSUMER_END,
        EVENT_USER_ANSWERED                = ZCC_EVENT_USER_ANSWERED,
        EVENT_USER_END                     = ZCC_EVENT_USER_END,
        EVENT_USER_HOLD                    = ZCC_EVENT_USER_HOLD,
        EVENT_USER_UNHOLD                  = ZCC_EVENT_USER_UNHOLD,
    };

    Client();
    ~Client();

    static void initialize(const string& ca, int is_verify_cert = 1, const char* agent = nullptr);
    static void uninitialize();
    void configure(const MediaParams& params, int media_types, bool enable_application_layer_encryption = false, bool apply_defaults = true);

    void enableVideo(bool enable);
    void enableAudio(bool enable);
    void enableTranscript(bool enable);
    void enableDeskshare(bool enable);

    void setOnJoinConfirm(JoinConfirmFn callback);
    void setOnSessionUpdate(SessionUpdateFn callback);
    void setOnUserUpdate(UserUpdateFn callback);
    void setOnDeskshareData(DsDataFn callback);
    void setOnAudioData(AudioDataFn callback);
    void setOnVideoData(VideoDataFn callback);
    void setOnTranscriptData(TranscriptDataFn callback);
    void setOnLeave(LeaveFn callback);
    void setOnEventEx(EventExFn callback);

    void subscribeEvent(const std::vector<int>& events);
    void unsubscribeEvent(const std::vector<int>& events);

    void setDeskshareParams(const DeskshareParams& ds_params);
    void setVideoParams(const VideoParams& video_params);
    void setAudioParams(const AudioParams& audio_params);
    void setTranscriptParams(const TranscriptParams& transcript_params);
    void setProxy(const string& proxy_type, const string& proxy_url);
    void subscribeVideo(int user_id, bool subscribe);

    void setOnParticipantVideo(ParticipantVideoFn callback);
    void setOnVideoSubscribed(VideoSubscribedFn callback);

    void join(const string& meeting_uuid, const string& rtms_stream_id, const string& signature, const string& server_url, int timeout = -1);

    void poll();
    void release();

    string uuid() const;
    string streamId() const;

    // rtms_sdk_sink overrides — called by the SDK from within poll()
    void on_join_confirm(int reason) override;
    void on_session_update(int op, struct session_info* sess) override;
    void on_user_update(int op, struct participant_info* pi) override;
    void on_audio_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) override;
    void on_video_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) override;
    void on_ds_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) override;
    void on_transcript_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) override;
    void on_leave(int reason) override;
    void on_event_ex(const std::string& compact_str) override;
    void on_participant_video(std::vector<int> users, bool is_on) override;
    void on_video_subscript_resp(int user_id, int status, std::string error) override;

private:
    mutable mutex mutex_;
    rtms_sdk* sdk_;

    string meeting_uuid_;
    string rtms_stream_id_;

    int enabled_media_types_;
    bool media_params_updated_;
    MediaParams media_params_;

    JoinConfirmFn join_confirm_callback_;
    SessionUpdateFn session_update_callback_;
    UserUpdateFn user_update_callback_;
    DsDataFn ds_data_callback_;
    AudioDataFn audio_data_callback_;
    VideoDataFn video_data_callback_;
    TranscriptDataFn transcript_data_callback_;
    LeaveFn leave_callback_;
    EventExFn event_ex_callback_;
    ParticipantVideoFn participant_video_callback_;
    VideoSubscribedFn video_subscribed_callback_;

    std::vector<int> subscribed_events_;

    // Event subscription timing: subscriptions must be deferred until after join is confirmed
    bool join_confirmed_;
    std::vector<int> pending_event_subscriptions_;
    void processPendingSubscriptions();

    void throwIfError(int result, const std::string& operation) const;
    void updateMediaConfiguration(int mediaType, bool enable = true);
};
}

#endif // RTMS_H
