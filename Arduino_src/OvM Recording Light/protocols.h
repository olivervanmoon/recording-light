// protocols.h — MIDI control surface protocol handlers
// Included by both OvM Recording Light.ino and tests/test_protocols.cpp.
// Contains no Arduino-specific types — Serial is resolved by the including file.

// ── State variables ──────────────────────────────────────────────────────────

// HUI (Pro Tools, modern versions)
static byte huiZone          = 0xFF;
static bool playingFlagHUI   = false;
static bool recordingFlagHUI = false;

// Mackie Control (Logic Pro X, Ableton Live)
static bool recordingFlagMackie = false;

// M-Audio Keyboard (legacy Pro Tools workaround, removed in ProTools 2023.x+)
static bool playingFlagMAudio      = false;
static bool recordEnableFlagMAudio = false;

// Reset all state — called between unit tests
inline void resetProtocolState() {
  huiZone               = 0xFF;
  playingFlagHUI        = false;
  recordingFlagHUI      = false;
  recordingFlagMackie   = false;
  playingFlagMAudio     = false;
  recordEnableFlagMAudio = false;
}

// ── Protocol: HUI (Pro Tools, modern) ───────────────────────────────────────

inline void handleSysEx() {
  // Discard all SysEx bytes up to and including the end marker (0xF7).
  // Pro Tools sends SysEx for display/LED bulk updates. Without this flush,
  // those bytes corrupt the 3-byte parser.
  while (Serial.available()) {
    if (Serial.read() == 0xF7) break;
  }
}

inline bool checkHUI(byte message[]) {
  // Keepalive: Pro Tools sends a ping (90 00 00) and expects a pong (90 00 7F)
  // within ~1 second or it reports the device as disconnected.
  if (message[0] == 0x90 && message[1] == 0x00 && message[2] == 0x00) {
    byte pong[] = {0x90, 0x00, 0x7F};
    Serial.write(pong, 3);
    return (playingFlagHUI && recordingFlagHUI);
  }

  // HUI encodes transport state as paired CC messages:
  //   B0 0C [zone]      — zone select
  //   B0 2C [port_val]  — port (bits 3:0) + state (bit 6)
  // Transport zone = 0x0E: REW=1, FF=2, STOP=3, PLAY=4, RECORD=5
  // Both PLAY and RECORD must be active to indicate actually recording
  // (RECORD alone means armed but not yet rolling).
  if (message[0] == 0xB0) {
    if (message[1] == 0x0C) {
      huiZone = message[2];
    } else if (message[1] == 0x2C) {
      if (huiZone == 0x0E) {                    // transport zone
        byte port  =  message[2] & 0x0F;        // bits 3:0
        bool state = (message[2] >> 6) & 0x01;  // bit 6
        if      (port == 0x04) { playingFlagHUI   = state; }  // PLAY LED
        else if (port == 0x05) { recordingFlagHUI = state; }  // RECORD LED
      }
      huiZone = 0xFF;  // always reset after consuming port byte
    }
  }
  return (playingFlagHUI && recordingFlagHUI);
}

// ── Protocol: Mackie Control (Logic Pro X, Ableton Live) ────────────────────

inline bool checkMackieControl(byte message[]) {
  // Single note (0x5F) carries both on (vel 0x7F) and off (vel 0x00).
  if (message[0] == 0x90 and message[1] == 0x5F) {
    if      (message[2] == 0x7F) { recordingFlagMackie = true;  }
    else if (message[2] == 0x00) { recordingFlagMackie = false; }
  }
  return recordingFlagMackie;
}

// ── Protocol: M-Audio Keyboard (legacy Pro Tools, pre-2023.x) ───────────────
// Original workaround for Pro Tools' limited control surface options.
// Kept for backwards compatibility with older Pro Tools installations.

inline bool checkMAudioKeyboard(byte message[]) {
  if (message[0] == 0xB0) {
    if      (message[1] == 0x75) { playingFlagMAudio      = (message[2] == 0x7F); }
    else if (message[1] == 0x76) { recordEnableFlagMAudio = (message[2] == 0x7F); }
  }
  return (recordEnableFlagMAudio and playingFlagMAudio);
}
