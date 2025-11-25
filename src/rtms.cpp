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
    : stat_time_(info.stat_time),
      status_(info.status) {
    // Safely handle session_id pointer (can be NULL or invalid)
    if (info.session_id && (uintptr_t)info.session_id > 0xFFFF) {
        session_id_ = std::string(info.session_id);
    } else {
        session_id_ = "";
    }

    // Safely handle stream_id pointer (can be NULL or invalid)
    if (info.stream_id && (uintptr_t)info.stream_id > 0xFFFF) {
        stream_id_ = std::string(info.stream_id);
    } else {
        stream_id_ = "";
    }

    // Safely handle fixed-size array - may not be null-terminated!
    // strnlen stops at null OR max length, whichever comes first
    meeting_id_ = std::string(info.meeting_id, strnlen(info.meeting_id, MAX_MEETING_ID_LEN));
}

string Session::sessionId() const {
    return session_id_;
}

string Session::streamId() const {
    return stream_id_;
}

string Session::meetingId() const {
    return meeting_id_;
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

BaseMediaParams::BaseMediaParams()
    : content_type_(0), codec_(0), data_opt_(0) {}

void BaseMediaParams::setContentType(int content_type) {
    content_type_ = content_type;
}

void BaseMediaParams::setCodec(int codec) {
    codec_ = codec;
}

void BaseMediaParams::setDataOpt(int data_opt) {
    data_opt_ = data_opt;
}

int BaseMediaParams::contentType() const {
    return content_type_;
}

int BaseMediaParams::codec() const {
    return codec_;
}

int BaseMediaParams::dataOpt() const {
    return data_opt_;
}

AudioParams::AudioParams()
    : BaseMediaParams(), sample_rate_(3), channel_(2),
      duration_(20), frame_size_(960) {
    // Set sensible defaults for audio streaming
    // These values work out-of-box and allow per-participant audio identification
    setContentType(2);  // RAW_AUDIO
    setCodec(4);        // OPUS
    setDataOpt(2);      // AUDIO_MULTI_STREAMS - enables per-participant audio with userId
}

AudioParams::AudioParams(int content_type, int codec, int sample_rate, 
                               int channel, int data_opt, int duration, int frame_size)
    : BaseMediaParams(), sample_rate_(sample_rate), channel_(channel),
      duration_(duration), frame_size_(frame_size) {
    setContentType(content_type);
    setCodec(codec);
    setDataOpt(data_opt);
}

void AudioParams::setSampleRate(int sample_rate) {
    sample_rate_ = sample_rate;
}

void AudioParams::setChannel(int channel) {
    channel_ = channel;
}

void AudioParams::setDuration(int duration) {
    duration_ = duration;
}

void AudioParams::setFrameSize(int frame_size) {
    frame_size_ = frame_size;
}

int AudioParams::sampleRate() const {
    return sample_rate_;
}

int AudioParams::channel() const {
    return channel_;
}

int AudioParams::duration() const {
    return duration_;
}

int AudioParams::frameSize() const {
    return frame_size_;
}

audio_parameters AudioParams::toNative() const {
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

void AudioParams::validate() const {
    // Validate required fields are set (non-zero)
    if (contentType() == 0) {
        throw std::invalid_argument("AudioParams: contentType must be set (e.g., RAW_AUDIO=2)");
    }
    if (codec() == 0) {
        throw std::invalid_argument("AudioParams: codec must be set (e.g., OPUS=4)");
    }
    if (channel_ == 0) {
        throw std::invalid_argument("AudioParams: channel must be set (e.g., STEREO=2)");
    }
    if (dataOpt() == 0) {
        throw std::invalid_argument("AudioParams: dataOpt must be set (e.g., AUDIO_MULTI_STREAMS=2)");
    }

    // Validate codec-specific requirements
    if (codec() == 4) { // OPUS
        if (sample_rate_ != 3) { // SR_48K
            throw std::invalid_argument("AudioParams: OPUS codec requires 48kHz sample rate (sampleRate=3)");
        }
    }

    // Validate frame size matches sample rate and duration if both are set
    if (duration_ > 0 && frame_size_ > 0) {
        // Expected frame size calculation based on sample rate
        int expected_frame_size = 0;
        switch (sample_rate_) {
            case 0: expected_frame_size = 8000 * duration_ / 1000; break;   // SR_8K
            case 1: expected_frame_size = 16000 * duration_ / 1000; break;  // SR_16K
            case 2: expected_frame_size = 32000 * duration_ / 1000; break;  // SR_32K
            case 3: expected_frame_size = 48000 * duration_ / 1000; break;  // SR_48K
            default:
                throw std::invalid_argument("AudioParams: invalid sample rate value");
        }

        if (frame_size_ != expected_frame_size) {
            throw std::invalid_argument(
                "AudioParams: frameSize (" + std::to_string(frame_size_) +
                ") does not match sampleRate and duration (expected " +
                std::to_string(expected_frame_size) + " for " + std::to_string(duration_) + "ms)"
            );
        }
    }
}

VideoParams::VideoParams()
    : BaseMediaParams(), resolution_(0), fps_(0) {}

VideoParams::VideoParams(int content_type, int codec, int resolution, int data_opt, int fps)
    : BaseMediaParams(), resolution_(resolution), fps_(fps) {
    setContentType(content_type);
    setCodec(codec);
    setDataOpt(data_opt);
}

void VideoParams::setResolution(int resolution)
{
    resolution_ = resolution;
}

void VideoParams::setFps(int fps) {
    fps_ = fps;
}

int VideoParams::resolution() const {
    return resolution_;
}

int VideoParams::fps() const {
    return fps_;
}

DeskshareParams::DeskshareParams() 
    : BaseMediaParams(), resolution_(0), fps_(0) {}

DeskshareParams::DeskshareParams(int content_type, int codec, int resolution, int fps)
    : BaseMediaParams(), resolution_(resolution), fps_(fps) {
    setContentType(content_type);
    setCodec(codec);
}

ds_parameters DeskshareParams::toNative() const {
    ds_parameters params;
    params.content_type = contentType();
    params.codec = codec();
    params.resolution = resolution_;
    params.fps = fps_;
    return params;
}

video_parameters VideoParams::toNative() const {
    video_parameters params;
    params.content_type = contentType();
    params.codec = codec();
    params.resolution = resolution_;
    params.data_opt = dataOpt();
    params.fps = fps_;
    return params;
}

void DeskshareParams::setResolution(int resolution)
{
    resolution_ = resolution;
}

void DeskshareParams::setFps(int fps) {
    fps_ = fps;
}

int DeskshareParams::resolution() const {
    return resolution_;
}

int DeskshareParams::fps() const {
    return fps_;
}

MediaParams::MediaParams(): audio_params_(nullptr), video_params_(nullptr), ds_params_(nullptr) {}

MediaParams::~MediaParams() = default;

void MediaParams::setDeskshareParams(const DeskshareParams& ds_params)
{
    ds_params_ = make_unique<DeskshareParams>(ds_params);
}

void MediaParams::setAudioParams(const AudioParams& audio_params)
{
    audio_params_ = make_unique<AudioParams>(audio_params);
}

void MediaParams::setVideoParams(const VideoParams& video_params) {
    video_params_ = make_unique<VideoParams>(video_params);
}

const DeskshareParams& MediaParams::deskshareParams() const
{
    if (!ds_params_) {
        throw Exception(RTMS_SDK_NOT_EXIST, "DS parameters not set");
    }
    return *ds_params_;
}

const AudioParams& MediaParams::audioParams() const
{
    if (!audio_params_) {
        throw Exception(RTMS_SDK_NOT_EXIST, "Audio parameters not set");
    }
    return *audio_params_;
}

const VideoParams& MediaParams::videoParams() const {
    if (!video_params_) {
        throw Exception(RTMS_SDK_NOT_EXIST, "Video parameters not set");
    }
    return *video_params_;
}

bool MediaParams::hasDeskshareParams() const
{
    return ds_params_ != nullptr;
}

bool MediaParams::hasAudioParams() const
{
    return audio_params_ != nullptr;
}

bool MediaParams::hasVideoParams() const {
    return video_params_ != nullptr;
}

media_parameters MediaParams::toNative() const {
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

    if (ds_params_) {
        ds_parameters* ds_params = new ds_parameters();
        *ds_params = ds_params_->toNative();
        params.ds_param = ds_params;
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

void Client::initialize(const string& ca_path, int is_verify_cert, const char* agent) {
    // SDK will crash if agent is nullptr, so pass empty string instead
    const char* agent_str = (agent != nullptr) ? agent : "";
    int result = rtms_init(ca_path.empty() ? nullptr : ca_path.c_str(), is_verify_cert, agent_str);
    if (result != RTMS_SDK_OK) {
        throw Exception(result, "Failed to initialize RTMS SDK");
    }
}

void Client::uninitialize() {
    rtms_uninit();
}

void Client::configure(const MediaParams& params, int media_types, bool enable_application_layer_encryption) {
    // Store the media parameters for future use
    media_params_ = params;
    enabled_media_types_ = media_types;
    media_params_updated_ = true;
    
    if (!params.hasAudioParams() && !params.hasVideoParams() && !params.hasDeskshareParams()) {
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

void Client::enableDeskshare(bool enable) {
    updateMediaConfiguration(MediaType::DESKSHARE, enable);
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

void Client::setOnDeskshareData(DsDataFn callback){
    lock_guard<mutex> lock(mutex_);
    ds_data_callback_ = std::move(callback);

    updateMediaConfiguration(MediaType::DESKSHARE);
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

void Client::setOnEventEx(EventExFn callback) {
    lock_guard<mutex> lock(mutex_);
    event_ex_callback_ = std::move(callback);
}

void Client::setDeskshareParams(const DeskshareParams& ds_params)
{
    media_params_.setDeskshareParams(ds_params);
}

void Client::setVideoParams(const VideoParams& video_params)
{
    media_params_.setVideoParams(video_params);
}

void Client::setAudioParams(const AudioParams& audio_params)
{
    media_params_.setAudioParams(audio_params);
}

void Client::join(const string& meeting_uuid, const string& rtms_stream_id, 
                    const string& signature, const string& server_url, int timeout) {
    struct rtms_csdk_ops ops;
    memset(&ops, 0, sizeof(ops));
    ops.on_join_confirm = &Client::handleJoinConfirm;
    ops.on_session_update = &Client::handleSessionUpdate;
    ops.on_user_update = &Client::handleUserUpdate;
    ops.on_ds_data = &Client::handleDsData;
    ops.on_audio_data = &Client::handleAudioData;
    ops.on_video_data = &Client::handleVideoData;
    ops.on_transcript_data = &Client::handleTranscriptData;
    ops.on_leave = &Client::handleLeave;
    ops.on_event_ex = &Client::handleEventEx;
    
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

void Client::handleDsData(rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, rtms_metadata* md) {
    Client* client = getClient(sdk);
    if (client && buf && size > 0 && md) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->ds_data_callback_) {
            vector<uint8_t> data(buf, buf + size);
            Metadata metadata(*md);
            client->ds_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::handleAudioData(struct rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, struct rtms_metadata* md) {
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

void Client::handleVideoData(struct rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, struct rtms_metadata* md) {
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

void Client::handleTranscriptData(struct rtms_csdk* sdk, unsigned char* buf, int size, uint64_t timestamp, struct rtms_metadata* md) {
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

void Client::handleEventEx(struct rtms_csdk* sdk, const char* buf, int size) {
    Client* client = getClient(sdk);
    if (client && buf && size > 0) {
        lock_guard<mutex> lock(client->mutex_);
        if (client->event_ex_callback_) {
            string event_data(buf, size);
            client->event_ex_callback_(event_data);
        }
    }
}

} // namespace rtms