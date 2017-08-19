#pragma once

namespace ramus {

using namespace nall;

namespace Hash {

struct MD5 : nall::Hash::Hash {
  nallHash(MD5)

  auto reset() -> void override {
    for(auto& n : queue) n = 0;
    for(auto  n : range(4)) h[n] = square(n);
    queued = length = 0;
  }

  auto input(uint8_t value) -> void override {
    byte(value);
    length++;
  }

  auto output() const -> vector<uint8_t> override {
    MD5 self(*this);
    self.finish();
    vector<uint8_t> result;
    for(auto h : self.h) {
      for(auto n : range(4)) result.append(h >> n * 8);
    }
    return result;
  }

  auto value() const -> uint128_t {
    uint128_t value = 0;
    for(auto byte : output()) value = value << 8 | byte;
    return value;
  }

private:
  auto byte(uint8_t value) -> void {
    uint32_t shift = (queued & 3) * 8;
    queue[queued >> 2] &= ~(0xff << shift);
    queue[queued >> 2] |= (value << shift);
    if(++queued == 64) block(), queued = 0;
  }

  auto block() -> void {
    uint32_t t[4];
    for(auto n : range(4)) t[n] = h[n];
    uint32_t f;
    uint32_t g;
    for(auto i : range(64)) {
      switch(i & 48) {
      case  0:
        f = (t[1] & t[2]) | (~t[1] & t[3]);
        g = i;
        break;
      case 16:
        f = (t[3] & t[1]) | (~t[3] & t[2]);
        g = (5 * i + 1) % 16;
        break;
      case 32:
        f = t[1] ^ t[2] ^ t[3];
        g = (3 * i + 5) % 16;
        break;
      case 48:
        f = t[2] ^ (t[1] | ~t[3]);
        g = (7 * i) % 16;
        break;
      }
      uint32_t temp = t[3];
      t[3] = t[2];
      t[2] = t[1];
      t[1] = t[1] + rol((t[0] + f + cube(i) + queue[g]), shift(i));
      t[0] = temp;
    }
    for(auto n : range(4)) h[n] += t[n];
  }

  auto finish() -> void {
    uint64_t oldLength = length;
    byte(0x80);
    while(queued != 56) byte(0x00);
    for(auto n : range(8)) byte(oldLength * 8 >> n * 8);
  }

  auto rol(uint32_t x, uint32_t n) -> uint32_t {
    return (x << n) | (x >> 32 - n);
  }

  auto shift(uint n) -> uint {
    static const uint value[64] = {
       7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
       5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
       4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
       6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,
    };
    return value[n];
  }

  auto square(uint n) -> uint32_t {
    static const uint32_t value[8] = {
      0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476,
    };
    return value[n];
  }

  auto cube(uint n) -> uint32_t {
    static const uint32_t value[64] = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
      0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
      0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
      0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
      0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
      0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
    };
    return value[n];
  }

  uint32_t queue[16];
  uint32_t h[4] = {0};
  uint32_t queued = 0;
  uint64_t length = 0;
};

}

}
