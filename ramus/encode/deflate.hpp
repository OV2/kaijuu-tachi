#pragma once

namespace ramus {

using namespace nall;

namespace Encode {

namespace Deflate {
  struct BitBuffer : vector<uint8_t> {
    uint bitQueue = 0;
    int bitCount = 0;
    inline auto pushBits(uint bits, int _bitCount) -> void;
  };
  inline auto compress(const uint8_t* source, uint length) -> BitBuffer;
  inline auto compress(vector<uint8_t> source) -> BitBuffer;
}

inline auto deflate(const uint8_t* data, const uint length) -> Deflate::BitBuffer {
  return Deflate::compress(data, length);
}

inline auto deflate(vector<uint8_t> input) -> Deflate::BitBuffer {
  return Deflate::compress(input);
}

namespace Deflate {

enum : uint {
  HASH_BITS = 12,
  HASH_SIZE = (1 << HASH_BITS),

  MIN_MATCH = 3,
  MAX_MATCH = 255 + MIN_MATCH,

  WINDOW_SIZE = 32768,
};

auto BitBuffer::pushBits(uint bits, int _bitCount) -> void {
  assert(bitCount + _bitCount <= 32);
  bitQueue |= bits << bitCount;
  bitCount += _bitCount;
  while(bitCount >= 8) {
    append(bitQueue & 0xff);
    bitQueue >>= 8;
    bitCount -= 8;
  }
}

inline auto bitmirror(uint8_t input) {
  uint8_t output = 0;
  output |= (input & 0x01) << 7;
  output |= (input & 0x02) << 5;
  output |= (input & 0x04) << 3;
  output |= (input & 0x08) << 1;
  output |= (input & 0x10) >> 1;
  output |= (input & 0x20) >> 3;
  output |= (input & 0x40) >> 5;
  output |= (input & 0x80) >> 7;
  return output;
}

inline auto literal(BitBuffer& buffer, uint8_t c) -> void {
  if(c <= 143) buffer.pushBits(bitmirror(0x30 + c), 8);
  else         buffer.pushBits(bitmirror(0x90 - 144 + c) << 1 | 1, 9);
}

inline auto match(BitBuffer& buffer, int distance, int length) -> void {
  int symbol;
  int left, right;
  static const short lens[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
  };
  static const short lext[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
  };
  static const short dists[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577
  };
  static const short dext[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13
  };

  while(length > 0) {
    int currentLength = length > 260 ? 258 : (length <= 258 ? length : length - 3);
    length -= currentLength;

    left = -1;
    right = sizeof(lens) / sizeof(*lens);
    while(true) {
      assert(right - left >= 2);
      int middle = (right + left) / 2;
      uint16_t min = lens[middle];
      uint16_t max = middle == 27 ? lens[28] - 1 : min + (1 << lext[middle]) - 1;
           if(currentLength < min) right = middle;
      else if(currentLength > max) left  = middle;
      else { symbol = middle + 257; break; }
    }

    if(symbol <= 279) buffer.pushBits(bitmirror((symbol - 256) << 1), 7);
    else              buffer.pushBits(bitmirror(0xc0 - 280 + symbol), 8);
    buffer.pushBits(currentLength - lens[symbol - 257], lext[symbol - 257]);

    left = -1;
    right = sizeof(dists) / sizeof(*dists);
    while(true) {
      assert(right - left >= 2);
      int middle = (right + left) / 2;
      uint16_t min = dists[middle];
      uint16_t max = min + (1 << dext[middle]) - 1;
           if(distance < min) right = middle;
      else if(distance > max) left  = middle;
      else { symbol = middle; break; }
    }

    buffer.pushBits(bitmirror(symbol << 3), 5);
    buffer.pushBits(distance - dists[symbol], dext[symbol]);
  }
}

inline auto deflateStartBlock(BitBuffer& buffer) -> void {
  bool final = true;
  uint type = 1;  //static Huffman block

  buffer.pushBits(final, 1);
  buffer.pushBits(type, 2);
}

inline auto deflateFinishBlock(BitBuffer& buffer) -> void {
  buffer.pushBits(0b0000000, 7);  //close block
  buffer.pushBits(0b0000000, 7);  //flush remaining bits
}

inline auto compress(const uint8_t* source, uint length) -> BitBuffer {
  BitBuffer buffer;

  deflateStartBlock(buffer);

  const uint8_t* hashtable[HASH_SIZE] = {0};

  static auto hash = [](const uint8_t* p) -> int {
    int v = (p[0] << 16) | (p[1] << 8) | p[2];
    int hash = ((v >> (3 * 8 - HASH_BITS)) - v) & (HASH_SIZE - 1);
    return hash;
  };

  const uint8_t* end = source + length;
  while(source < end - MIN_MATCH) {
    const uint8_t** bucket = &hashtable[hash(source) & (HASH_SIZE - 1)];
    const uint8_t* subs = *bucket;
    *bucket = source;
    if(subs && source > subs && (source - subs) <= WINDOW_SIZE && !memory::compare(source, subs, MIN_MATCH)) {
      source += MIN_MATCH;
      const uint8_t* m = subs + MIN_MATCH;
      int matchLength = MIN_MATCH;
      while(*source == *m && matchLength < MAX_MATCH && source < end) source++, m++, matchLength++;
      match(buffer, source - matchLength - subs, matchLength);
    } else {
      literal(buffer, *source++);
    }
  }

  while(source < end) literal(buffer, *source++);

  deflateFinishBlock(buffer);

  return buffer;
}

inline auto compress(vector<uint8_t> source) -> BitBuffer {
  return compress(source.data(), source.size());
}

}

}

}
