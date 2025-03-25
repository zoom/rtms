#ifndef __RTMS_SDK_H__
#define __RTMS_SDK_H__

#include <list>
#include <mutex>
using namespace std;

#include "rtms_common.h"

class rtms_sdk_sink
{
	public:
		virtual void on_join_confirm(int reason) = 0;
		virtual void on_session_update(int op, struct session_info *sess) {};
		virtual void on_user_update(int op, struct participant_info *pi) {} 
		virtual void on_audio_data(unsigned char *data_buf, int size, unsigned int timestamp, struct rtms_metadata *md) = 0;
		virtual void on_video_data(unsigned char *data_buf, int size, unsigned int timestamp, const char *rtms_session_id, struct rtms_metadata *md) {}
		virtual void on_transcript_data(unsigned char *data_buf, int size, unsigned int timestamp, struct rtms_metadata *md) {} 
		virtual void on_leave(int reason) = 0;

	protected:
        	virtual ~rtms_sdk_sink() {}
};

class rtms_sdk;

class rtms_sdk_provider
{
	public:
		static rtms_sdk_provider *instance();
		int init(const char *ca_path);
		rtms_sdk *create_sdk();
		int release_sdk(rtms_sdk *);
		void uninit(void);

	protected:
		rtms_sdk_provider();
		~rtms_sdk_provider();

		static rtms_sdk_provider *s_inst;
	       	static mutex mtx;	
};

class rtms_sdk_impl;
class rtms_sdk
{
	public:
		rtms_sdk();
		~rtms_sdk();
	
		int open(rtms_sdk_sink *);
		int config(struct media_parameters *, int media_types, int ale);
		//timeout: milliseconds. -1 means use default value 10000 in rtms SDK.
		int join(const char *meeting_uuid, const char *rtms_stream_id, const char *signature, const char *server_url, int timeout);
		int leave(int reason);
		int poll(void);

	private:
		rtms_sdk_impl *m_impl;
};


/*
#ifdef __cplusplus
extern "C" {
#endif

	struct rtms_csdk {
	};

	struct rtms_sdk_ops {

		void (*on_join_confirm)(struct rtms_csdk *sdk, int reason);
		void (*on_session_update)(struct rtms_csdk *sdk, struct session_info *sess[], int size);
	       	void (*on_audio_data)(struct rtms_csdk *sdk, unsigned char *buf, int size, char *user_name);
		void (*on_video_data)(struct rtms_csdk *sdk, unsigned char *buf, int size, char *rtms_session_id, char *user_name);
		void (*on_transcript_data)(struct rtms_csdk *sdk, unsigned char *buf, int size, char *user_name);

	};

	struct rtms_csdk *rtms_alloc();
	int rtms_config(struct rtms_csdk *sdk, struct media_parameters *param);
	int rtms_set_callbacks(struct rtms_csdk *sdk, struct rtms_sdk_ops *sdk_ops);
	int rtms_join(struct rtms_csdk *sdk, char *meeting_uuid, char *rtms_session_id, char *signature, char *server_url);
	int rtms_poll(void);
	//int rtms_leave(struct rtms_csdk *sdk, int reason);
	int rtms_release(struct rtms_csdk *sdk);


#ifdef __cplusplus
}
#endif

*/


#endif //__RTMS_SDK_H__
