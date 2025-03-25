#ifndef RTMS_COMMON_H
#define RTMS_COMMON_H


struct session_info {
        char *session_id;
	int stat_time;
        int status;
};

enum session_status {
	SESS_STATUS_ACTIVE,
	SESS_STATUS_PAUSED,
};

struct participant_info {
	int participant_id;
	char *participant_name;
};

struct audio_parameters {
        int content_type;
        int codec;
        int sample_rate;
        int channel;
        int data_opt;
        int duration;
	int frame_size;
};

struct video_parameters {
	int content_type;
        int codec;
        int resolution;
	int data_opt;
        int fps;
};

struct media_parameters {
        struct audio_parameters *audio_param;
        struct video_parameters *video_param;
	/*
        media_parameters()
        {
                audio_param = nullptr;
                video_param = nullptr;
        }
	*/
};

enum media_type {

    SDK_AUDIO = 0x01,
    SDK_VIDEO = 0x1 << 1,
    SDK_DESKSHARE = 0x1 << 2,
    SDK_TRANSCRIPT = 0x1 << 3,
    SDK_CHAT = 0x1 << 4,
    SDK_ALL = 0x1 << 5, 	//means all sessions share a single data connection

};

enum session_event {
	SESSION_ADD,
	SESSION_STOP,
	SESSION_PAUSE,
	SESSION_RESUME,
};

enum user_event {
	USER_JOIN,
	USER_LEAVE,
};

struct rtms_metadata {
	char *user_name;
	int user_id;
};


enum error_code {
	RTMS_SDK_FAILURE = -1,
	RTMS_SDK_OK = 0,
	RTMS_SDK_TIMEOUT,
	RTMS_SDK_NOT_EXIST,
	RTMS_SDK_WRONG_TYPE,
	RTMS_SDK_INVALID_STATUS,
	RTMS_SDK_INVALID_ARGS,
};


#endif //RTMS_COMMON_H
