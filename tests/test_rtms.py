#!/usr/bin/env python3
"""
Comprehensive test suite for Python RTMS SDK

Mirrors the Node.js test coverage in tests/rtms.test.ts
"""

import pytest
import rtms
from unittest.mock import Mock, patch, MagicMock
import json


class TestConstants:
    """Test that all constants are properly defined"""

    def test_media_type_constants(self):
        """Test media type constants match expected values"""
        assert rtms.MEDIA_TYPE_AUDIO == 1
        assert rtms.MEDIA_TYPE_VIDEO == 2
        assert rtms.MEDIA_TYPE_DESKSHARE == 4
        assert rtms.MEDIA_TYPE_TRANSCRIPT == 8
        assert rtms.MEDIA_TYPE_CHAT == 16
        assert rtms.MEDIA_TYPE_ALL == 32

    def test_session_event_constants(self):
        """Test session event constants"""
        assert rtms.SESSION_EVENT_ADD == 1
        assert rtms.SESSION_EVENT_STOP == 2
        assert rtms.SESSION_EVENT_PAUSE == 3
        assert rtms.SESSION_EVENT_RESUME == 4

    def test_user_event_constants(self):
        """Test user event constants"""
        assert rtms.USER_EVENT_JOIN == 0
        assert rtms.USER_EVENT_LEAVE == 1

    def test_status_constants(self):
        """Test SDK status constants"""
        assert rtms.RTMS_SDK_FAILURE == -1
        assert rtms.RTMS_SDK_OK == 0
        assert rtms.RTMS_SDK_TIMEOUT == 1
        assert rtms.RTMS_SDK_NOT_EXIST == 2

    def test_audio_codec_constants(self):
        """Test audio codec constants exist"""
        assert 'OPUS' in rtms.AudioCodec
        assert 'L16' in rtms.AudioCodec
        assert 'G711' in rtms.AudioCodec

    def test_video_codec_constants(self):
        """Test video codec constants exist"""
        assert 'H264' in rtms.VideoCodec
        assert 'JPG' in rtms.VideoCodec

    def test_sample_rate_constants(self):
        """Test audio sample rate constants"""
        assert 'SR_48K' in rtms.AudioSampleRate
        assert 'SR_16K' in rtms.AudioSampleRate
        assert 'SR_8K' in rtms.AudioSampleRate


class TestLogging:
    """Test logging functionality"""

    def test_log_levels_enum(self):
        """Test LogLevel enum values"""
        assert rtms.LogLevel.ERROR == 0
        assert rtms.LogLevel.WARN == 1
        assert rtms.LogLevel.INFO == 2
        assert rtms.LogLevel.DEBUG == 3
        assert rtms.LogLevel.TRACE == 4

    def test_log_format_enum(self):
        """Test LogFormat enum values"""
        assert rtms.LogFormat.PROGRESSIVE == 'progressive'
        assert rtms.LogFormat.JSON == 'json'

    def test_configure_logger(self):
        """Test logger configuration"""
        # Should not raise errors
        rtms.configure_logger({
            'level': 'debug',
            'format': 'json',
            'enabled': True
        })

        rtms.configure_logger({
            'level': 'info',
            'format': 'progressive',
            'enabled': False
        })

    def test_log_functions_exist(self):
        """Test that all log functions are available"""
        assert callable(rtms.log_debug)
        assert callable(rtms.log_info)
        assert callable(rtms.log_warn)
        assert callable(rtms.log_error)


class TestClient:
    """Test Client class functionality"""

    def test_client_instantiation(self):
        """Test that Client can be instantiated"""
        client = rtms.Client()
        assert client is not None
        assert isinstance(client, rtms.Client)

    def test_client_has_join_method(self):
        """Test that Client has join method"""
        client = rtms.Client()
        assert hasattr(client, 'join')
        assert callable(client.join)

    def test_client_has_leave_method(self):
        """Test that Client has leave method"""
        client = rtms.Client()
        assert hasattr(client, 'leave')
        assert callable(client.leave)

    def test_client_has_callback_methods(self):
        """Test that Client has all callback registration methods"""
        client = rtms.Client()
        assert hasattr(client, 'onJoinConfirm')
        assert hasattr(client, 'onSessionUpdate')
        assert hasattr(client, 'onUserUpdate')
        assert hasattr(client, 'onAudioData')
        assert hasattr(client, 'onVideoData')
        assert hasattr(client, 'onDeskshareData')
        assert hasattr(client, 'onTranscriptData')
        assert hasattr(client, 'onLeave')

    def test_client_has_param_methods(self):
        """Test that Client has parameter configuration methods"""
        client = rtms.Client()
        assert hasattr(client, 'setAudioParams')
        assert hasattr(client, 'setVideoParams')
        assert hasattr(client, 'setDeskshareParams')

    def test_client_callback_registration(self):
        """Test callback registration doesn't raise errors"""
        client = rtms.Client()

        @client.onJoinConfirm
        def on_join(reason):
            pass

        @client.onTranscriptData
        def on_transcript(data, size, timestamp, metadata):
            pass

        # Should complete without errors
        assert True


class TestParameterValidation:
    """Test parameter validation functionality"""

    def test_audio_params_exist(self):
        """Test AudioParams class exists"""
        assert hasattr(rtms, 'AudioParams')

    def test_video_params_exist(self):
        """Test VideoParams class exists"""
        assert hasattr(rtms, 'VideoParams')

    def test_deskshare_params_exist(self):
        """Test DeskshareParams class exists"""
        assert hasattr(rtms, 'DeskshareParams')


class TestGlobalClient:
    """Test global client singleton functionality"""

    def test_global_join_exists(self):
        """Test global join function exists"""
        assert hasattr(rtms, 'join')
        assert callable(rtms.join)

    def test_global_leave_exists(self):
        """Test global leave function exists"""
        assert hasattr(rtms, 'leave')
        assert callable(rtms.leave)

    def test_global_callback_decorators_exist(self):
        """Test global callback decorators exist"""
        assert hasattr(rtms, 'onJoinConfirm')
        assert hasattr(rtms, 'onSessionUpdate')
        assert hasattr(rtms, 'onUserUpdate')
        assert hasattr(rtms, 'onAudioData')
        assert hasattr(rtms, 'onVideoData')
        assert hasattr(rtms, 'onTranscriptData')
        assert hasattr(rtms, 'onLeave')

    def test_global_param_functions_exist(self):
        """Test global parameter setting functions exist"""
        assert hasattr(rtms, 'setAudioParams')
        assert hasattr(rtms, 'setVideoParams')
        assert hasattr(rtms, 'setDeskshareParams')


class TestUtilityFunctions:
    """Test utility functions"""

    def test_generate_signature_exists(self):
        """Test signature generation function exists"""
        assert hasattr(rtms, 'generate_signature')
        assert callable(rtms.generate_signature)

    def test_find_ca_certificate_exists(self):
        """Test CA certificate finder exists"""
        assert hasattr(rtms, 'find_ca_certificate')
        assert callable(rtms.find_ca_certificate)

    def test_initialize_exists(self):
        """Test initialize function exists"""
        assert hasattr(rtms, 'initialize')
        assert callable(rtms.initialize)

    def test_uninitialize_exists(self):
        """Test uninitialize function exists"""
        assert hasattr(rtms, 'uninitialize')
        assert callable(rtms.uninitialize)


class TestWebhookFunctionality:
    """Test webhook-related functionality"""

    def test_on_webhook_event_exists(self):
        """Test webhook event decorator exists"""
        assert hasattr(rtms, 'on_webhook_event')
        assert callable(rtms.on_webhook_event)

    def test_webhook_response_class_exists(self):
        """Test WebhookResponse class exists"""
        # WebhookResponse is internal but should be importable
        from rtms import WebhookResponse
        assert WebhookResponse is not None

    def test_client_webhook_decorator(self):
        """Test client webhook event decorator"""
        client = rtms.Client()
        assert hasattr(client, 'on_webhook_event')
        assert callable(client.on_webhook_event)

        # Test decorator usage
        @client.on_webhook_event()
        def handle_webhook(payload):
            pass

        assert True


class TestDataClasses:
    """Test data classes and structures"""

    def test_session_class_exists(self):
        """Test Session class exists"""
        assert hasattr(rtms, 'Session')

    def test_participant_class_exists(self):
        """Test Participant class exists"""
        assert hasattr(rtms, 'Participant')

    def test_metadata_class_exists(self):
        """Test Metadata class exists"""
        assert hasattr(rtms, 'Metadata')


class TestModuleExports:
    """Test that all expected exports are available"""

    def test_client_export(self):
        """Test Client is exported"""
        assert 'Client' in rtms.__all__

    def test_constant_exports(self):
        """Test constants are exported"""
        exports_to_check = [
            'MEDIA_TYPE_AUDIO',
            'MEDIA_TYPE_VIDEO',
            'MEDIA_TYPE_TRANSCRIPT',
            'SESSION_EVENT_ADD',
            'USER_EVENT_JOIN',
            'RTMS_SDK_OK'
        ]
        for export in exports_to_check:
            assert export in rtms.__all__

    def test_function_exports(self):
        """Test functions are exported"""
        functions = [
            'join',
            'leave',
            'initialize',
            'uninitialize',
            'generate_signature',
            'configure_logger'
        ]
        for func in functions:
            assert func in rtms.__all__

    def test_enum_exports(self):
        """Test enums are exported"""
        enums = [
            'AudioCodec',
            'VideoCodec',
            'AudioSampleRate',
            'VideoResolution',
            'LogLevel',
            'LogFormat'
        ]
        for enum in enums:
            assert enum in rtms.__all__


class TestThreadSafety:
    """Test thread-safe features"""

    def test_client_has_join_queue(self):
        """Test client has join queue for thread safety"""
        client = rtms.Client()
        assert hasattr(client, '_join_queue')
        assert hasattr(client, '_process_join_queue')

    def test_client_has_polling_control(self):
        """Test client has polling control"""
        client = rtms.Client()
        assert hasattr(client, '_poll_if_needed')
        assert hasattr(client, '_running')


# Run tests
if __name__ == '__main__':
    pytest.main([__file__, '-v'])
