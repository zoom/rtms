#include "rtms.h"
#include <cstring>
#include <iostream>

namespace rtms {

unordered_map<struct rtms_csdk*, Client*> Client::sdk_registry_;
mutex Client::registry_mutex_;

Exception::Exception(int error_code, const string& message)
    : runtime_error(message), error_code_(error_code) {}

int Exception::code() const noexcept {
    return error_code_;
}

Metadata::Metadata(const rtms_metadata& metadata)
    : user_name_(metadata.user_name ? metadata.user_name : ""),
      user_id_(metadata.user_id) {}

string Metadata::userName() const {
    return user_name_;
}

int Metadata::userId() const {
    return user_id_;
}

Session::Session(const session_info& info)
    : session_id_(info.session_id ? info.session_id : ""),
      stat_time_(info.stat_time),
      status_(info.status) {}

string Session::sessionId() const {
    return session_id_;
}

int Session::statTime() const {
    return stat_time_;
}

int Session::status() const {
    return status_;
}

bool Session::isActive() const {
    return status_ == SESS_STATUS_ACTIVE;
}

bool Session::isPaused() const {
    return status_ == SESS_STATUS_PAUSED;
}

Participant::Participant(const participant_info& info)
    : id_(info.participant_id),
      name_(info.participant_name ? info.participant_name : "") {}

int Participant::id() const {
    return id_;
}

string Participant::name() const {
    return name_;
}

BaseMediaParameters::BaseMediaParameters()
    : content_type_(0), codec_(0), data_opt_(0) {}

void BaseMediaParameters::setContentType(int content_type) {
    content_type_ = content_type;
}

void BaseMediaParameters::setCodec(int codec) {
    codec_ = codec;
}

void BaseMediaParameters::setDataOpt(int data_opt) {
    data_opt_ = data_opt;
}

int BaseMediaParameters::contentType() const {
    return content_type_;
}

int BaseMediaParameters::codec() const {
    return codec_;
}

int BaseMediaParameters::dataOpt() const {
    return data_opt_;
}

AudioParameters::AudioParameters()
    : BaseMediaParameters(), sample_rate_(0), channel_(0),
      duration_(0), frame_size_(0) {}

AudioParameters::AudioParameters(int content_type, int codec, int sample_rate, 
                               int channel, int data_opt, int duration, int frame_size)
    : BaseMediaParameters(), sample_rate_(sample_rate), channel_(channel),
      duration_(duration), frame_size_(frame_size) {
    setContentType(content_type);
    setCodec(codec);
    setDataOpt(data_opt);
}

void AudioParameters::setSampleRate(int sample_rate) {
    sample_rate_ = sample_rate;
}

void AudioParameters::setChannel(int channel) {
    channel_ = channel;
}

void AudioParameters::setDuration(int duration) {
    duration_ = duration;
}

void AudioParameters::setFrameSize(int frame_size) {
    frame_size_ = frame_size;
}

int AudioParameters::sampleRate() const {
    return sample_rate_;
}

int AudioParameters::channel() const {
    return channel_;
}

int AudioParameters::duration() const {
    return duration_;
}

int AudioParameters::frameSize() const {
    return frame_size_;
}

audio_parameters AudioParameters::toNative() const {
    audio_parameters params;
    params.content_type = contentType();
    params.codec = codec();
    params.sample_rate = sample_rate_;
    params.channel = channel_;
    params.data_opt = dataOpt();
    params.duration = duration_;
    params.frame_size = frame_size_;
    return params;
}

VideoParameters::VideoParameters()
    : BaseMediaParameters(), resolution_(0), fps_(0) {}

VideoParameters::VideoParameters(int content_type, int codec, int resolution, int data_opt, int fps)
    : BaseMediaParameters(), resolution_(resolution), fps_(fps) {
    setContentType(content_type);
    setCodec(codec);
    setDataOpt(data_opt);
}

void VideoParameters::setResolution(int resolution) {
    resolution_ = resolution;
}

void VideoParameters::setFps(int fps) {
    fps_ = fps;
}

int VideoParameters::resolution() const {
    return resolution_;
}

int VideoParameters::fps() const {
    return fps_;
}

video_parameters VideoParameters::toNative() const {
    video_parameters params;
    params.content_type = contentType();
    params.codec = codec();
    params.resolution = resolution_;
    params.data_opt = dataOpt();
    params.fps = fps_;
    return params;
}

MediaParameters::MediaParameters(): audio_params_(nullptr), video_params_(nullptr) {}

MediaParameters::~MediaParameters() = default;

void MediaParameters::setAudioParameters(const AudioParameters& audio_params) {
    audio_params_ = make_unique<AudioParameters>(audio_params);
}

void MediaParameters::setVideoParameters(const VideoParameters& video_params) {
    video_params_ = make_unique<VideoParameters>(video_params);
}

const AudioParameters& MediaParameters::audioParameters() const {
    if (!audio_params_) {
        throw Exception(RTMS_SDK_NOT_EXIST, "Audio parameters not set");
    }
    return *audio_params_;
}

const VideoParameters& MediaParameters::videoParameters() const {
    if (!video_params_) {
        throw Exception(RTMS_SDK_NOT_EXIST, "Video parameters not set");
    }
    return *video_params_;
}

bool MediaParameters::hasAudioParameters() const {
    return audio_params_ != nullptr;
}

bool MediaParameters::hasVideoParameters() const {
    return video_params_ != nullptr;
}

media_parameters MediaParameters::toNative() const {
    media_parameters params;
    memset(&params, 0, sizeof(params));
    params.audio_param = nullptr;
    params.video_param = nullptr;
    
    if (audio_params_) {
        audio_parameters* audio_params = new audio_parameters();
        *audio_params = audio_params_->toNative();
        params.audio_param = audio_params;
    }
    
    if (video_params_) {
        video_parameters* video_params = new video_parameters();
        *video_params = video_params_->toNative();
        params.video_param = video_params;
    }
    
    return params;
}

Client::Client() 
    : sdk_(nullptr), 
      enabled_media_types_(0), 
      media_params_updated_(false) {
    sdk_ = rtms_alloc();
    if (!sdk_) {
        throw Exception(RTMS_SDK_FAILURE, "Failed to allocate RTMS SDK instance");
    }
    
    lock_guard<mutex> lock(registry_mutex_);
    sdk_registry_[sdk_] = this;
}

Client::~Client() {
    try {
        if (sdk_) {
            {
                lock_guard<mutex> lock(registry_mutex_);
                sdk_registry_.erase(sdk_);
            }
            
            rtms_release(sdk_);
            sdk_ = nullptr;
        }
    } catch (const exception& e) {
        cerr << "Error during Client destruction: " << e.what() << endl;
    }
}

void Client::initialize(const string& ca_path) {
    int result = rtms_init(ca_path.empty() ? nullptr : ca_path.c_str());
    if (result != RTMS_SDK_OK) {
        throw Exception(result, "Failed to initialize RTMS SDK");
    }
}

void Client::uninitialize() {
    rtms_uninit();
}

void Client::configure(const MediaParameters& params, int media_types, bool enable_application_layer_encryption) {
    // Store the media parameters for future use
    media_params_ = params;
    enabled_media_types_ = media_types;
    media_params_updated_ = true;
    
    if (!params.hasAudioParameters() && !params.hasVideoParameters()) {
        int result = rtms_config(sdk_, NULL, media_types, enable_application_layer_encryption ? 1 : 0);
        throwIfError(result, "configure with null params");
        return;
    }
    
    media_parameters native_params = params.toNative();
    
    int result = rtms_config(sdk_, &native_params, media_types, enable_application_layer_encryption ? 1 : 0);
    
    if (native_params.audio_param) {
        delete native_params.audio_param;
    }
    if (native_params.video_param) {
        delete native_params.video_param;
    }
    
    throwIfError(result, "configure");
}

void Client::enableVideo(bool enable) {
    updateMediaConfiguration(MediaType::VIDEO, enable);
}

void Client::enableAudio(bool enable) {
    updateMediaConfiguration(MediaType::AUDIO, enable);
}

void Client::enableTranscript(bool enable) {
    updateMediaConfiguration(MediaType::TRANSCRIPT, enable);
}

void Client::updateMediaConfiguration(int mediaType, bool enable) {

    if (enable) { 
        enabled_media_types_ |= mediaType;
    } else {
        enabled_media_types_ &= mediaType;
    }
    
    if (sdk_) {
        try {
            configure(media_params_, enabled_media_types_, false);
        } catch (const Exception& e) {
            cerr << "Warning: Failed to update media configuration: " << e.what() << endl;
        }
    }
}

void Client::setOnJoinConfirm(JoinConfirmFn callback) {
    lock_guard<mutex> lock(mutex_);
    join_confirm_callback_ = std::move(callback);
}

void Client::setOnSessionUpdate(SessionUpdateFn callback) {
    lock_guard<mutex> lock(mutex_);
    session_update_callback_ = std::move(callback);
}

void Client::setOnUserUpdate(UserUpdateFn callback) {
    lock_guard<mutex> lock(mutex_);
    user_update_callback_ = std::move(callback);
}

void Client::setOnAudioData(AudioDataFn callback) {
    lock_guard<mutex> lock(mutex_);
    audio_data_callback_ = std::move(callback);
    
    updateMediaConfiguration(MediaType::AUDIO);
}

void Client::setOnVideoData(VideoDataFn callback) {
    lock_guard<mutex> lock(mutex_);
    video_data_callback_ = std::move(callback);
    
    updateMediaConfiguration(MediaType::VIDEO);
}

void Client::setOnTranscriptData(TranscriptDataFn callback) {
    lock_guard<mutex> lock(mutex_);
    transcript_data_callback_ = std::move(callback);
    
    updateMediaConfiguration(MediaType::TRANSCRIPT);
}

void Client::setOnLeave(LeaveFn callback) {
    lock_guard<mutex> lock(mutex_);
    leave_callback_ = std::move(callback);
}

void Client::setVideoParameters(const VideoParameters& video_params)
{
    media_params_.setVideoParameters(video_params);
}

void Client::setAudioParameters(const AudioParameters& audio_params)
{
    media_params_.setAudioParameters(audio_params);
}

void Client::join(const string& meeting_uuid, const string& rtms_stream_id, 
                    const string& signature, const string& server_url, int timeout) {
    struct rtms_csdk_ops ops;
    memset(&ops, 0, sizeof(ops));
    ops.on_join_confirm = &Client::handleJoinConfirm;
    ops.on_session_update = &Client::handleSessionUpdate;
    ops.on_user_update = &Client::handleUserUpdate;
    ops.on_audio_data = &Client::handleAudioData;
    ops.on_video_data = &Client::handleVideoData;
    ops.on_transcript_data = &Client::handleTranscriptData;
    ops.on_leave = &Client::handleLeave;
    
    int result = rtms_set_callbacks(sdk_, &ops);
    throwIfError(result, "set_callbacks");
    
    if (enabled_media_types_ > 0 && !media_params_updated_) {
        try {
            configure(media_params_, enabled_media_types_, false);
        } catch (const Exception& e) {
            cerr << "Warning: Failed to configure media types before join: " << e.what() << endl;
        }
    }
    
    result = rtms_join(sdk_, meeting_uuid.c_str(), rtms_stream_id.c_str(), 
                      signature.c_str(), server_url.c_str(), timeout);
    throwIfError(result, "join");
    
    lock_guard<mutex> lock(mutex_);
    meeting_uuid_ = meeting_uuid;
    rtms_stream_id_ = rtms_stream_id;
}

void Client::poll() {
    int result = rtms_poll(sdk_);
    throwIfError(result, "poll");
}

void Client::release() {
    int result = rtms_release(sdk_);
    throwIfError(result, "release");
    
    {
        lock_guard<mutex> lock(registry_mutex_);
        sdk_registry_.erase(sdk_);
    }
    
    sdk_ = nullptr;
}

string Client::uuid() const {
    lock_guard<mutex> lock(mutex_);
    return meeting_uuid_;
}

string Client::streamId() const {
    lock_guard<mutex> lock(mutex_);
    return rtms_stream_id_;
}

void Client::throwIfError(int result, const string& operation) const {
    if (result != RTMS_SDK_OK) {
        string error_message;
        switch (result) {
            case RTMS_SDK_FAILURE:
                error_message = "Operation failed";
                break;
            case RTMS_SDK_TIMEOUT:
                error_message = "Operation timed out";
                break;
            case RTMS_SDK_NOT_EXIST:
                error_message = "Resource does not exist";
                break;
            case RTMS_SDK_WRONG_TYPE:
                error_message = "Wrong type";
                break;
            case RTMS_SDK_INVALID_STATUS:
                error_message = "Invalid status";
                break;
            case RTMS_SDK_INVALID_ARGS:
                error_message = "Invalid arguments";
                break;
            default:
                error_message = "Unknown error";
                break;
        }
        throw Exception(result, operation + " failed: " + error_message);
    }
}

Client* Client::getClient(struct rtms_csdk* sdk) {
    lock_guard<mutex> lock(registry_mutex_);
    auto it = sdk_registry_.find(sdk);
    if (it == sdk_registry_.end()) {
        return nullptr;
    }
    return it->second;
}

void Client::handleJoinConfirm(struct rtms_csdk* sdk, int reason) {
    Client* client = getClient(sdk);
    if (client) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->join_confirm_callback_) {
            client->join_confirm_callback_(reason);
        }
    }
}

void Client::handleSessionUpdate(struct rtms_csdk* sdk, int op, struct session_info* sess) {
    Client* client = getClient(sdk);
    if (client && sess) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->session_update_callback_) {
            Session session(*sess);
            client->session_update_callback_(op, session);
        }
    }
}

void Client::handleUserUpdate(struct rtms_csdk* sdk, int op, struct participant_info* pi) {
    Client* client = getClient(sdk);
    if (client && pi) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->user_update_callback_) {
            Participant participant(*pi);
            client->user_update_callback_(op, participant);
        }
    }
}

void Client::handleAudioData(struct rtms_csdk* sdk, unsigned char* buf, int size, 
                               unsigned int timestamp, struct rtms_metadata* md) {
    Client* client = getClient(sdk);
    if (client && buf && size > 0 && md) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->audio_data_callback_) {
            vector<uint8_t> data(buf, buf + size);
            Metadata metadata(*md);
            client->audio_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::handleVideoData(struct rtms_csdk* sdk, unsigned char* buf, int size, 
                               unsigned int timestamp, struct rtms_metadata* md) {
    Client* client = getClient(sdk);
    if (client && buf && size > 0 && md) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->video_data_callback_) {
            vector<uint8_t> data(buf, buf + size);
            Metadata metadata(*md);
            client->video_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::handleTranscriptData(struct rtms_csdk* sdk, unsigned char* buf, int size, 
                                    unsigned int timestamp, struct rtms_metadata* md) {
    Client* client = getClient(sdk);
    if (client && buf && size > 0 && md) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->transcript_data_callback_) {
            vector<uint8_t> data(buf, buf + size);
            Metadata metadata(*md);
            client->transcript_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::handleLeave(struct rtms_csdk* sdk, int reason) {
    Client* client = getClient(sdk);
    if (client) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->leave_callback_) {
            client->leave_callback_(reason);
        }
    }
}

} // namespace rtms