#pragma once

namespace ramus {

namespace Decode {

struct Wave {
  Wave(const vector<uint8_t>& data_);

  auto sample() -> array<double, 2>;

  explicit operator bool() const;

  struct Format { enum : uint {
    PCM        = 0x0001,
    IEEE_FLOAT = 0x0003,
    ALAW       = 0x0006,
    MULAW      = 0x0007,
    EXTENSIBLE = 0xfffe
  };};

  uint bitDepth;
  uint channels;
  uint frequency;

private:
  vector<uint8_t> data;
  const uint8_t* pos;

  uint format;

  uint remainingSamples;
};

Wave::Wave(const vector<uint8_t>& data_) {
  data = data_;

  const uint8_t* pos_ = data.data();
  remainingSamples = 0;

  string header = "";
  header.reserve(4);

  uint32_t byteRate;
  uint16_t sampleSize;

  auto readHeader = [&]() -> void {
    memory::copy((uint8_t*)header.data(), pos_, 4);
    pos_ += 4;
  };

  auto read8 = [&]() -> uint8_t {
    return *pos_++;
  };

  auto read16 = [&]() -> uint16_t {
    return read8() | read8() << 8;
  };

  auto read32 = [&]() -> uint32_t {
    return read16() | read16() << 16;
  };

  readHeader();
  if(header != "RIFF") return;
  uint waveSize = read32();

  readHeader();
  if(header != "WAVE") return;

  uint chunkSize;

  while(pos_ - data.data() < data.size()) {
    readHeader();
    chunkSize = read32();

    if(header == "fmt ") {
      format     = read16();
      channels   = read16();
      frequency  = read32();
      byteRate   = read32();
      sampleSize = read16();
      bitDepth   = read16();

      chunkSize -= 16;
      if(chunkSize) return;

      if(sampleSize != channels * (bitDepth >> 3)) return;
      if(byteRate   != frequency * sampleSize) return;
    } else if(header == "data") {
      pos = pos_;
      if(format == Format::PCM) remainingSamples = chunkSize / sampleSize;
      pos_ += chunkSize;
    } else if(header == "fact") {
      if(format != Format::PCM) remainingSamples = read32();
    } else {  //Ignore other chunks
      while(chunkSize--) read8();
    }
  }
}

auto Wave::sample() -> array<double, 2> {
  if(remainingSamples == 0) return {};

  auto read8 = [&]() -> uint8_t {
    return *pos++;
  };

  auto read16 = [&]() -> uint16_t {
    return read8() | read8() << 8;
  };

  auto read32 = [&]() -> uint32_t {
    return read16() | read16() << 16;
  };

  auto readFloat32 = [&]() -> float32_t {
    //TODO: below code works only on x86
    float32_t n = *((float32_t*)pos);
    pos += 4;
    return n;
  };

  auto readSample = [&]() -> double {
    if(format == Format::PCM) {
      return read16() / 32768.0;
    } else if(format == Format::IEEE_FLOAT) {
      return readFloat32();
    } else {
      return 0.0;
    }
  };

  remainingSamples--;

  array<double, 2> sample;
  sample[0] = readSample();
  sample[1] = channels > 1 ? readSample() : sample[0];
  return sample;
}

Wave::operator bool() const {
  return remainingSamples > 0;
}

}

}
