#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <initializer_list>

// ── Arduino type aliases ─────────────────────────────────────────────────────
typedef uint8_t byte;

const int HIGH = 1;
const int LOW  = 0;

// ── Mock Serial ──────────────────────────────────────────────────────────────
struct MockSerial {
  std::vector<byte> readBuf;
  std::vector<byte> writeBuf;
  size_t readPos = 0;

  void reset() {
    readBuf.clear();
    writeBuf.clear();
    readPos = 0;
  }

  void feed(std::initializer_list<byte> data) {
    readBuf.insert(readBuf.end(), data);
  }

  bool available() { return readPos < readBuf.size(); }

  byte peek() { return available() ? readBuf[readPos] : 0; }

  byte read() { return available() ? readBuf[readPos++] : 0; }

  size_t readBytes(byte* buf, size_t len) {
    size_t n = 0;
    while (n < len && available()) buf[n++] = read();
    return n;
  }

  size_t write(const byte* data, size_t len) {
    writeBuf.insert(writeBuf.end(), data, data + len);
    return len;
  }

  void begin(int) {}
} Serial;

// ── Mock GPIO ────────────────────────────────────────────────────────────────
int lastDigitalWritePin   = -1;
int lastDigitalWriteValue = -1;

void digitalWrite(int pin, int value) {
  lastDigitalWritePin   = pin;
  lastDigitalWriteValue = value;
}

void pinMode(int, int) {}
