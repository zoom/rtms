/**
 * Integration tests for the index.ts TypeScript wrapper layer.
 *
 * Unlike rtms.test.ts (which mocks the entire module), these tests run against
 * the real built ESM output (build/Release/index.js) via child node processes.
 * This provides a genuine red-green TDD cycle: methods missing from index.ts
 * cause failures here even if the mock in rtms.test.ts is already wired.
 *
 * Requires build/Release/index.js to exist (task build:js).
 */

import { execSync } from 'child_process';
import { existsSync } from 'fs';
import * as path from 'path';

const BUILD = path.resolve(__dirname, '../../build/Release/index.js');

// The built module uses a default export. Prefix every eval with this preamble.
const PREAMBLE = `import * as _mod from '${BUILD}'; const rtms = _mod.default;`;

/** Run an expression with a Client instance; returns true if exit code 0. */
function run(expr: string): boolean {
  try {
    execSync(
      `node --input-type=module --eval "${PREAMBLE} const c = new rtms.Client(); process.exit((${expr}) ? 0 : 1);"`,
      { stdio: 'pipe' }
    );
    return true;
  } catch {
    return false;
  }
}

/** Run an expression at module level (no Client). */
function runModule(expr: string): boolean {
  try {
    execSync(
      `node --input-type=module --eval "${PREAMBLE} process.exit((${expr}) ? 0 : 1);"`,
      { stdio: 'pipe' }
    );
    return true;
  } catch {
    return false;
  }
}

describe('index.ts wrapper — real built module', () => {
  beforeAll(() => {
    if (!existsSync(BUILD)) {
      throw new Error(`Build output not found: ${BUILD}\nRun 'task build:js' first.`);
    }
  });

  // --------------------------------------------------------------------------
  describe('Client — connection methods', () => {
    test('join is a function', () => {
      expect(run("typeof c.join === 'function'")).toBe(true);
    });

    test('leave is a function', () => {
      expect(run("typeof c.leave === 'function'")).toBe(true);
    });
  });

  // --------------------------------------------------------------------------
  describe('Client — parameter methods', () => {
    test('setAudioParams is a function', () => {
      expect(run("typeof c.setAudioParams === 'function'")).toBe(true);
    });

    test('setVideoParams is a function', () => {
      expect(run("typeof c.setVideoParams === 'function'")).toBe(true);
    });

    test('setDeskshareParams is a function', () => {
      expect(run("typeof c.setDeskshareParams === 'function'")).toBe(true);
    });

    test('setTranscriptParams is a function', () => {
      expect(run("typeof c.setTranscriptParams === 'function'")).toBe(true);
    });

    test('setProxy is a function', () => {
      expect(run("typeof c.setProxy === 'function'")).toBe(true);
    });

    test('setProxy does not throw for http', () => {
      expect(run("(c.setProxy('http', 'http://proxy.example.com:8080'), true)")).toBe(true);
    });

    test('setProxy does not throw for https', () => {
      expect(run("(c.setProxy('https', 'https://proxy.example.com:8080'), true)")).toBe(true);
    });
  });

  // --------------------------------------------------------------------------
  describe('Client — callback registration methods', () => {
    const callbacks = [
      'onJoinConfirm', 'onSessionUpdate', 'onParticipantEvent',
      'onActiveSpeakerEvent', 'onSharingEvent', 'onEventEx',
      'onAudioData', 'onVideoData', 'onDeskshareData', 'onTranscriptData', 'onLeave',
    ];

    for (const method of callbacks) {
      test(`${method} is a function`, () => {
        expect(run(`typeof c.${method} === 'function'`)).toBe(true);
      });
    }
  });

  // --------------------------------------------------------------------------
  describe('Client — event subscription methods', () => {
    test('subscribeEvent is a function', () => {
      expect(run("typeof c.subscribeEvent === 'function'")).toBe(true);
    });

    test('unsubscribeEvent is a function', () => {
      expect(run("typeof c.unsubscribeEvent === 'function'")).toBe(true);
    });
  });

  // --------------------------------------------------------------------------
  describe('Client — individual video subscription', () => {
    test('subscribeVideo is a function', () => {
      expect(run("typeof c.subscribeVideo === 'function'")).toBe(true);
    });

    test('onParticipantVideo is a function', () => {
      expect(run("typeof c.onParticipantVideo === 'function'")).toBe(true);
    });

    test('onVideoSubscribed is a function', () => {
      expect(run("typeof c.onVideoSubscribed === 'function'")).toBe(true);
    });

    test('onParticipantVideo accepts a callback without throwing', () => {
      expect(run("(c.onParticipantVideo(() => {}), true)")).toBe(true);
    });

    test('onVideoSubscribed accepts a callback without throwing', () => {
      expect(run("(c.onVideoSubscribed(() => {}), true)")).toBe(true);
    });
  });

  // --------------------------------------------------------------------------
  describe('Module — utility functions', () => {
    test('generateSignature is a function', () => {
      expect(runModule("typeof rtms.generateSignature === 'function'")).toBe(true);
    });

    test('configureLogger is a function', () => {
      expect(runModule("typeof rtms.configureLogger === 'function'")).toBe(true);
    });

    test('onWebhookEvent is a function', () => {
      expect(runModule("typeof rtms.onWebhookEvent === 'function'")).toBe(true);
    });

    test('Client.initialize is a static function', () => {
      expect(runModule("typeof rtms.Client.initialize === 'function'")).toBe(true);
    });

    test('Client.uninitialize is a static function', () => {
      expect(runModule("typeof rtms.Client.uninitialize === 'function'")).toBe(true);
    });
  });

  // --------------------------------------------------------------------------
  describe('Module — constants', () => {
    test('MEDIA_TYPE_AUDIO === 1', () => {
      expect(runModule('rtms.MEDIA_TYPE_AUDIO === 1')).toBe(true);
    });

    test('MEDIA_TYPE_VIDEO === 2', () => {
      expect(runModule('rtms.MEDIA_TYPE_VIDEO === 2')).toBe(true);
    });

    test('MEDIA_TYPE_TRANSCRIPT === 8', () => {
      expect(runModule('rtms.MEDIA_TYPE_TRANSCRIPT === 8')).toBe(true);
    });

    test('RTMS_SDK_OK === 0', () => {
      expect(runModule('rtms.RTMS_SDK_OK === 0')).toBe(true);
    });

    test('SESSION_EVENT_ADD === 1', () => {
      expect(runModule('rtms.SESSION_EVENT_ADD === 1')).toBe(true);
    });

    test('TranscriptLanguage.ENGLISH === 9', () => {
      expect(runModule('rtms.TranscriptLanguage && rtms.TranscriptLanguage.ENGLISH === 9')).toBe(true);
    });

    test('TranscriptLanguage.NONE === -1', () => {
      expect(runModule('rtms.TranscriptLanguage && rtms.TranscriptLanguage.NONE === -1')).toBe(true);
    });

    test('AudioCodec dict is exported', () => {
      expect(runModule("typeof rtms.AudioCodec === 'object' && rtms.AudioCodec !== null")).toBe(true);
    });

    test('VideoCodec dict is exported', () => {
      expect(runModule("typeof rtms.VideoCodec === 'object' && rtms.VideoCodec !== null")).toBe(true);
    });
  });
});
