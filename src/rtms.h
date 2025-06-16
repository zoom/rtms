#ifndef RTMS_H
#define RTMS_H

#include "rtms_csdk.h"
#include <functional>
#include <sstream>
#include <thread>
#include <mutex>
#include <unordered_map>
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

    int statTime() const;
    int status() const;
    bool isActive() const;
    bool isPaused() const;

private:
    string session_id_;
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

        ds_parameters toNative() const;

private:
        int resolution_;
        int fps_;
};


class AudioParams : public BaseMediaParams {
public:
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

class Client {

public:
    using JoinConfirmFn = function<void(int)>;
    using SessionUpdateFn = function<void(int, const Session&)>;
    using UserUpdateFn = function<void(int, const Participant&)>;
    using DsDataFn = function<void(const vector<uint8_t>&, uint64_t,  const Metadata&)>;
    using AudioDataFn = function<void(const vector<uint8_t>&, uint64_t, const Metadata&)>;
    using VideoDataFn = function<void(const vector<uint8_t>&, uint64_t,  const Metadata&)>;
    using TranscriptDataFn = function<void(const vector<uint8_t>&, uint64_t, const Metadata&)>;
    using LeaveFn = function<void(int)>;

    // Media type constants
    enum MediaType {
        AUDIO = 1,
        VIDEO = 2,
        DESKSHARE = 4,
        TRANSCRIPT = 8,
        CHAT = 16,
        ALL = 31  // Sum of all types (1+2+4+8+16)
    };

    Client();
    ~Client();

    static void initialize(const string& ca);
    static void uninitialize();
    void configure(const MediaParams& params, int media_types, bool enable_application_layer_encryption = false);

    void enableVideo(bool enable);
    void enableAudio(bool enable);
    void enableTranscript(bool enable);
    void enableDeskshare(bool enable);

    void setOnJoinConfirm(JoinConfirmFn callback);
    void setOnSessionUpdate(SessionUpdateFn callback);
    void setOnDeskshareData(DsDataFn callback);
    void setOnUserUpdate(UserUpdateFn callback);
    void setOnAudioData(AudioDataFn callback);
    void setOnVideoData(VideoDataFn callback);
    void setOnTranscriptData(TranscriptDataFn callback);
    void setOnLeave(LeaveFn callback);

    void setDeskshareParams(const DeskshareParams& ds_params);
    void setVideoParams(const VideoParams& video_params);
    void setAudioParams(const AudioParams& audio_params);

    void join(const string& meeting_uuid, const string& rtms_stream_id, const string& signature, const string& server_url, int timeout = -1);

    void poll();
    void release();
    
    string uuid() const;
    string streamId() const;
private:
    mutable mutex mutex_;
    rtms_csdk* sdk_;

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

    static void handleJoinConfirm(struct rtms_csdk* sdk, int reason);
    static void handleSessionUpdate(struct rtms_csdk* sdk, int op, struct session_info* sess);
    static void handleUserUpdate(struct rtms_csdk* sdk, int op, struct participant_info* pi);
    static void handleDsData(struct rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, struct rtms_metadata* md);
    static void handleAudioData(struct rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, struct rtms_metadata* md);
    static void handleVideoData(struct rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, struct rtms_metadata* md);
    static void handleTranscriptData(struct rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, struct rtms_metadata* md);
    static void handleLeave(struct rtms_csdk* sdk, int reason);

    static unordered_map<struct rtms_csdk*, Client*> sdk_registry_;
    static mutex registry_mutex_;

    void throwIfError(int result, const std::string& operation) const;
    static Client* getClient(struct rtms_csdk* sdk);
    
    void updateMediaConfiguration(int mediaType, bool enable = true);
};
}

#endif // RTMS_H