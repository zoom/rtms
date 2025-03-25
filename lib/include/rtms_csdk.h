#ifndef RTMS_CSDK_H
#define RTMS_CSDK_H

#include "rtms_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rtms_csdk {
	char *meeting_uuid;
	char *rtms_stream_id;
};

typedef void (*fn_on_join_confirm)(struct rtms_csdk *sdk, int reason);
typedef void (*fn_on_session_update)(struct rtms_csdk *sdk, int op, struct session_info *sess);
typedef void (*fn_on_user_update)(struct rtms_csdk *sdk, int op, struct participant_info *pi);
typedef void (*fn_on_audio_data)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md);
typedef void (*fn_on_video_data)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, const char *rtms_session_id, struct rtms_metadata *md);
typedef void (*fn_on_transcript_data)(struct rtms_csdk *sdk, unsigned char *buf, int size, unsigned int timestamp, struct rtms_metadata *md);
typedef void (*fn_on_leave)(struct rtms_csdk *sdk, int reason);

struct rtms_csdk_ops {
                fn_on_join_confirm on_join_confirm;
                fn_on_session_update on_session_update;
                fn_on_user_update on_user_update;
                fn_on_audio_data on_audio_data;
                fn_on_video_data on_video_data;
                fn_on_transcript_data on_transcript_data;
                fn_on_leave on_leave;
};

int rtms_init(const char *ca_path);
struct rtms_csdk *rtms_alloc();

//ale: application layer encryption, that is if media payload will be encrypted. by default it's disabled for websocket protocol. it's not applied to UDP which always enabled.
int rtms_config(struct rtms_csdk *sdk, struct media_parameters *param, int media_types, int ale);
int rtms_set_callbacks(struct rtms_csdk *sdk, struct rtms_csdk_ops *sdk_ops);

//timeout - milliseconds, -1 means use defalut value in rtms SDK which is 10000 
int rtms_join(struct rtms_csdk *sdk, const char *meeting_uuid, const char *rtms_stream_id, const char *signature, const char *server_url, int timeout);
int rtms_poll(struct rtms_csdk *sdk);
//int rtms_leave(struct rtms_csdk *sdk, int reason);
int rtms_release(struct rtms_csdk *sdk);
void rtms_uninit(void);

#ifdef __cplusplus
}
#endif

#endif //RTMS_CSDK_H
