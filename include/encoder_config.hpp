#ifndef ENCODER_CONFIG_HPP
#define ENCODER_CONFIG_HPP

#include <utility>
#include <cstdint>

struct H264EncoderConfig {
  enum Profile {
    BASELINE,
    MAIN,
    HIGH,
  };

  /**
   * A Framerate is a (numerator, denominator) pair.
   */
  using Framerate = std::pair<uint32_t, uint32_t>;

  enum Level {
    H264_2,
    H264_21,
    H264_22,
    H264_3,
    H264_31,
    H264_32,
    H264_4,
    H264_41,
    H264_42,
    H264_5,
    H264_51,
  };

  /**
   * Construct a config with some default values. (See the implementation for
   * the actual values that will be used.)
   */
  static H264EncoderConfig defaultConfig();

  uint32_t bitrate;
  Framerate framerate;
  Profile profile;
  Level level;
  bool immutableInputEnabled;
  bool inlineHeaderEnabled;
  bool SPSTimingEnabled;
  bool inlineVectorsEnabled;
};

#endif // ENCODER_CONFIG_HPP
