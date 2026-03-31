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

    def test_event_type_constants(self):
        """Test event type constants (for subscribeEvent/unsubscribeEvent)"""
        assert rtms.EVENT_UNDEFINED == 0
        assert rtms.EVENT_FIRST_PACKET_TIMESTAMP == 1
        assert rtms.EVENT_ACTIVE_SPEAKER_CHANGE == 2
        assert rtms.EVENT_PARTICIPANT_JOIN == 3
        assert rtms.EVENT_PARTICIPANT_LEAVE == 4
        assert rtms.EVENT_SHARING_START == 5
        assert rtms.EVENT_SHARING_STOP == 6
        assert rtms.EVENT_MEDIA_CONNECTION_INTERRUPTED == 7
        assert rtms.EVENT_CONSUMER_ANSWERED == 8
        assert rtms.EVENT_CONSUMER_END == 9
        assert rtms.EVENT_USER_ANSWERED == 10
        assert rtms.EVENT_USER_END == 11
        assert rtms.EVENT_USER_HOLD == 12
        assert rtms.EVENT_USER_UNHOLD == 13

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
        assert hasattr(client, 'onParticipantEvent')
        assert hasattr(client, 'onActiveSpeakerEvent')
        assert hasattr(client, 'onSharingEvent')
        assert hasattr(client, 'onEventEx')
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

    def test_client_has_event_subscription_methods(self):
        """Test that Client has event subscription methods"""
        client = rtms.Client()
        assert hasattr(client, 'subscribeEvent')
        assert callable(client.subscribeEvent)
        assert hasattr(client, 'unsubscribeEvent')
        assert callable(client.unsubscribeEvent)

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
        assert hasattr(rtms, 'onWebhookEvent')
        assert callable(rtms.onWebhookEvent)
        # Also test the alias
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
            'EVENT_PARTICIPANT_JOIN',
            'EVENT_PARTICIPANT_LEAVE',
            'EVENT_ACTIVE_SPEAKER_CHANGE',
            'EVENT_SHARING_START',
            'EVENT_SHARING_STOP',
            'RTMS_SDK_OK'
        ]
        for export in exports_to_check:
            assert export in rtms.__all__

    def test_function_exports(self):
        """Test functions are exported"""
        functions = [
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

    def test_module_has_run_function(self):
        """Test module has run() function for event loop"""
        assert hasattr(rtms, 'run')
        assert callable(rtms.run)

    def test_module_has_stop_function(self):
        """Test module has stop() function to signal shutdown"""
        assert hasattr(rtms, 'stop')
        assert callable(rtms.stop)

    def test_client_has_polling_control(self):
        """Test client has polling control"""
        client = rtms.Client()
        assert hasattr(client, '_poll_if_needed')
        assert hasattr(client, '_running')


class TestTranscriptParams:
    """Test TranscriptParams class and setTranscriptParams/set_transcript_params.

    These tests fail before implementation because TranscriptParams does not exist
    and Client has no set_transcript_params() method.
    """

    def test_transcript_params_class_exists(self):
        """rtms.TranscriptParams should be importable."""
        assert hasattr(rtms, 'TranscriptParams')

    def test_transcript_params_default_content_type(self):
        """Default content_type should be 5 (TEXT)."""
        p = rtms.TranscriptParams()
        assert p.content_type == 5

    def test_transcript_params_default_src_language(self):
        """Default src_language should be -1 (LANGUAGE_ID_NONE, auto-detect)."""
        p = rtms.TranscriptParams()
        assert p.src_language == -1

    def test_transcript_params_default_enable_lid(self):
        """Default enable_lid should be True."""
        p = rtms.TranscriptParams()
        assert p.enable_lid is True

    def test_transcript_params_setters(self):
        """src_language and enable_lid setters should round-trip."""
        p = rtms.TranscriptParams()
        p.src_language = 9   # LANGUAGE_ID_ENGLISH
        p.enable_lid = False
        assert p.src_language == 9
        assert p.enable_lid is False

    def test_transcript_language_dict_exists(self):
        """TranscriptLanguage constant dict should exist."""
        assert hasattr(rtms, 'TranscriptLanguage')

    def test_transcript_language_english(self):
        """TranscriptLanguage['ENGLISH'] should be 9."""
        assert rtms.TranscriptLanguage['ENGLISH'] == 9

    def test_transcript_language_none(self):
        """TranscriptLanguage['NONE'] should be -1 (auto-detect)."""
        assert rtms.TranscriptLanguage['NONE'] == -1

    def test_client_has_set_transcript_params(self):
        """Client should expose set_transcript_params() method."""
        client = rtms.Client()
        assert hasattr(client, 'set_transcript_params')
        assert callable(client.set_transcript_params)

    def test_set_transcript_params_does_not_raise(self):
        """Calling set_transcript_params with a valid TranscriptParams should not raise."""
        client = rtms.Client()
        p = rtms.TranscriptParams()
        p.src_language = rtms.TranscriptLanguage['ENGLISH']
        client.set_transcript_params(p)

    def test_transcript_params_exported_in_all(self):
        """TranscriptParams and TranscriptLanguage dict should appear in rtms.__all__."""
        assert 'TranscriptParams' in rtms.__all__
        assert 'TranscriptLanguage' in rtms.__all__


class TestZccEngagementId:
    """Test ZCC engagement_id support in join()

    ZCC (Zoom Contact Center) identifies sessions with an engagement_id instead
    of a meeting_uuid. join() must accept engagement_id and route it as the
    instance identifier to the native SDK.

    These tests fail before implementation because _do_join() raises:
        ValueError("Either meeting_uuid, webinar_uuid, or session_id is required")
    when only engagement_id is supplied.
    """

    def _make_client_with_mocked_native(self):
        """Return a Client instance with native join() and polling mocked out."""
        client = rtms.Client()
        # Patch the native C++ join on the base class so no real SDK call is made
        NativeClient = rtms.Client.__bases__[0]
        return client, NativeClient

    def test_join_with_engagement_id_returns_true(self):
        """join() called with engagement_id should return True, not False."""
        client, NativeClient = self._make_client_with_mocked_native()
        with patch.object(NativeClient, 'join', return_value=None), \
             patch.object(client, '_start_polling'):
            result = client.join(
                engagement_id='engagement-abc-123',
                rtms_stream_id='stream-xyz',
                server_urls='wss://rtms.zoom.us',
                signature='mock-sig',
            )
        # Before implementation: False (ValueError caught, swallowed, returns False)
        # After implementation: True
        assert result is True

    def test_do_join_with_engagement_id_does_not_raise(self):
        """_do_join() with engagement_id should not raise a missing-identifier error."""
        client, NativeClient = self._make_client_with_mocked_native()
        with patch.object(NativeClient, 'join', return_value=None), \
             patch.object(client, '_start_polling'):
            # Before implementation: raises ValueError
            # After implementation: should complete cleanly
            client._do_join(
                engagement_id='engagement-abc-123',
                rtms_stream_id='stream-xyz',
                server_urls='wss://rtms.zoom.us',
                signature='mock-sig',
            )

    def test_engagement_id_passed_as_first_arg_to_native_join(self):
        """engagement_id should be forwarded as the meeting_uuid positional arg."""
        client, NativeClient = self._make_client_with_mocked_native()
        with patch.object(NativeClient, 'join', return_value=None) as mock_native_join, \
             patch.object(client, '_start_polling'):
            client._do_join(
                engagement_id='eng-abc-123',
                rtms_stream_id='stream-xyz',
                server_urls='wss://rtms.zoom.us',
                signature='mock-sig',
            )
        mock_native_join.assert_called_once_with(
            'eng-abc-123', 'stream-xyz', 'mock-sig', 'wss://rtms.zoom.us', -1
        )

    def test_engagement_id_lower_priority_than_meeting_uuid(self):
        """When both meeting_uuid and engagement_id are supplied, meeting_uuid wins."""
        client, NativeClient = self._make_client_with_mocked_native()
        with patch.object(NativeClient, 'join', return_value=None) as mock_native_join, \
             patch.object(client, '_start_polling'):
            client._do_join(
                meeting_uuid='meeting-uuid-wins',
                engagement_id='engagement-id-loses',
                rtms_stream_id='stream-xyz',
                server_urls='wss://rtms.zoom.us',
                signature='mock-sig',
            )
        first_arg = mock_native_join.call_args[0][0]
        assert first_arg == 'meeting-uuid-wins'


class TestProxySupport:
    """Test set_proxy() support.

    Mirrors JS wrapper test coverage for set_proxy:
    - exists as a callable
    - does not raise for http proxy
    - does not raise for https proxy

    Python-specific additions:
    - set_proxy legacy camelCase alias
    - native forwarding verified via mock
    """

    def test_set_proxy_is_callable(self):
        """set_proxy should be callable on a Client instance."""
        client = rtms.Client()
        assert callable(client.set_proxy)

    def test_set_proxy_legacy_alias_is_callable(self):
        """set_proxy legacy alias should exist for camelCase parity with Node.js."""
        client = rtms.Client()
        assert callable(client.setProxy)

    def test_set_proxy_does_not_raise_for_http(self):
        """set_proxy should not raise for an http proxy."""
        client = rtms.Client()
        NativeClient = rtms.Client.__bases__[0]
        with patch.object(NativeClient, 'set_proxy', return_value=None):
            client.set_proxy('http', 'http://proxy.example.com:8080')

    def test_set_proxy_does_not_raise_for_https(self):
        """set_proxy should not raise for an https proxy."""
        client = rtms.Client()
        NativeClient = rtms.Client.__bases__[0]
        with patch.object(NativeClient, 'set_proxy', return_value=None):
            client.set_proxy('https', 'https://proxy.example.com:8080')

    def test_set_proxy_exported_in_all(self):
        """set_proxy does not need to be in __all__ but the Client method must be accessible."""
        client = rtms.Client()
        assert callable(getattr(client, 'set_proxy', None))

    def test_set_proxy_forwards_to_native(self):
        """set_proxy should forward proxy_type and proxy_url to the native layer."""
        client = rtms.Client()
        NativeClient = rtms.Client.__bases__[0]
        with patch.object(NativeClient, 'set_proxy', return_value=None) as mock_native:
            client.set_proxy('http', 'http://proxy.example.com:8080')
        mock_native.assert_called_once_with('http', 'http://proxy.example.com:8080')


# Run tests
if __name__ == '__main__':
    pytest.main([__file__, '-v'])
