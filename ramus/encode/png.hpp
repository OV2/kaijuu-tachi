#pragma once

#include <ramus/encode/deflate.hpp>
#include <ramus/hash/adler-32.hpp>

namespace ramus {

using namespace nall;

namespace Encode {

struct PNG {
  inline PNG();
  inline ~PNG();

  inline auto load(const string& filename) -> bool;
  inline auto load(const uint8_t* sourceData, uint sourceSize) -> bool;
  inline auto readbits(const uint8_t*& data) -> uint;

  struct Info {
    uint width;
    uint height;
    uint bitDepth;
    //colorType:
    //0 = L (luma)
    //2 = R,G,B
    //3 = P (palette)
    //4 = L,A
    //6 = R,G,B,A
    uint colorType;
    uint compressionMethod;
    uint filterType;
    uint interlaceMethod;

    uint bytesPerPixel;
    uint pitch;

    uint8_t palette[256][3];
  } info;

protected:
  enum class FourCC : uint32_t {
    IHDR = 0x49484452,
    PLTE = 0x504c5445,
    IDAT = 0x49444154,
    IEND = 0x49454e44,
  };

  enum class Filter : uint8_t {
    None, Sub, Up, Average, Paeth
  };

  struct Chunk {
    FourCC name;
    vector<uint8_t> data;
    auto append(uint datum, uint size = 1) -> void {
      while(size--) data.append(datum >> (size << 3));
    }
    auto append(vector<uint8_t> _data) -> void {
      data.append(_data);
    }
    auto reset() -> void {
      data.reset();
    }
  };

public:
  static auto create(const string& filename, const uint32_t* data, unsigned width, unsigned height, bool alpha) -> bool {
    file fp{filename, file::mode::write};
    if(!fp) return false;

    Info info;
    info.width             = width;
    info.height            = height;
    info.bitDepth          = 8;
    info.colorType         = alpha ? 6 : 2;
    info.compressionMethod = 0;  //DEFLATE
    info.filterType        = 0;  //None Sub Up Average Paeth
    info.interlaceMethod   = 0;  //Don't use Adam7
    unsigned bytesPerPixel = alpha ? 4 : 3;

    fp.writem(0x89504e47, 4);  //signature
    fp.writem(0x0d0a1a0a, 4);  //signature

    Chunk chunk;

    auto write = [&](file& fp, Chunk& chunk) -> void {
      nall::Hash::CRC32 hash;
      for(uint i : rrange(4)) hash.input((uint32_t)chunk.name >> (i << 3));
      hash.input(chunk.data);
      fp.writem(chunk.data.size(), 4);
      fp.writem((uint32_t)chunk.name, 4);
      fp.write(chunk.data.data(), chunk.data.size());
      for(uint i : rrange(4)) fp.write(hash.value() >> (i << 3));
    };

    //IHDR
    chunk.reset();
    chunk.name = FourCC::IHDR;
    chunk.append(info.width, 4);
    chunk.append(info.height, 4);
    chunk.append(info.bitDepth);
    chunk.append(info.colorType);
    chunk.append(info.compressionMethod);
    chunk.append(info.filterType);
    chunk.append(info.interlaceMethod);
    write(fp, chunk);

    //IDAT
    chunk.reset();
    chunk.name = FourCC::IDAT;

    vector<uint8_t> idatData;
    idatData.reserve((1 + width * bytesPerPixel) * height);
    for(auto y : range(height)) {
      auto filterType = Filter::Paeth;
      idatData.append((uint8_t)filterType);
      const uint32_t* p = data + y * width;
      for(auto x : range(width)) {
        uint32_t ac = x == 0 ? 0x00000000 : *(p - 1);
        uint32_t bc = y == 0 ? 0x00000000 : *(p - width);
        uint32_t cc = x == 0 || y == 0 ? 0x00000000 : *(p - width - 1);

        uint32_t current = *p++;
        for(uint i : range(bytesPerPixel)) {
          uint8_t a = ac >> (i << 3);
          uint8_t b = bc >> (i << 3);
          uint8_t c = cc >> (i << 3);
          uint8_t compare;
          switch(filterType) {
          case Filter::None:    compare = 0x00; break;
          case Filter::Sub:     compare = a; break;
          case Filter::Up:      compare = b; break;
          case Filter::Average: compare = (a + b) / 2; break;
          case Filter::Paeth:
            int paeth = a + b - c;
            int pa = abs(paeth - (int)a);
            int pb = abs(paeth - (int)b);
            int pc = abs(paeth - (int)c);
                 if(pa <= pb && pa <= pc) compare = a;
            else if(pb <= pc)             compare = b;
            else                          compare = c;
            break;
          }
          uint8_t byte = current >> (i << 3);
          current &= ~0 ^ 0xff << (i << 3);
          current |= ((byte - compare) & 0xff) << (i << 3);
        }
        if((info.colorType & 3) == 2) {
          idatData.append(current >> 16);  //R
          idatData.append(current >>  8);  //G
          idatData.append(current >>  0);  //B
        }
        if(info.colorType & 4) idatData.append(current >> 24);  //A
      }
    }

    chunk.append(0x78);
    chunk.append(0x5e);
    chunk.append(ramus::Encode::deflate(idatData));
    uint32_t adler32 = ramus::Hash::Adler32(idatData).value();
    idatData.reset();
    chunk.append(adler32, 4);
    write(fp, chunk);

    //IEND
    chunk.reset();
    chunk.name = FourCC::IEND;
    write(fp, chunk);

    return true;
  }

};

}

}
