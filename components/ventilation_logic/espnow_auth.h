// ==========================================================================
// VentoSync HRV – ESPHome Custom Component
// https://github.com/thomasengeroff-dotcom/VentoSync
//
// Copyright (c) 2026 Thomas Engeroff
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
// File:        espnow_auth.h
// Description: Pure ESP-NOW message authentication (HMAC-SHA256 trailer,
//              replay guard). No ESPHome dependencies — unit-tested natively.
// Author:      Thomas Engeroff
// Created:     2026-10-18
// Modified:    2026-10-18
// ==========================================================================
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

/// @file espnow_auth.h
/// @brief Authenticated ESP-NOW frames (protocol v13).
///
/// Every ESP-NOW payload (VentilationPacket and the plaintext ROOM_DISC /
/// ROOM_CONF discovery strings, unicast and broadcast) gets a 16-byte trailer:
///
///     payload | epoch (u32 LE) | seq (u32 LE) | tag (8 bytes)
///
/// `tag` = first 8 bytes of HMAC-SHA256(key, src_mac | payload | epoch | seq).
/// The sender MAC is part of the MAC input, so a captured frame cannot be
/// replayed under another sender address. `epoch` is incremented (and
/// persisted) once per boot, `seq` per frame, so (epoch, seq) is strictly
/// increasing per sender; ReplayGuard rejects anything not newer than the
/// last accepted frame of that sender.
///
/// Scope: authenticity and replay protection only — the payload stays
/// readable (no encryption). Known gaps (documented in
/// en_esp-now-communication.md): the ReplayGuard lives in RAM, so after a
/// receiver reboots — or after a sender was silent for `idle_expiry_ms`
/// (default 15 min) — the first frame of that sender is accepted even if it is
/// an old capture; the sender's next genuine frame supersedes it. The expiry
/// lets a device whose NVS was erased (USB flash, epoch restarts at 1) rejoin
/// its room without rebooting the other devices.
namespace ventosync {
namespace espnow_auth {

constexpr size_t KEY_SIZE = 32;     ///< HMAC key (SHA-256 of the passphrase, see __init__.py).
constexpr size_t MAC_SIZE = 6;      ///< ESP-NOW sender address.
constexpr size_t TAG_SIZE = 8;      ///< Truncated HMAC-SHA256.
constexpr size_t TRAILER_SIZE = 4 + 4 + TAG_SIZE;
constexpr size_t DIGEST_SIZE = 32;

using Key = std::array<uint8_t, KEY_SIZE>;
using Mac = std::array<uint8_t, MAC_SIZE>;

// ---------------------------------------------------------------------------
// SHA-256 (FIPS 180-4) — small, dependency-free, verified by the unit tests.
// ---------------------------------------------------------------------------
class Sha256 {
 public:
  Sha256() { reset(); }

  void reset() {
    static constexpr uint32_t INIT[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                         0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    std::memcpy(h_, INIT, sizeof(h_));
    total_len_ = 0;
    buf_len_ = 0;
  }

  void update(const uint8_t *data, size_t len) {
    total_len_ += len;
    while (len > 0) {
      const size_t n = (len < 64 - buf_len_) ? len : 64 - buf_len_;
      std::memcpy(buf_ + buf_len_, data, n);
      buf_len_ += n;
      data += n;
      len -= n;
      if (buf_len_ == 64) {
        compress(buf_);
        buf_len_ = 0;
      }
    }
  }

  void finish(std::array<uint8_t, DIGEST_SIZE> &out) {
    const uint64_t bit_len = total_len_ * 8u;
    const uint8_t pad = 0x80;
    update(&pad, 1);
    const uint8_t zero = 0;
    while (buf_len_ != 56) update(&zero, 1);
    uint8_t len_be[8];
    for (int i = 0; i < 8; i++) len_be[i] = static_cast<uint8_t>(bit_len >> (56 - 8 * i));
    update(len_be, 8);
    for (int i = 0; i < 8; i++) {
      out[4 * i] = static_cast<uint8_t>(h_[i] >> 24);
      out[4 * i + 1] = static_cast<uint8_t>(h_[i] >> 16);
      out[4 * i + 2] = static_cast<uint8_t>(h_[i] >> 8);
      out[4 * i + 3] = static_cast<uint8_t>(h_[i]);
    }
  }

 private:
  static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

  void compress(const uint8_t block[64]) {
    static constexpr uint32_t K[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
      w[i] = (static_cast<uint32_t>(block[4 * i]) << 24) | (static_cast<uint32_t>(block[4 * i + 1]) << 16) |
             (static_cast<uint32_t>(block[4 * i + 2]) << 8) | static_cast<uint32_t>(block[4 * i + 3]);
    }
    for (int i = 16; i < 64; i++) {
      const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3], e = h_[4], f = h_[5], g = h_[6], h = h_[7];
    for (int i = 0; i < 64; i++) {
      const uint32_t t1 = h + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
      const uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    h_[0] += a;
    h_[1] += b;
    h_[2] += c;
    h_[3] += d;
    h_[4] += e;
    h_[5] += f;
    h_[6] += g;
    h_[7] += h;
  }

  uint32_t h_[8];
  uint64_t total_len_;
  uint8_t buf_[64];
  size_t buf_len_;
};

// ---------------------------------------------------------------------------
// HMAC-SHA256 (RFC 2104) for keys up to 64 bytes.
// ---------------------------------------------------------------------------
class HmacSha256 {
 public:
  HmacSha256(const uint8_t *key, size_t key_len) {
    uint8_t k[64] = {0};
    if (key_len > 64) {
      Sha256 kh;
      kh.update(key, key_len);
      kh.finish(k);
    } else {
      std::memcpy(k, key, key_len);
    }
    for (size_t i = 0; i < 64; i++) {
      ipad_[i] = static_cast<uint8_t>(k[i] ^ 0x36u);
      opad_[i] = static_cast<uint8_t>(k[i] ^ 0x5cu);
    }
    inner_.update(ipad_, 64);
  }

  void update(const uint8_t *data, size_t len) { inner_.update(data, len); }

  void finish(uint8_t out[DIGEST_SIZE]) {
    uint8_t inner_digest[DIGEST_SIZE];
    inner_.finish(inner_digest);
    Sha256 outer;
    outer.update(opad_, 64);
    outer.update(inner_digest, DIGEST_SIZE);
    outer.finish(out);
  }

 private:
  Sha256 inner_;
  uint8_t ipad_[64];
  uint8_t opad_[64];
};

// ---------------------------------------------------------------------------
// Frame counter
// ---------------------------------------------------------------------------

/// Per-sender frame counter: epoch (per boot, persisted) + seq (per frame).
struct Counter {
  uint32_t epoch = 0;
  uint32_t seq = 0;

  /// Strict ordering used by ReplayGuard.
  bool newer_than(const Counter &o) const {
    return epoch > o.epoch || (epoch == o.epoch && seq > o.seq);
  }
};

inline void put_u32_le(uint8_t *p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v);
  p[1] = static_cast<uint8_t>(v >> 8);
  p[2] = static_cast<uint8_t>(v >> 16);
  p[3] = static_cast<uint8_t>(v >> 24);
}

inline uint32_t get_u32_le(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

/**
 * @brief   Computes the truncated tag over src_mac | payload | epoch | seq.
 */
inline void compute_tag(const Key &key, const uint8_t *src_mac, const uint8_t *payload, size_t len,
                        const Counter &ctr, uint8_t tag[TAG_SIZE]) {
  HmacSha256 hmac(key.data(), key.size());
  hmac.update(src_mac, MAC_SIZE);
  hmac.update(payload, len);
  uint8_t ctr_le[8];
  put_u32_le(ctr_le, ctr.epoch);
  put_u32_le(ctr_le + 4, ctr.seq);
  hmac.update(ctr_le, sizeof(ctr_le));
  uint8_t digest[DIGEST_SIZE];
  hmac.finish(digest);
  std::memcpy(tag, digest, TAG_SIZE);
}

/**
 * @brief   Appends the authentication trailer to an outgoing payload.
 *
 * @param[in]     key      Room key.
 * @param[in]     src_mac  This device's ESP-NOW (STA) MAC — the address the
 *                         receivers see as the sender.
 * @param[in,out] frame    Payload; the trailer is appended in place.
 * @param[in]     ctr      Frame counter (caller increments seq per frame).
 */
inline void seal(const Key &key, const uint8_t *src_mac, std::vector<uint8_t> &frame, const Counter &ctr) {
  uint8_t tag[TAG_SIZE];
  compute_tag(key, src_mac, frame.data(), frame.size(), ctr, tag);
  const size_t off = frame.size();
  frame.resize(off + TRAILER_SIZE);
  put_u32_le(frame.data() + off, ctr.epoch);
  put_u32_le(frame.data() + off + 4, ctr.seq);
  std::memcpy(frame.data() + off + 8, tag, TAG_SIZE);
}

/// Result of verify().
enum class Verdict : uint8_t {
  OK,         ///< Tag valid (replay check is separate, see ReplayGuard).
  TOO_SHORT,  ///< Not even a trailer (e.g. firmware without authentication).
  BAD_TAG,    ///< Forged, corrupted or signed with another key.
};

/**
 * @brief   Checks the trailer of a received frame (constant-time tag compare).
 *
 * @param[out] payload_len  Length of the payload without trailer (on OK).
 * @param[out] ctr          The frame's counter (on OK) — pass to ReplayGuard.
 */
inline Verdict verify(const Key &key, const uint8_t *src_mac, const uint8_t *data, size_t len,
                      size_t &payload_len, Counter &ctr) {
  if (len <= TRAILER_SIZE) return Verdict::TOO_SHORT;
  const size_t plen = len - TRAILER_SIZE;
  Counter c;
  c.epoch = get_u32_le(data + plen);
  c.seq = get_u32_le(data + plen + 4);
  uint8_t expected[TAG_SIZE];
  compute_tag(key, src_mac, data, plen, c, expected);
  uint8_t diff = 0;
  for (size_t i = 0; i < TAG_SIZE; i++) diff |= static_cast<uint8_t>(expected[i] ^ data[plen + 8 + i]);
  if (diff != 0) return Verdict::BAD_TAG;
  payload_len = plen;
  ctr = c;
  return Verdict::OK;
}

/**
 * @brief   Remembers the newest accepted counter per sender MAC.
 *
 * @details Fixed capacity (no heap growth from spoofed senders — a sender
 *          only gets an entry after its tag was verified). When full, the
 *          least recently accepted sender is evicted. An entry without an
 *          accepted frame for `idle_expiry_ms` is forgotten (see file header).
 */
class ReplayGuard {
 public:
  static constexpr size_t CAPACITY = 16;
  static constexpr uint32_t DEFAULT_IDLE_EXPIRY_MS = 15u * 60u * 1000u;

  explicit ReplayGuard(uint32_t idle_expiry_ms = DEFAULT_IDLE_EXPIRY_MS) : idle_expiry_ms_(idle_expiry_ms) {}

  /// @return true if `ctr` is newer than the last accepted frame of `mac`
  ///         (or `mac` is unknown / expired); the counter is then remembered.
  /// @param now_ms  millis(); all comparisons are wrap-safe.
  bool accept(const uint8_t *mac, const Counter &ctr, uint32_t now_ms) {
    Entry *slot = nullptr;
    for (auto &e : entries_) {
      if (e.used && std::memcmp(e.mac.data(), mac, MAC_SIZE) == 0) {
        const bool expired = now_ms - e.touched_ms >= idle_expiry_ms_;
        if (!expired && !ctr.newer_than(e.last)) return false;
        slot = &e;
        break;
      }
    }
    if (slot == nullptr) {
      slot = &entries_[0];
      for (auto &e : entries_) {
        if (!e.used) {
          slot = &e;
          break;
        }
        if (now_ms - e.touched_ms > now_ms - slot->touched_ms) slot = &e; // least recently accepted
      }
      std::memcpy(slot->mac.data(), mac, MAC_SIZE);
      slot->used = true;
    }
    slot->last = ctr;
    slot->touched_ms = now_ms;
    return true;
  }

  void clear() { entries_ = {}; }

 private:
  struct Entry {
    Mac mac{};
    Counter last{};
    uint32_t touched_ms = 0;
    bool used = false;
  };
  std::array<Entry, CAPACITY> entries_{};
  uint32_t idle_expiry_ms_;
};

}  // namespace espnow_auth
}  // namespace ventosync
