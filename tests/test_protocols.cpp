#include "mock_arduino.h"
#include "../Arduino_src/OvM Recording Light/protocols.h"
#include <cstdio>

// ── Test harness ─────────────────────────────────────────────────────────────
int passed = 0, failed = 0;

void check(const char* name, bool cond) {
  if (cond) { printf("  PASS  %s\n", name); passed++; }
  else      { printf("  FAIL  %s\n", name); failed++; }
}

void reset() { Serial.reset(); resetProtocolState(); }

// ── HUI tests ────────────────────────────────────────────────────────────────

void test_hui_ping_sends_pong() {
  reset();
  byte msg[] = {0x90, 0x00, 0x00};
  checkHUI(msg);
  check("HUI ping: pong (90 00 7F) written to serial",
        Serial.writeBuf == std::vector<byte>{0x90, 0x00, 0x7F});
  check("HUI ping: recording state unchanged",
        recordingFlagHUI == false);
}

void test_hui_record_only_not_recording() {
  reset();
  byte zone[] = {0xB0, 0x0C, 0x0E};
  byte port[] = {0xB0, 0x2C, 0x45};  // RECORD on
  checkHUI(zone);
  bool result = checkHUI(port);
  check("HUI record-only (armed, not playing) → not recording", result == false);
}

void test_hui_play_only_not_recording() {
  reset();
  byte zone[] = {0xB0, 0x0C, 0x0E};
  byte port[] = {0xB0, 0x2C, 0x44};  // PLAY on
  checkHUI(zone);
  bool result = checkHUI(port);
  check("HUI play-only (not in record mode) → not recording", result == false);
}

void test_hui_play_and_record_recording() {
  reset();
  byte zoneA[] = {0xB0, 0x0C, 0x0E};
  byte play[]  = {0xB0, 0x2C, 0x44};  // PLAY on
  byte zoneB[] = {0xB0, 0x0C, 0x0E};
  byte rec[]   = {0xB0, 0x2C, 0x45};  // RECORD on
  checkHUI(zoneA); checkHUI(play);
  checkHUI(zoneB); bool result = checkHUI(rec);
  check("HUI play + record → recording", result == true);
}

void test_hui_record_off() {
  reset();
  playingFlagHUI   = true;  // simulate were-recording state
  recordingFlagHUI = true;
  byte zone[] = {0xB0, 0x0C, 0x0E};
  byte port[] = {0xB0, 0x2C, 0x05};  // RECORD off
  checkHUI(zone);
  bool result = checkHUI(port);
  check("HUI record OFF (zone 0E, port 05)", result == false);
}

void test_hui_play_port_updates_play_flag() {
  reset();
  byte zone[] = {0xB0, 0x0C, 0x0E};
  byte port[] = {0xB0, 0x2C, 0x44};  // PLAY on
  checkHUI(zone);
  checkHUI(port);
  check("HUI PLAY port: playingFlagHUI set",         playingFlagHUI   == true);
  check("HUI PLAY port: recordingFlagHUI unchanged",  recordingFlagHUI == false);
}

void test_hui_stop_clears_recording() {
  reset();
  playingFlagHUI   = true;  // were recording
  recordingFlagHUI = true;
  byte zone[] = {0xB0, 0x0C, 0x0E};
  byte port[] = {0xB0, 0x2C, 0x04};  // PLAY off
  checkHUI(zone);
  bool result = checkHUI(port);
  check("HUI PLAY off while recording → not recording", result == false);
}

void test_hui_wrong_zone_ignored() {
  reset();
  byte zone[] = {0xB0, 0x0C, 0x0A};  // not transport zone
  byte port[] = {0xB0, 0x2C, 0x45};  // record port value
  checkHUI(zone);
  checkHUI(port);
  check("HUI wrong zone: recording state unchanged", recordingFlagHUI == false);
}

void test_hui_zone_resets_after_port() {
  reset();
  byte zone[] = {0xB0, 0x0C, 0x0E};
  byte port[] = {0xB0, 0x2C, 0x45};
  checkHUI(zone);
  check("HUI zone stored after zone select",  huiZone == 0x0E);
  checkHUI(port);
  check("HUI zone reset after port consumed", huiZone == 0xFF);
}

// ── Mackie Control tests ──────────────────────────────────────────────────────

void test_mackie_on() {
  reset();
  byte msg[] = {0x90, 0x5F, 0x7F};
  checkMackieControl(msg);
  check("Mackie: note 5F vel 7F → recording ON", recordingFlagMackie == true);
}

void test_mackie_off() {
  reset();
  recordingFlagMackie = true;
  byte msg[] = {0x90, 0x5F, 0x00};
  checkMackieControl(msg);
  check("Mackie: note 5F vel 00 → recording OFF", recordingFlagMackie == false);
}

void test_mackie_unrelated_message_unchanged() {
  reset();
  recordingFlagMackie = true;
  byte msg[] = {0x90, 0x3C, 0x7F};  // different note
  checkMackieControl(msg);
  check("Mackie: unrelated note leaves state unchanged", recordingFlagMackie == true);
}

// ── M-Audio Keyboard tests ────────────────────────────────────────────────────

void test_maudio_play_only_not_recording() {
  reset();
  byte msg[] = {0xB0, 0x75, 0x7F};
  bool result = checkMAudioKeyboard(msg);
  check("M-Audio: play-only → not recording", result == false);
}

void test_maudio_rec_only_not_recording() {
  reset();
  byte msg[] = {0xB0, 0x76, 0x7F};
  bool result = checkMAudioKeyboard(msg);
  check("M-Audio: rec-enable-only → not recording", result == false);
}

void test_maudio_play_and_rec_recording() {
  reset();
  byte play[] = {0xB0, 0x75, 0x7F};
  byte rec[]  = {0xB0, 0x76, 0x7F};
  checkMAudioKeyboard(play);
  bool result = checkMAudioKeyboard(rec);
  check("M-Audio: play + rec-enable → recording", result == true);
}

void test_maudio_stop_clears_recording() {
  reset();
  playingFlagMAudio = true;
  recordEnableFlagMAudio = true;
  byte stop[] = {0xB0, 0x75, 0x00};
  bool result = checkMAudioKeyboard(stop);
  check("M-Audio: play stop → not recording", result == false);
}

// ── Protocol isolation tests ──────────────────────────────────────────────────

void test_hui_zone_cc_does_not_trigger_maudio() {
  reset();
  byte msg[] = {0xB0, 0x0C, 0x0E};  // HUI zone select CC
  checkMAudioKeyboard(msg);
  check("HUI zone CC doesn't affect M-Audio flags",
        !playingFlagMAudio && !recordEnableFlagMAudio);
}

void test_mackie_note_does_not_trigger_hui_pong() {
  reset();
  byte msg[] = {0x90, 0x5F, 0x7F};  // Mackie record ON
  checkHUI(msg);
  check("Mackie note 5F doesn't trigger HUI pong", Serial.writeBuf.empty());
}

void test_hui_ping_does_not_trigger_mackie() {
  reset();
  byte msg[] = {0x90, 0x00, 0x00};  // HUI ping
  checkMackieControl(msg);
  check("HUI ping (note 00) doesn't affect Mackie flag", recordingFlagMackie == false);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
  printf("── HUI ──────────────────────────────────────────\n");
  test_hui_ping_sends_pong();
  test_hui_record_only_not_recording();
  test_hui_play_only_not_recording();
  test_hui_play_and_record_recording();
  test_hui_record_off();
  test_hui_stop_clears_recording();
  test_hui_play_port_updates_play_flag();
  test_hui_wrong_zone_ignored();
  test_hui_zone_resets_after_port();

  printf("── Mackie Control ───────────────────────────────\n");
  test_mackie_on();
  test_mackie_off();
  test_mackie_unrelated_message_unchanged();

  printf("── M-Audio Keyboard ─────────────────────────────\n");
  test_maudio_play_only_not_recording();
  test_maudio_rec_only_not_recording();
  test_maudio_play_and_rec_recording();
  test_maudio_stop_clears_recording();

  printf("── Protocol isolation ───────────────────────────\n");
  test_hui_zone_cc_does_not_trigger_maudio();
  test_mackie_note_does_not_trigger_hui_pong();
  test_hui_ping_does_not_trigger_mackie();

  printf("\n%d passed, %d failed\n", passed, failed);
  return failed > 0 ? 1 : 0;
}
