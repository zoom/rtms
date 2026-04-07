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
enum class MEDIA_CONTENT_TYPE {
    UNDEFINED   = 0,
    RTP         = 1,
    RAW_AUDIO   = 2,
    RAW_VIDEO   = 3,
    FILE_STREAM = 4,
    TEXT        = 5,
};

// MEDIA_PAYLOAD_TYPE — codec identifiers (shared by audio and video)
enum class MEDIA_PAYLOAD_TYPE {
    UNDEFINED = 0,
    L16       = 1,
    G711      = 2,
    G722      = 3,
    OPUS      = 4,
    JPG       = 5,
    PNG       = 6,
    H264      = 7,
};

// AUDIO_SAMPLE_RATE
enum class AUDIO_SAMPLE_RATE {
    SR_8K  = 0,
    SR_16K = 1,
    SR_32K = 2,
    SR_48K = 3,
};

// AUDIO_CHANNEL
enum class AUDIO_CHANNEL {
    MONO   = 1,
    STEREO = 2,
};

// MEDIA_RESOLUTION — for video and deskshare
enum class MEDIA_RESOLUTION {
    SD  = 1,
    HD  = 2,
    FHD = 3,
    QHD = 4,
};

// MEDIA_DATA_OPTION — audio and video stream delivery modes
// Note: VIDEO_SINGLE_INDIVIDUAL_STREAM is also known as VIDEO_MIXED_SPEAKER_VIEW in older SDK docs.
enum class MEDIA_DATA_OPTION {
    UNDEFINED                      = 0,
    AUDIO_MIXED_STREAM             = 1,
    AUDIO_MULTI_STREAMS            = 2,
    VIDEO_SINGLE_ACTIVE_STREAM     = 3,
    VIDEO_SINGLE_INDIVIDUAL_STREAM = 4,
    VIDEO_MIXED_GALLERY_VIEW       = 5,
};

// MEDIA_DATA_TYPE — bitmask of media stream types
// SDK_ALL = 0x1<<5 = 32 (matches rtms_common.h sdk_type::SDK_ALL)
enum class MEDIA_DATA_TYPE {
    UNDEFINED  = 0,
    AUDIO      = 1,
    VIDEO      = 2,
    DESKSHARE  = 4,
    TRANSCRIPT = 8,
    CHAT       = 16,
    ALL        = 32,
};

// RTMS_SESSION_STATE
enum class RTMS_SESSION_STATE {
    INACTIVE   = 0,
    INITIALIZE = 1,
    STARTED    = 2,
    PAUSED     = 3,
    RESUMED    = 4,
    STOPPED    = 5,
};

// RTMS_STREAM_STATE
enum class RTMS_STREAM_STATE {
    INACTIVE    = 0,
    ACTIVE      = 1,
    INTERRUPTED = 2,
    TERMINATING = 3,
    TERMINATED  = 4,
    PAUSED      = 5,
    RESUMED     = 6,
};

// RTMS_EVENT_TYPE — standard meeting events for subscribeEvent
enum class RTMS_EVENT_TYPE {
        UNDEFINED                    = 0,
        FIRST_PACKET_TIMESTAMP       = 1,
        ACTIVE_SPEAKER_CHANGE        = 2,
        PARTICIPANT_JOIN             = 3,
        PARTICIPANT_LEAVE            = 4,
        SHARING_START                = 5,
        SHARING_STOP                 = 6,
        MEDIA_CONNECTION_INTERRUPTED = 7,
        PARTICIPANT_VIDEO_ON         = 8,
        PARTICIPANT_VIDEO_OFF        = 9,
        // ZCC voice events (kept here for backward compatibility)
        CONSUMER_ANSWERED            = 8,
        CONSUMER_END                 = 9,
        USER_ANSWERED                = 10,
        USER_END                     = 11,
        USER_HOLD                    = 12,
        USER_UNHOLD                  = 13,
};


// RTMS_ZCC_VOICE_EVENT_TYPE — Zoom Contact Center voice events
enum class RTMS_ZCC_VOICE_EVENT_TYPE {
    UNDEFINED                      = 0,
    CONSUMER_ANSWERED              = 8,
    CONSUMER_END                   = 9,
    USER_ANSWERED                  = 10,
    USER_END                       = 11,
    USER_HOLD                      = 12,
    USER_UNHOLD                    = 13,
    MONITOR_STARTED                = 14,
    MONITOR_TRANSITIONED           = 15,
    MONITOR_ENDED                  = 16,
    TAKEOVER_STARTED               = 17,
    TRANSFER_INITIATED             = 18,
    TRANSFER_CANCELED              = 19,
    TRANSFER_ACCEPTED              = 20,
    TRANSFER_COMPLETED             = 21,
    TRANSFER_REJECTED              = 22,
    TRANSFER_TIMEOUT               = 23,
    CONFERENCE_CANCELED            = 24,
    CONFERENCE_PARTICIPANT_CANCELED = 25,
    CONFERENCE_PARTICIPANT_INVITED = 26,
    CONFERENCE_PARTICIPANT_REJECTED = 27,
    CONFERENCE_PARTICIPANT_TIMEOUT = 28,
};

// RTMS_MESSAGE_TYPE — WebSocket protocol message types
enum class RTMS_MESSAGE_TYPE {
    UNDEFINED                = 0,
    SIGNALING_HAND_SHAKE_REQ = 1,
    SIGNALING_HAND_SHAKE_RESP = 2,
    DATA_HAND_SHAKE_REQ      = 3,
    DATA_HAND_SHAKE_RESP     = 4,
    EVENT_SUBSCRIPTION       = 5,
    EVENT_UPDATE             = 6,
    CLIENT_READY_ACK         = 7,
    STREAM_STATE_UPDATE      = 8,
    SESSION_STATE_UPDATE     = 9,
    SESSION_STATE_REQ        = 10,
    SESSION_STATE_RESP       = 11,
    KEEP_ALIVE_REQ           = 12,
    KEEP_ALIVE_RESP          = 13,
    MEDIA_DATA_AUDIO         = 14,
    MEDIA_DATA_VIDEO         = 15,
    MEDIA_DATA_SHARE         = 16,
    MEDIA_DATA_TRANSCRIPT    = 17,
    MEDIA_DATA_CHAT          = 18,
    STREAM_STATE_REQ         = 19,
    STREAM_STATE_RESP        = 20,
    STREAM_CLOSE_REQ         = 21,
    STREAM_CLOSE_RESP        = 22,
    META_DATA_AUDIO          = 23,
    META_DATA_VIDEO          = 24,
    META_DATA_SHARE          = 25,
    META_DATA_TRANSCRIPT     = 26,
    META_DATA_CHAT           = 27,
    VIDEO_SUBSCRIPTION_REQ   = 28,
    VIDEO_SUBSCRIPTION_RESP  = 29,
};

// RTMS_STOP_REASON — reasons for stream termination
enum class RTMS_STOP_REASON {
    UNDEFINED                              = 0,
    HOST_TRIGGERED                         = 1,
    USER_TRIGGERED                         = 2,
    USER_LEFT                              = 3,
    USER_EJECTED                           = 4,
    HOST_DISABLED_APP                      = 5,
    MEETING_ENDED                          = 6,
    STREAM_CANCELED                        = 7,
    STREAM_REVOKED                         = 8,
    ALL_APPS_DISABLED                      = 9,
    INTERNAL_EXCEPTION                     = 10,
    CONNECTION_TIMEOUT                     = 11,
    INSTANCE_CONNECTION_INTERRUPTED        = 12,
    SIGNAL_CONNECTION_INTERRUPTED          = 13,
    DATA_CONNECTION_INTERRUPTED            = 14,
    SIGNAL_CONNECTION_CLOSED_ABNORMALLY    = 15,
    DATA_CONNECTION_CLOSED_ABNORMALLY      = 16,
    EXIT_SIGNAL                            = 17,
    AUTHENTICATION_FAILURE                 = 18,
    AWAIT_RECONNECTION_TIMEOUT             = 19,
    RECEIVER_REQUEST_CLOSE                 = 20,
    CUSTOMER_DISCONNECTED                  = 21,
    AGENT_DISCONNECTED                     = 22,
    ADMIN_DISABLED_APP                     = 23,
    KEEP_ALIVE_TIMEOUT                     = 24,
    MANUAL_API_TRIGGERED                   = 25,
    STREAMING_NOT_SUPPORTED                = 26,
};

// RTMS_TRANSCRIPT_LANGUAGE — source language IDs for transcript configuration.
// Use NONE (-1) to enable automatic language detection (LID).
enum class RTMS_TRANSCRIPT_LANGUAGE {
    NONE                = -1,
    ARABIC              = 0,
    BENGALI             = 1,
    CANTONESE           = 2,
    CATALAN             = 3,
    CHINESE_SIMPLIFIED  = 4,
    CHINESE_TRADITIONAL = 5,
    CZECH               = 6,
    DANISH              = 7,
    DUTCH               = 8,
    ENGLISH             = 9,
    ESTONIAN            = 10,
    FINNISH             = 11,
    FRENCH_CANADA       = 12,
    FRENCH_FRANCE       = 13,
    GERMAN              = 14,
    HEBREW              = 15,
    HINDI               = 16,
    HUNGARIAN           = 17,
    INDONESIAN          = 18,
    ITALIAN             = 19,
    JAPANESE            = 20,
    KOREAN              = 21,
    MALAY               = 22,
    PERSIAN             = 23,
    POLISH              = 24,
    PORTUGUESE          = 25,
    ROMANIAN            = 26,
    RUSSIAN             = 27,
    SPANISH             = 28,
    SWEDISH             = 29,
    TAGALOG             = 30,
    TAMIL               = 31,
    TELUGU              = 32,
    THAI                = 33,
    TURKISH             = 34,
    UKRAINIAN           = 35,
    VIETNAMESE          = 36,
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
        AUDIO      = 1,
        VIDEO      = 2,
        DESKSHARE  = 4,
        TRANSCRIPT = 8,
        CHAT       = 16,
        ALL        = 32,
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
