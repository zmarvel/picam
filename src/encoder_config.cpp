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
