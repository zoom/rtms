#include "rtms.h"
#include <cstring>
#include <iostream>
#include <algorithm>

namespace rtms {

Exception::Exception(int error_code, const string& message)
    : runtime_error(message), error_code_(error_code) {}

int Exception::code() const noexcept {
    return error_code_;
}

AiTargetLanguage::AiTargetLanguage(const ai_target_lan& atl)
    : lid_(atl.lid),
      tone_id_(atl.toneid),
      voice_id_(atl.voice_id),
      engine_(atl.engine) {}

int AiTargetLanguage::lid() const { return lid_; }
int AiTargetLanguage::toneId() const { return tone_id_; }
string AiTargetLanguage::voiceId() const { return voice_id_; }
string AiTargetLanguage::engine() const { return engine_; }

AiInterpreter::AiInterpreter(const ai_interpreter& aii)
    : lid_(aii.lid),
      timestamp_(aii.timestamp),
      channel_num_(aii.channel_num),
      sample_rate_(aii.sample_rate) {
    int count = (aii.target_size > 0 && aii.target_size < 100) ? aii.target_size : 0;
    targets_.reserve(count);
    for (int i = 0; i < count; ++i)
        targets_.emplace_back(aii.atl[i]);
}

int AiInterpreter::lid() const { return lid_; }
uint64_t AiInterpreter::timestamp() const { return timestamp_; }
int AiInterpreter::channelNum() const { return channel_num_; }
int AiInterpreter::sampleRate() const { return sample_rate_; }
const vector<AiTargetLanguage>& AiInterpreter::targets() const { return targets_; }

Metadata::Metadata(const rtms_metadata& metadata)
    : user_name_(metadata.user_name ? metadata.user_name : ""),
      user_id_(metadata.user_id),
      start_ts_(metadata.start_ts),
      end_ts_(metadata.end_ts),
      ai_interpreter_(metadata.aii) {}

string Metadata::userName() const { return user_name_; }
int Metadata::userId() const { return user_id_; }
uint64_t Metadata::startTs() const { return start_ts_; }
uint64_t Metadata::endTs() const { return end_ts_; }
const AiInterpreter& Metadata::aiInterpreter() const { return ai_interpreter_; }

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
    : BaseMediaParams(), resolution_((int)MEDIA_RESOLUTION::HD), fps_(30) {
    setContentType((int)MEDIA_CONTENT_TYPE::RAW_VIDEO);
    setCodec((int)MEDIA_PAYLOAD_TYPE::H264);
    setDataOpt((int)MEDIA_DATA_OPTION::VIDEO_SINGLE_ACTIVE_STREAM);
}

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

TranscriptParams::TranscriptParams()
    : BaseMediaParams(), src_language_(-1), enable_lid_(true) {
    setContentType(5);  // TEXT
}

void TranscriptParams::setSrcLanguage(int src_language) {
    src_language_ = src_language;
}

void TranscriptParams::setEnableLid(bool enable_lid) {
    enable_lid_ = enable_lid;
}

int TranscriptParams::srcLanguage() const {
    return src_language_;
}

bool TranscriptParams::enableLid() const {
    return enable_lid_;
}

transcript_parameters TranscriptParams::toNative() const {
    transcript_parameters p{};
    p.content_type  = contentType();
    p.src_language  = src_language_;
    p.enable_lid    = enable_lid_;
    return p;
}

DeskshareParams::DeskshareParams()
    : BaseMediaParams(), resolution_((int)MEDIA_RESOLUTION::HD), fps_(30) {
    setContentType((int)MEDIA_CONTENT_TYPE::RAW_VIDEO);
    setCodec((int)MEDIA_PAYLOAD_TYPE::H264);
    setDataOpt((int)MEDIA_DATA_OPTION::VIDEO_SINGLE_ACTIVE_STREAM);
}

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

// Shared validation for video/deskshare codec and fps compatibility
static void validateCodecFps(int codec, int fps, const std::string& param_type) {
    // Codec constants from MEDIA_PAYLOAD_TYPE enum
    const int CODEC_JPG = 5;
    const int CODEC_PNG = 6;
    const int CODEC_H264 = 7;

    if (codec == CODEC_JPG || codec == CODEC_PNG) {
        if (fps > 5) {
            throw std::invalid_argument(
                param_type + ": JPG/PNG codec requires fps <= 5 (got " + std::to_string(fps) +
                "). Use H264 codec for higher frame rates."
            );
        }
    } else if (codec == CODEC_H264) {
        if (fps > 30) {
            throw std::invalid_argument(
                param_type + ": H264 codec supports max fps of 30 (got " + std::to_string(fps) + ")"
            );
        }
        if (fps <= 0) {
            throw std::invalid_argument(
                param_type + ": fps must be > 0 for H264 codec"
            );
        }
    }
}

void VideoParams::validate() const {
    validateCodecFps(codec(), fps_, "VideoParams");

    if (resolution_ == 0) {
        throw std::invalid_argument("VideoParams: resolution must be set (e.g., HD=2)");
    }
}

void DeskshareParams::validate() const {
    validateCodecFps(codec(), fps_, "DeskshareParams");
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

void MediaParams::setTranscriptParams(const TranscriptParams& transcript_params) {
    transcript_params_ = make_unique<TranscriptParams>(transcript_params);
}

const TranscriptParams& MediaParams::transcriptParams() const {
    if (!transcript_params_) {
        throw Exception(RTMS_SDK_NOT_EXIST, "Transcript parameters not set");
    }
    return *transcript_params_;
}

bool MediaParams::hasTranscriptParams() const {
    return transcript_params_ != nullptr;
}

media_parameters MediaParams::toNative() const {
    media_parameters params;
    memset(&params, 0, sizeof(params));
    params.audio_param = nullptr;
    params.video_param = nullptr;
    params.ds_param = nullptr;
    params.tr_param = nullptr;

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

    if (transcript_params_) {
        transcript_parameters* tr_params = new transcript_parameters();
        *tr_params = transcript_params_->toNative();
        params.tr_param = tr_params;
    }

    return params;
}

Client::Client()
    : sdk_(nullptr),
      enabled_media_types_(0),
      media_params_updated_(false),
      sdk_opened_(false),
      join_confirmed_(false) {
    sdk_ = rtms_sdk_provider::instance()->create_sdk();
    if (!sdk_) {
        throw Exception(RTMS_SDK_FAILURE, "Failed to allocate RTMS SDK instance");
    }
}

Client::~Client() {
    try {
        if (sdk_) {
            rtms_sdk_provider::instance()->release_sdk(sdk_);
            sdk_ = nullptr;
        }
    } catch (const exception& e) {
        cerr << "Error during Client destruction: " << e.what() << endl;
    }
}

void Client::initialize(const string& ca_path, int is_verify_cert, const char* agent) {
    if (agent && agent[0] != '\0') {
        g_agent = agent;
    }
    int result = rtms_sdk_provider::instance()->init(
        ca_path.empty() ? nullptr : ca_path.c_str(),
        is_verify_cert != 0
    );
    if (result != RTMS_SDK_OK) {
        throw Exception(result, "Failed to initialize RTMS SDK");
    }
}

void Client::uninitialize() {
    rtms_sdk_provider::instance()->uninit();
}

void Client::configure(const MediaParams& params, int media_types, bool enable_application_layer_encryption, bool apply_defaults) {
    // Always store the configuration so join() can apply it later
    media_params_ = params;
    enabled_media_types_ = media_types;
    media_params_updated_ = true;

    // Don't call sdk_->config() until sdk_->open() has been called in join().
    // Calling config() before open() (e.g. from setOnVideoData) hangs for VIDEO.
    if (!sdk_opened_) return;

    // If a media type is enabled but no params were provided, fill in defaults.
    // Skip when called from setAudioParams/setVideoParams/setDeskshareParams (apply_defaults=false).
    if (apply_defaults) {
        if ((media_types & MediaType::AUDIO) && !media_params_.hasAudioParams()) {
#ifdef RTMS_DEBUG
            cerr << "[DEBUG CONFIG] Audio enabled but no params set, using defaults (OPUS/AUDIO_MULTI_STREAMS)" << endl;
#endif
            media_params_.setAudioParams(AudioParams());
        }
        if ((media_types & MediaType::VIDEO) && !media_params_.hasVideoParams()) {
#ifdef RTMS_DEBUG
            cerr << "[DEBUG CONFIG] Video enabled but no params set, using defaults (H264/HD/30fps)" << endl;
#endif
            media_params_.setVideoParams(VideoParams());
        }
        if ((media_types & MediaType::DESKSHARE) && !media_params_.hasDeskshareParams()) {
#ifdef RTMS_DEBUG
            cerr << "[DEBUG CONFIG] Deskshare enabled but no params set, using defaults (H264/HD/30fps)" << endl;
#endif
            media_params_.setDeskshareParams(DeskshareParams());
        }
    }

    if (!media_params_.hasAudioParams() && !media_params_.hasVideoParams() && !media_params_.hasDeskshareParams()) {
#ifdef RTMS_DEBUG
        cerr << "[DEBUG CONFIG] Calling config with NULL params, media_types=" << media_types << endl;
#endif
        int result = sdk_->config(nullptr, media_types, enable_application_layer_encryption ? 1 : 0);
        throwIfError(result, "configure with null params");
        return;
    }

    media_parameters native_params = media_params_.toNative();

#ifdef RTMS_DEBUG
    cerr << "[DEBUG CONFIG] Calling config with params, media_types=" << media_types << endl;
    if (native_params.audio_param) {
        cerr << "[DEBUG CONFIG] audio_param: data_opt=" << native_params.audio_param->data_opt
             << " content_type=" << native_params.audio_param->content_type
             << " codec=" << native_params.audio_param->codec << endl;
    } else {
        cerr << "[DEBUG CONFIG] audio_param is NULL" << endl;
    }
#endif

    int result = sdk_->config(&native_params, media_types, enable_application_layer_encryption ? 1 : 0);

    if (native_params.audio_param) {
        delete native_params.audio_param;
    }
    if (native_params.video_param) {
        delete native_params.video_param;
    }
    if (native_params.ds_param) {
        delete native_params.ds_param;
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

    if (sdk_ && sdk_opened_) {
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
    {
        lock_guard<mutex> lock(mutex_);
        user_update_callback_ = std::move(callback);
    }
    subscribeEvent({
        (int)EVENT_TYPE::ACTIVE_SPEAKER_CHANGE,
        (int)EVENT_TYPE::PARTICIPANT_JOIN,
        (int)EVENT_TYPE::PARTICIPANT_LEAVE,
    });
}

void Client::setOnDeskshareData(DsDataFn callback){
    lock_guard<mutex> lock(mutex_);
    ds_data_callback_ = std::move(callback);

    updateMediaConfiguration(MediaType::DESKSHARE);
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

void Client::subscribeEvent(const std::vector<int>& events) {
    if (events.empty()) return;
    if (!sdk_) {
        throw Exception(RTMS_SDK_INVALID_STATUS, "SDK not initialized");
    }

    lock_guard<mutex> lock(mutex_);

    // If not yet joined, queue the subscriptions for later
    if (!join_confirmed_) {
        for (int event : events) {
            if (std::find(pending_event_subscriptions_.begin(),
                         pending_event_subscriptions_.end(), event) ==
                         pending_event_subscriptions_.end()) {
                pending_event_subscriptions_.push_back(event);
            }
        }
        return;
    }

    // Already joined - subscribe immediately (release lock first to avoid holding during SDK call)
    mutex_.unlock();
    int result = sdk_->subscribe_event(const_cast<int*>(events.data()), static_cast<int>(events.size()));
    mutex_.lock();

    if (result != RTMS_SDK_OK) {
        throw Exception(result, "subscribe_event failed");
    }

    // Track subscribed events for cleanup
    for (int event : events) {
        if (std::find(subscribed_events_.begin(), subscribed_events_.end(), event) == subscribed_events_.end()) {
            subscribed_events_.push_back(event);
        }
    }
}

void Client::unsubscribeEvent(const std::vector<int>& events) {
    if (events.empty()) return;
    if (!sdk_) {
        throw Exception(RTMS_SDK_INVALID_STATUS, "SDK not initialized");
    }

    int result = sdk_->unsubscribe_event(const_cast<int*>(events.data()), static_cast<int>(events.size()));
    throwIfError(result, "unsubscribe_event");

    // Remove from tracked events
    lock_guard<mutex> lock(mutex_);
    for (int event : events) {
        subscribed_events_.erase(
            std::remove(subscribed_events_.begin(), subscribed_events_.end(), event),
            subscribed_events_.end()
        );
    }
}

void Client::setDeskshareParams(const DeskshareParams& ds_params)
{
    // Validate before setting (throws on invalid params)
    ds_params.validate();

    media_params_.setDeskshareParams(ds_params);

    // If deskshare is already enabled, reconfigure with the new params
    // Pass apply_defaults=false to avoid filling in defaults for other media types
    if (enabled_media_types_ & MediaType::DESKSHARE) {
        try {
            configure(media_params_, enabled_media_types_, false, false);
        } catch (const Exception& e) {
            cerr << "Warning: Failed to reconfigure deskshare params: " << e.what() << endl;
        }
    }
}

void Client::setVideoParams(const VideoParams& video_params)
{
    // Validate before setting (throws on invalid params)
    video_params.validate();

    media_params_.setVideoParams(video_params);

    // If video is already enabled, reconfigure with the new params
    // Pass apply_defaults=false to avoid filling in defaults for other media types
    if (enabled_media_types_ & MediaType::VIDEO) {
        try {
            configure(media_params_, enabled_media_types_, false, false);
        } catch (const Exception& e) {
            cerr << "Warning: Failed to reconfigure video params: " << e.what() << endl;
        }
    }
}

void Client::setAudioParams(const AudioParams& audio_params)
{
    // Validate before setting (throws on invalid params)
    audio_params.validate();

    media_params_.setAudioParams(audio_params);

    // If audio is already enabled, reconfigure with the new params
    // Pass apply_defaults=false to avoid filling in defaults for other media types
    if (enabled_media_types_ & MediaType::AUDIO) {
        try {
            configure(media_params_, enabled_media_types_, false, false);
        } catch (const Exception& e) {
            cerr << "Warning: Failed to reconfigure audio params: " << e.what() << endl;
        }
    }
}

void Client::setTranscriptParams(const TranscriptParams& transcript_params)
{
    media_params_.setTranscriptParams(transcript_params);

    // If transcript is already enabled, reconfigure with the new params
    if (enabled_media_types_ & MediaType::TRANSCRIPT) {
        try {
            configure(media_params_, enabled_media_types_, false, false);
        } catch (const Exception& e) {
            cerr << "Warning: Failed to reconfigure transcript params: " << e.what() << endl;
        }
    }
}

void Client::setProxy(const string& proxy_type, const string& proxy_url)
{
    int result = sdk_->set_proxy(proxy_type.c_str(), proxy_url.c_str());
    if (result != 0) {
        throw Exception(result, "setProxy failed: Operation failed");
    }
}

void Client::subscribeVideo(int user_id, bool subscribe)
{
    int result = sdk_->send_subscript_video(user_id, subscribe);
    throwIfError(result, "subscribeVideo");
}

void Client::setOnParticipantVideo(ParticipantVideoFn callback)
{
    {
        lock_guard<mutex> lock(mutex_);
        participant_video_callback_ = std::move(callback);
    }
    subscribeEvent({
        (int)EVENT_TYPE::PARTICIPANT_VIDEO_ON,
        (int)EVENT_TYPE::PARTICIPANT_VIDEO_OFF,
    });
}

void Client::setOnVideoSubscribed(VideoSubscribedFn callback)
{
    lock_guard<mutex> lock(mutex_);
    video_subscribed_callback_ = std::move(callback);
}

void Client::join(const string& meeting_uuid, const string& rtms_stream_id,
                    const string& signature, const string& server_url, int timeout) {
    // Register this client as the sink — replaces the old static callback registry
    int result = sdk_->open(this);
    throwIfError(result, "open");
    sdk_opened_ = true;

    // Apply any media configuration that was registered before join() via
    // setOnAudioData / setOnVideoData / setVideoParams etc. Those calls stored
    // the state in media_params_ / enabled_media_types_ but deferred the actual
    // sdk_->config() call until now (after open).
    if (enabled_media_types_ > 0) {
        try {
            configure(media_params_, enabled_media_types_, false);
        } catch (const Exception& e) {
            cerr << "Warning: Failed to configure media types before join: " << e.what() << endl;
        }
    }

    result = sdk_->join(meeting_uuid.c_str(), rtms_stream_id.c_str(),
                        signature.c_str(), server_url.c_str(), timeout);
    throwIfError(result, "join");

    lock_guard<mutex> lock(mutex_);
    meeting_uuid_ = meeting_uuid;
    rtms_stream_id_ = rtms_stream_id;
}

void Client::poll() {
    int result = sdk_->poll();
    throwIfError(result, "poll");
}

void Client::release() {
    sdk_->leave(0);

    // Reset join state — mark sdk_opened_ false before release_sdk so that
    // any stopCallbacks() calls triggered during cleanup (e.g. setOnAudioData
    // with an empty lambda) do not attempt sdk_->config() on a dead session.
    {
        lock_guard<mutex> lock(mutex_);
        sdk_opened_ = false;
        join_confirmed_ = false;
        pending_event_subscriptions_.clear();
        subscribed_events_.clear();
    }

    rtms_sdk_provider::instance()->release_sdk(sdk_);
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

void Client::processPendingSubscriptions() {
    // Called with mutex_ already held
    if (pending_event_subscriptions_.empty()) return;

    // Process all pending subscriptions now that we're joined
    int result = sdk_->subscribe_event(
        const_cast<int*>(pending_event_subscriptions_.data()),
        static_cast<int>(pending_event_subscriptions_.size())
    );

    if (result == RTMS_SDK_OK) {
        // Move pending to subscribed
        for (int event : pending_event_subscriptions_) {
            if (std::find(subscribed_events_.begin(), subscribed_events_.end(), event) ==
                subscribed_events_.end()) {
                subscribed_events_.push_back(event);
            }
        }
    }
    pending_event_subscriptions_.clear();
}

// ============================================================================
// rtms_sdk_sink virtual overrides
// ============================================================================

void Client::on_join_confirm(int reason) {
    lock_guard<mutex> lock(mutex_);

    // Mark as joined FIRST
    join_confirmed_ = true;

    // Process any pending event subscriptions before user callback
    processPendingSubscriptions();

    // Then invoke user callback
    if (join_confirm_callback_) {
        join_confirm_callback_(reason);
    }
}

void Client::on_session_update(int op, struct session_info* sess) {
    if (sess) {
        lock_guard<mutex> lock(mutex_);
        if (session_update_callback_) {
            Session session(*sess);
            session_update_callback_(op, session);
        }
    }
}

void Client::on_user_update(int op, struct participant_info* pi) {
    if (pi) {
        lock_guard<mutex> lock(mutex_);
        if (user_update_callback_) {
            Participant participant(*pi);
            user_update_callback_(op, participant);
        }
    }
}

void Client::on_ds_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) {
    if (data_buf && size > 0 && md) {
        lock_guard<mutex> lock(mutex_);
        if (ds_data_callback_) {
            vector<uint8_t> data(data_buf, data_buf + size);
            Metadata metadata(*md);
            ds_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::on_audio_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) {
    if (data_buf && size > 0 && md) {
#ifdef RTMS_DEBUG
        cerr << "[DEBUG AUDIO] md->user_id=" << md->user_id
             << " md->user_name=" << (md->user_name ? md->user_name : "(null)") << endl;
#endif
        lock_guard<mutex> lock(mutex_);
        if (audio_data_callback_) {
            vector<uint8_t> data(data_buf, data_buf + size);
            Metadata metadata(*md);
            audio_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::on_video_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) {
    if (data_buf && size > 0 && md) {
        lock_guard<mutex> lock(mutex_);
        if (video_data_callback_) {
            vector<uint8_t> data(data_buf, data_buf + size);
            Metadata metadata(*md);
            video_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::on_transcript_data(unsigned char* data_buf, int size, uint64_t timestamp, struct rtms_metadata* md) {
    if (data_buf && size > 0 && md) {
#ifdef RTMS_DEBUG
        cerr << "[DEBUG TRANSCRIPT] md->user_id=" << md->user_id
             << " md->user_name=" << (md->user_name ? md->user_name : "(null)") << endl;
#endif
        lock_guard<mutex> lock(mutex_);
        if (transcript_data_callback_) {
            vector<uint8_t> data(data_buf, data_buf + size);
            Metadata metadata(*md);
            transcript_data_callback_(data, timestamp, metadata);
        }
    }
}

void Client::on_leave(int reason) {
    lock_guard<mutex> lock(mutex_);
    if (leave_callback_) {
        leave_callback_(reason);
    }
}

void Client::on_event_ex(const std::string& compact_str) {
    if (!compact_str.empty()) {
        lock_guard<mutex> lock(mutex_);
        if (event_ex_callback_) {
            event_ex_callback_(compact_str);
        }
    }
}

void Client::on_participant_video(std::vector<int> users, bool is_on) {
    lock_guard<mutex> lock(mutex_);
    if (participant_video_callback_) {
        participant_video_callback_(users, is_on);
    }
}

void Client::on_video_subscript_resp(int user_id, int status, std::string error) {
    lock_guard<mutex> lock(mutex_);
    if (video_subscribed_callback_) {
        video_subscribed_callback_(user_id, status, error);
    }
}

} // namespace rtms
