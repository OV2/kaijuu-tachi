#pragma once

namespace ramus {
  
using namespace nall;

#include <ramus/shift-jis/jis0208.hpp>

auto convertShiftJIStoUTF8(string shiftjis) -> string {
  string utf8 = "";
  uint8_t s1 = 0;
  char utf8buffer[4];
  utf8buffer[3] = 0x00;
  for(uint index : range(shiftjis.size())) {
    uint8_t byte = shiftjis[index];
    if(s1) {
      uint8_t s2 = byte;
      uint8_t j1 = ((s1 & 0x3f) << 1) + 0x1f + (s2 >= 0x9f);
      uint8_t j2 = s2 < 0x9f ? s2 - 0x1f - (s2 >> 7) : s2 - 0x7e;
      uint32_t unicode = jis0208[(j1 - 0x21) * 94 + j2 - 0x21];
      utf8buffer[0] = ((unicode >> 12) & 0x0f) | 0xe0;
      utf8buffer[1] = ((unicode >>  6) & 0x3f) | 0x80;
      utf8buffer[2] = ((unicode >>  0) & 0x3f) | 0x80;
      utf8.append(string{utf8buffer});
      s1 = 0;
    } else if(byte & 0x80) {
      if((byte >= 0xa1 && byte <= 0xdf)) {
        uint32_t unicode = byte + 0xfec0;
        utf8buffer[0] = ((unicode >> 12) & 0x0f) | 0xe0;
        utf8buffer[1] = ((unicode >>  6) & 0x3f) | 0x80;
        utf8buffer[2] = ((unicode >>  0) & 0x3f) | 0x80;
        utf8.append(string{utf8buffer});
      } else {  //first byte of multi-byte character
        s1 = byte;
      }
    } else {
      if(byte == 0x5c) {
        utf8.append("¥");
      } else if(byte == 0x7e) {
        utf8.append("¯");
      } else if(byte) {
        utf8.append((char)byte);
      }
    }
  }
  return utf8;
}

}
