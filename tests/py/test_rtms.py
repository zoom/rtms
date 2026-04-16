#!/usr/bin/env python3
"""
Comprehensive test suite for Python RTMS SDK

Mirrors the Node.js test coverage in tests/rtms.test.ts
"""

import pytest
import rtms
from unittest.mock import Mock, patch, MagicMock
import json
import asyncio
import inspect


@pytest.fixture(autouse=True)
def clean_rtms_state():
    """Clear global rtms state before each test.

    Prevents clients created in earlier tests from leaking into later tests
    via the _clients registry, which would cause run_async(stop_on_empty=True)
    to never exit and _cleanup_all_clients() to block on unjoined C++ clients.
    """
    with rtms._clients_lock:
        rtms._clients.clear()
    rtms._running = False
    rtms._stop_event.clear()
    yield


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
        assert rtms.EVENT_PARTICIPANT_VIDEO_ON == 8
        assert rtms.EVENT_PARTICIPANT_VIDEO_OFF == 9
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
        assert 'OPUS' in rtms.AudioCodec.__members__
        assert 'L16' in rtms.AudioCodec.__members__
        assert 'G711' in rtms.AudioCodec.__members__

    def test_video_codec_constants(self):
        """Test video codec constants exist"""
        assert 'H264' in rtms.VideoCodec.__members__
        assert 'JPG' in rtms.VideoCodec.__members__

    def test_sample_rate_constants(self):
        """Test audio sample rate constants"""
        assert 'SR_48K' in rtms.AudioSampleRate.__members__
        assert 'SR_16K' in rtms.AudioSampleRate.__members__
        assert 'SR_8K' in rtms.AudioSampleRate.__members__


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

    def test_client_has_individual_video_methods(self):
        """Test that Client has individual video stream methods"""
        client = rtms.Client()
        assert hasattr(client, 'subscribe_video')
        assert callable(client.subscribe_video)
        assert hasattr(client, 'subscribeVideo')   # camelCase alias
        assert hasattr(client, 'on_participant_video')
        assert callable(client.on_participant_video)
        assert hasattr(client, 'onParticipantVideo')
        assert hasattr(client, 'on_video_subscribed')
        assert callable(client.on_video_subscribed)
        assert hasattr(client, 'onVideoSubscribed')

    def test_on_participant_video_accepts_callback(self):
        """Test that on_participant_video registers a callback without error"""
        client = rtms.Client()
        client.on_participant_video(lambda user_ids, is_on: None)

    def test_on_video_subscribed_accepts_callback(self):
        """Test that on_video_subscribed registers a callback without error"""
        client = rtms.Client()
        client.on_video_subscribed(lambda user_id, status, error: None)
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
        """Test client has EventLoop integration attributes"""
        client = rtms.Client()
        assert hasattr(client, '_do_alloc_and_join')
        assert hasattr(client, '_pending_join_params')


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
        # join() stores params and returns True immediately (defers to EventLoop)
        with patch.object(client, '_do_alloc_and_join'):
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
        """_do_alloc_and_join() with engagement_id should not raise a missing-identifier error."""
        client, NativeClient = self._make_client_with_mocked_native()
        client._pending_join_params = {
            'engagement_id': 'engagement-abc-123',
            'rtms_stream_id': 'stream-xyz',
            'server_urls': 'wss://rtms.zoom.us',
            'signature': 'mock-sig',
            'timeout': -1,
        }
        with patch.object(NativeClient, 'join', return_value=None), \
             patch.object(NativeClient, 'alloc', return_value=None):
            # Before implementation: raises ValueError
            # After implementation: should complete cleanly
            client._do_alloc_and_join()

    def test_engagement_id_passed_as_first_arg_to_native_join(self):
        """engagement_id should be forwarded as the meeting_uuid positional arg."""
        client, NativeClient = self._make_client_with_mocked_native()
        client._pending_join_params = {
            'engagement_id': 'eng-abc-123',
            'rtms_stream_id': 'stream-xyz',
            'server_urls': 'wss://rtms.zoom.us',
            'signature': 'mock-sig',
            'timeout': -1,
        }
        with patch.object(NativeClient, 'join', return_value=None) as mock_native_join, \
             patch.object(NativeClient, 'alloc', return_value=None):
            client._do_alloc_and_join()
        mock_native_join.assert_called_once_with(
            'eng-abc-123', 'stream-xyz', 'mock-sig', 'wss://rtms.zoom.us', -1
        )

    def test_engagement_id_lower_priority_than_meeting_uuid(self):
        """When both meeting_uuid and engagement_id are supplied, meeting_uuid wins."""
        client, NativeClient = self._make_client_with_mocked_native()
        client._pending_join_params = {
            'meeting_uuid': 'meeting-uuid-wins',
            'engagement_id': 'engagement-id-loses',
            'rtms_stream_id': 'stream-xyz',
            'server_urls': 'wss://rtms.zoom.us',
            'signature': 'mock-sig',
            'timeout': -1,
        }
        with patch.object(NativeClient, 'join', return_value=None) as mock_native_join, \
             patch.object(NativeClient, 'alloc', return_value=None):
            client._do_alloc_and_join()
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


class TestIndividualVideoSubscription:
    """Test subscribe_video() and on_participant_video / on_video_subscribed callbacks.

    These tests fail before implementation because:
    - Client has no subscribe_video() / subscribeVideo() method
    - Client has no on_participant_video() / on_video_subscribed() callback registration

    Per spec (branch 5):
    - subscribe_video(user_id, True)  → subscribe to individual participant stream
    - subscribe_video(user_id, False) → unsubscribe
    - on_participant_video(callback)  → fires when participant video state changes
                                        callback signature: (users: List[int], is_on: bool)
    - on_video_subscribed(callback)   → fires in response to subscribe_video()
                                        callback signature: (user_id: int, status: int, error: str)
    """

    def _native(self):
        return rtms.Client.__bases__[0]

    # ------------------------------------------------------------------
    # subscribe_video method

    def test_subscribe_video_is_callable(self):
        """subscribe_video should be callable on a Client instance."""
        client = rtms.Client()
        assert callable(getattr(client, 'subscribe_video', None))

    def test_subscribe_video_camelcase_alias_is_callable(self):
        """subscribeVideo camelCase alias must exist for parity with Node.js."""
        client = rtms.Client()
        assert callable(getattr(client, 'subscribeVideo', None))

    def test_subscribe_video_does_not_raise_for_subscribe(self):
        """subscribe_video(user_id, True) should not raise."""
        client = rtms.Client()
        with patch.object(self._native(), 'subscribe_video', return_value=None):
            client.subscribe_video(12345, True)

    def test_subscribe_video_does_not_raise_for_unsubscribe(self):
        """subscribe_video(user_id, False) should not raise."""
        client = rtms.Client()
        with patch.object(self._native(), 'subscribe_video', return_value=None):
            client.subscribe_video(12345, False)

    def test_subscribe_video_forwards_to_native(self):
        """subscribe_video should forward user_id and subscribe flag to native layer."""
        client = rtms.Client()
        with patch.object(self._native(), 'subscribe_video', return_value=None) as mock_native:
            client.subscribe_video(99999, True)
        mock_native.assert_called_once_with(99999, True)

    def test_subscribe_video_unsubscribe_forwards_to_native(self):
        """subscribe_video(user_id, False) should forward False to native layer."""
        client = rtms.Client()
        with patch.object(self._native(), 'subscribe_video', return_value=None) as mock_native:
            client.subscribe_video(99999, False)
        mock_native.assert_called_once_with(99999, False)

    # ------------------------------------------------------------------
    # on_participant_video callback

    def test_on_participant_video_is_callable(self):
        """on_participant_video should be callable on a Client instance."""
        client = rtms.Client()
        assert callable(getattr(client, 'on_participant_video', None))

    def test_on_participant_video_camelcase_alias_is_callable(self):
        """onParticipantVideo camelCase alias must exist for Node.js parity."""
        client = rtms.Client()
        assert callable(getattr(client, 'onParticipantVideo', None))

    def test_on_participant_video_does_not_raise(self):
        """Registering an on_participant_video callback should not raise."""
        client = rtms.Client()
        client.on_participant_video(lambda *_: None)

    def test_on_participant_video_callback_receives_users_and_flag(self):
        """on_participant_video callback must receive a list of user IDs and a bool."""
        client = rtms.Client()
        received = {}

        def cb(users, is_on):
            received['users'] = users
            received['is_on'] = is_on

        client.on_participant_video(cb)
        # Simulate the callback being fired by the native layer
        client._participant_video_callback([11111, 22222], True)
        assert received['users'] == [11111, 22222]
        assert received['is_on'] is True

    # ------------------------------------------------------------------
    # on_video_subscribed callback

    def test_on_video_subscribed_is_callable(self):
        """on_video_subscribed should be callable on a Client instance."""
        client = rtms.Client()
        assert callable(getattr(client, 'on_video_subscribed', None))

    def test_on_video_subscribed_camelcase_alias_is_callable(self):
        """onVideoSubscribed camelCase alias must exist for Node.js parity."""
        client = rtms.Client()
        assert callable(getattr(client, 'onVideoSubscribed', None))

    def test_on_video_subscribed_does_not_raise(self):
        """Registering an on_video_subscribed callback should not raise."""
        client = rtms.Client()
        client.on_video_subscribed(lambda *_: None)

    def test_on_video_subscribed_callback_receives_userid_status_error(self):
        """on_video_subscribed callback must receive user_id (int), status (int), error (str)."""
        client = rtms.Client()
        received = {}

        def cb(user_id, status, error):
            received['user_id'] = user_id
            received['status'] = status
            received['error'] = error

        client.on_video_subscribed(cb)
        # Simulate the callback being fired by the native layer
        client._video_subscribed_callback(12345, 0, '')
        assert received['user_id'] == 12345
        assert received['status'] == 0
        assert received['error'] == ''

    def test_on_video_subscribed_callback_reports_error_string(self):
        """on_video_subscribed callback should relay a non-empty error string."""
        client = rtms.Client()
        received = {}

        def cb(user_id, status, error):
            received['user_id'] = user_id
            received['status'] = status
            received['error'] = error

        client.on_video_subscribed(cb)
        client._video_subscribed_callback(12345, -1, 'subscription failed')
        assert received['status'] == -1
        assert received['error'] == 'subscription failed'


class TestGilRelease:
    """Tests that poll() releases the GIL, allowing other Python threads to run concurrently."""

    def test_poll_exists_on_client(self):
        client = rtms.Client()
        assert hasattr(client, 'poll')

    def test_poll_callable(self):
        client = rtms.Client()
        # Should not raise (SDK errors are OK; what matters is it doesn't hang)
        try:
            client.poll()
        except Exception:
            pass

    def test_poll_does_not_block_other_threads(self):
        """Smoke test: poll() on an unjoined client returns without hanging.

        NOTE: On an unjoined client with no active meeting, poll() returns in
        microseconds — too fast to observe true GIL-release concurrency. This
        test verifies poll() is callable and doesn't hang; it does NOT prove
        GIL release in a loaded scenario.

        True GIL-release verification requires either a live meeting (where
        poll() blocks waiting for SDK events) or a mock SDK with a sleep().
        The structural guarantee lives in src/python.cpp: py::gil_scoped_release.
        """
        import threading
        results = []

        def worker():
            results.append(threading.get_ident())

        client = rtms.Client()
        t = threading.Thread(target=worker)
        t.start()
        try:
            client.poll()
        except Exception:
            pass
        t.join(timeout=2.0)
        assert len(results) == 1, "Worker thread should have completed"


class TestRunAsync:
    """Tests the new run_async() asyncio-native event loop."""

    def test_run_async_exists(self):
        assert hasattr(rtms, 'run_async')

    def test_run_async_is_coroutine_function(self):
        assert inspect.iscoroutinefunction(rtms.run_async)

    def test_run_async_accepts_poll_interval(self):
        sig = inspect.signature(rtms.run_async)
        assert 'poll_interval' in sig.parameters

    def test_run_async_accepts_stop_on_empty(self):
        sig = inspect.signature(rtms.run_async)
        assert 'stop_on_empty' in sig.parameters

    def test_run_async_stops_when_stop_called(self):
        async def run_and_stop():
            loop = asyncio.get_event_loop()
            loop.call_later(0.05, rtms.stop)
            await rtms.run_async(poll_interval=0.01)

        asyncio.run(run_and_stop())  # Should complete, not hang

    def test_run_async_stop_on_empty_exits(self):
        async def run_empty():
            await rtms.run_async(stop_on_empty=True, poll_interval=0.01)

        asyncio.run(run_empty())  # No clients registered → exits immediately

    def test_run_async_composes_with_gather(self):
        """run_async() should work inside asyncio.gather without blocking."""
        results = []

        async def side_task():
            results.append('side_task_ran')

        async def test():
            await asyncio.gather(
                rtms.run_async(stop_on_empty=True, poll_interval=0.01),
                side_task(),
            )

        asyncio.run(test())
        assert 'side_task_ran' in results


class TestExecutorSupport:
    """Tests executor-based callback dispatch."""

    def test_client_accepts_executor_kwarg(self):
        from concurrent.futures import ThreadPoolExecutor
        executor = ThreadPoolExecutor(max_workers=2)
        client = rtms.Client(executor=executor)
        assert client is not None
        executor.shutdown(wait=False)

    def test_client_stores_executor(self):
        from concurrent.futures import ThreadPoolExecutor
        executor = ThreadPoolExecutor(max_workers=2)
        client = rtms.Client(executor=executor)
        assert client._executor is executor
        executor.shutdown(wait=False)

    def test_client_no_executor_default(self):
        client = rtms.Client()
        assert client._executor is None

    def test_run_accepts_executor_kwarg(self):
        sig = inspect.signature(rtms.run)
        assert 'executor' in sig.parameters

    def test_run_executor_default_is_none(self):
        sig = inspect.signature(rtms.run)
        assert sig.parameters['executor'].default is None

    def test_wrap_callback_exists(self):
        client = rtms.Client()
        assert hasattr(client, '_wrap_callback')

    def test_sync_callback_no_executor_passthrough(self):
        """Sync callback with no executor should be returned unchanged."""
        client = rtms.Client()

        def sync_cb(*_):
            pass

        wrapped = client._wrap_callback(sync_cb)
        assert wrapped is sync_cb, "No executor → callback passed through unchanged"

    def test_executor_wraps_sync_callback(self):
        """When executor is set, sync callbacks are wrapped for thread pool dispatch."""
        from concurrent.futures import ThreadPoolExecutor
        executor = ThreadPoolExecutor(max_workers=1)
        client = rtms.Client(executor=executor)

        def audio_cb(*_):
            pass

        wrapped = client._wrap_callback(audio_cb)
        assert wrapped is not audio_cb, "Executor wrapping should produce a different callable"
        executor.shutdown(wait=False)

    def test_async_callback_detected_and_wrapped(self):
        """Async coroutine callbacks should be auto-wrapped for event loop dispatch."""
        client = rtms.Client()

        async def async_cb(*_):
            pass

        wrapped = client._wrap_callback(async_cb)
        assert wrapped is not async_cb, "Async callback should be wrapped"

    def test_executor_applied_to_video_data_callback(self):
        from concurrent.futures import ThreadPoolExecutor
        executor = ThreadPoolExecutor(max_workers=1)
        client = rtms.Client(executor=executor)

        def video_cb(*_):
            pass

        wrapped = client._wrap_callback(video_cb)
        assert wrapped is not video_cb
        executor.shutdown(wait=False)


class TestContextManager:
    """Tests `with rtms.Client() as client:` protocol."""

    def test_client_has_enter(self):
        assert hasattr(rtms.Client, '__enter__')

    def test_client_has_exit(self):
        assert hasattr(rtms.Client, '__exit__')

    def test_enter_returns_client(self):
        client = rtms.Client()
        result = client.__enter__()
        assert result is client

    def test_exit_calls_leave(self):
        client = rtms.Client()
        with patch.object(client, 'leave') as mock_leave:
            client.__exit__(None, None, None)
        mock_leave.assert_called_once()

    def test_with_statement_basic(self):
        """with rtms.Client() as client: should work without error."""
        with patch.object(rtms.Client, 'leave'):
            with rtms.Client() as client:
                assert client is not None

    def test_exit_called_on_exception(self):
        """leave() should be called even when an exception is raised inside with block."""
        leave_called = []

        class TrackingClient(rtms.Client):
            def leave(self):
                leave_called.append(True)
                return True

        client = TrackingClient()
        try:
            with client:
                raise ValueError("test exception")
        except ValueError:
            pass

        assert len(leave_called) == 1, "leave() should be called on exception"

    def test_exit_does_not_suppress_exceptions(self):
        """__exit__ should return False/None so exceptions propagate."""
        client = rtms.Client()
        with patch.object(client, 'leave'):
            result = client.__exit__(ValueError, ValueError("test"), None)
        assert not result  # False or None — do not suppress

    def test_context_manager_integration(self):
        """Full with-statement: enter returns client, leave called on exit."""
        leave_called = []

        with patch.object(rtms.Client, 'leave', side_effect=lambda: leave_called.append(True) or True):
            with rtms.Client() as client:
                assert client is not None

        assert len(leave_called) == 1


class TestSnakeCaseCallbacks:
    """snake_case callback methods are canonical; camelCase are backward-compat aliases."""

    def test_on_audio_data_canonical(self):
        client = rtms.Client()
        client.on_audio_data(lambda *_: None)

    def test_on_video_data_canonical(self):
        client = rtms.Client()
        client.on_video_data(lambda *_: None)

    def test_on_transcript_data_canonical(self):
        client = rtms.Client()
        client.on_transcript_data(lambda *_: None)

    def test_on_deskshare_data_canonical(self):
        client = rtms.Client()
        client.on_deskshare_data(lambda *_: None)

    def test_on_join_confirm_canonical(self):
        client = rtms.Client()
        client.on_join_confirm(lambda _: None)

    def test_on_session_update_canonical(self):
        client = rtms.Client()
        client.on_session_update(lambda *_: None)

    def test_on_user_update_canonical(self):
        client = rtms.Client()
        client.on_user_update(lambda *_: None)

    def test_on_leave_canonical(self):
        client = rtms.Client()
        client.on_leave(lambda _: None)

    def test_on_event_ex_canonical(self):
        client = rtms.Client()
        client.on_event_ex(lambda _: None)

    def test_on_participant_event_canonical(self):
        client = rtms.Client()
        with patch.object(client, 'subscribeEvent'):
            client.on_participant_event(lambda *_: None)

    def test_on_active_speaker_event_canonical(self):
        client = rtms.Client()
        with patch.object(client, 'subscribeEvent'):
            client.on_active_speaker_event(lambda *_: None)

    def test_on_sharing_event_canonical(self):
        client = rtms.Client()
        with patch.object(client, 'subscribeEvent'):
            client.on_sharing_event(lambda *_: None)

    def test_on_media_connection_interrupted_canonical(self):
        client = rtms.Client()
        with patch.object(client, 'subscribeEvent'):
            client.on_media_connection_interrupted(lambda *_: None)

    def test_onAudioData_alias_works(self):
        client = rtms.Client()
        client.onAudioData(lambda *_: None)

    def test_onVideoData_alias_works(self):
        client = rtms.Client()
        client.onVideoData(lambda *_: None)

    def test_onLeave_alias_works(self):
        client = rtms.Client()
        client.onLeave(lambda _: None)

    def test_camelCase_is_alias_for_snake_case(self):
        """camelCase names should be Python-level aliases pointing to the same function as snake_case.

        After fix: on_audio_data is defined in __init__.py and onAudioData = on_audio_data.
        At the class level, they should be identical objects.
        """
        assert rtms.Client.onAudioData is rtms.Client.on_audio_data


# Run tests
if __name__ == '__main__':
    pytest.main([__file__, '-v'])
