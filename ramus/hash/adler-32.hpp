#pragma once

#include <nall/hash/hash.hpp>

namespace ramus {

using namespace nall;

namespace Hash {

struct Adler32 : nall::Hash::Hash {
  nallHash(Adler32)

  auto reset() -> void override {
    a = 1;
    b = 0;
  }

  auto input(uint8_t value) -> void override {
    a = (a + value) % 65521;
    b = (b + a) % 65521;
  }

  auto output() const -> vector<uint8_t> {
    vector<uint8_t> result;
    result.append(b >> 8);
    result.append(b & 0xff);
    result.append(a >> 8);
    result.append(a & 0xff);
    return result;
  }

  auto value() const -> uint32_t {
    return b << 16 | a;
  }

private:
  uint16_t a = 0;
  uint16_t b = 0;
};

}

}
