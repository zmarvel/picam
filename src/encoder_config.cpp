
#include "encoder_config.hpp"

static const int H264_DEFAULT_BITRATE = 17000000;

H264EncoderConfig H264EncoderConfig::defaultConfig() {
  H264EncoderConfig cfg = {
    .bitrate = H264_DEFAULT_BITRATE,
    .framerate = { 30, 1 },
    .profile = Profile::HIGH,
    .level = Level::H264_42,
    .immutableInputEnabled = true,
    .inlineHeaderEnabled = false,
    .SPSTimingEnabled = false,
    .inlineVectorsEnabled = false,
  };
  return cfg;
}
