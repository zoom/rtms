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
            }
            return *this;
        }

        void setDeskshareParams(const DeskshareParams& ds_params);
        void setAudioParams(const AudioParams& audio_params);
        void setVideoParams(const VideoParams& video_params);

        const DeskshareParams& deskshareParams() const;
        const AudioParams& audioParams() const;
        const VideoParams& videoParams() const;

        bool hasDeskshareParams() const;
        bool hasAudioParams() const;
        bool hasVideoParams() const;

        media_parameters toNative() const;

    private:
        std::unique_ptr<DeskshareParams> ds_params_;
        std::unique_ptr<AudioParams> audio_params_;
        std::unique_ptr<VideoParams> video_params_;
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

    // Media type constants
    enum MediaType {
        AUDIO = 1,
        VIDEO = 2,
        DESKSHARE = 4,
        TRANSCRIPT = 8,
        CHAT = 16,
        ALL = 31  // Sum of all types (1+2+4+8+16)
    };

    // Event types for subscribeEvent/unsubscribeEvent
    // These match the RTMS_EVENT_TYPE enum from Zoom's C++ SDK
    // Used with on_event_ex callback (JSON events)
    enum EventType {
        EVENT_UNDEFINED = 0,
        EVENT_FIRST_PACKET_TIMESTAMP = 1,
        EVENT_ACTIVE_SPEAKER_CHANGE = 2,
        EVENT_PARTICIPANT_JOIN = 3,
        EVENT_PARTICIPANT_LEAVE = 4,
        EVENT_SHARING_START = 5,
        EVENT_SHARING_STOP = 6,
        EVENT_MEDIA_CONNECTION_INTERRUPTED = 7,
        EVENT_CONSUMER_ANSWERED = 8,
        EVENT_CONSUMER_END = 9,
        EVENT_USER_ANSWERED = 10,
        EVENT_USER_END = 11,
        EVENT_USER_HOLD = 12,
        EVENT_USER_UNHOLD = 13
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
