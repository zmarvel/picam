/*
 * Copyright (C) 2019 Zack Marvel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ENCODER_CONFIG_HPP
#define ENCODER_CONFIG_HPP

#include <utility>
#include <cstdint>
#include <interface/mmal/mmal.h>

struct BaseEncoderConfig {
  BaseEncoderConfig()
    : immutableInputEnabled{true},
    inlineHeaderEnabled{false},
    SPSTimingEnabled{false},
    inlineVectorsEnabled{false}
  { }

  /**
   * Configure the encoder input and output ports as necessary. The input port
   * will probably be automatically configured when its source is connected.
   * Base classes should override this method, but still call the base class'
   * implementation.
   */
  virtual MMAL_STATUS_T configure(MMAL_PORT_T* input, MMAL_PORT_T* output);

  bool immutableInputEnabled;
  bool inlineHeaderEnabled;
  bool SPSTimingEnabled;
  bool inlineVectorsEnabled;
};

struct H264EncoderConfig : public BaseEncoderConfig {
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
  H264EncoderConfig()
  : H264EncoderConfig{
    DEFAULT_BITRATE,
    { 30, 1 },
    Profile::HIGH,
    Level::H264_42}
  { }

  H264EncoderConfig(uint32_t bitrate, Framerate framerate, Profile profile,
      Level level)
    : BaseEncoderConfig{},
    bitrate{bitrate},
    framerate{framerate},
    profile{profile},
    level{level}
  { }

  virtual MMAL_STATUS_T configure(MMAL_PORT_T* input, MMAL_PORT_T* output)
    override;

  uint32_t bitrate;
  Framerate framerate;
  Profile profile;
  Level level;

  const uint32_t DEFAULT_BITRATE = 17000000;
};

struct PNGEncoderConfig : public BaseEncoderConfig {
  PNGEncoderConfig() : BaseEncoderConfig{} { }

  virtual MMAL_STATUS_T configure(MMAL_PORT_T* input, MMAL_PORT_T* output)
    override;
};

struct JPEGEncoderConfig : public BaseEncoderConfig {
  JPEGEncoderConfig() : BaseEncoderConfig{} { }

  virtual MMAL_STATUS_T configure(MMAL_PORT_T* input, MMAL_PORT_T* output)
    override;
};


#endif // ENCODER_CONFIG_HPP
