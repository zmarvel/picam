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

#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_parameters_camera.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>

#include "encoder_config.hpp"
#include "logging.hpp"

MMAL_STATUS_T BaseEncoderConfig::configure(MMAL_PORT_T* input,
    MMAL_PORT_T* output) {
  MMAL_STATUS_T status = MMAL_SUCCESS;
  mmal_format_copy(output->format, input->format);

  output->format->bitrate = 17000000;
  output->format->es->video.frame_rate.num = 0;
  output->format->es->video.frame_rate.den = 1;

  output->buffer_size = output->buffer_size_recommended;
  if (output->buffer_size < output->buffer_size_min) {
    output->buffer_size = output->buffer_size_min;
  }

  output->buffer_num = output->buffer_num_recommended;
  if (output->buffer_num < output->buffer_num_min) {
    output->buffer_num = output->buffer_num_min;
  }

  return MMAL_SUCCESS;
}

MMAL_STATUS_T H264EncoderConfig::configure(MMAL_PORT_T* input,
    MMAL_PORT_T* output) {
  MMAL_STATUS_T status = BaseEncoderConfig::configure(input, output);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  output->format->bitrate = bitrate;
  output->format->encoding = MMAL_ENCODING_H264;

  // According to RaspiVid.c, we should set the framerate to 0 to make sure
  // connecting it to the input port causes it to get set accordingly
  //output->format->es->video.frame_rate.num = cfg.framerate.first;
  //output->format->es->video.frame_rate.den = cfg.framerate.second;
  output->format->es->video.frame_rate.num = 0;
  output->format->es->video.frame_rate.den = 1;

  status = mmal_port_format_commit(output);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  MMAL_VIDEO_PROFILE_T videoProfile = MMAL_VIDEO_PROFILE_DUMMY;
  switch (profile) {
    case H264EncoderConfig::Profile::BASELINE:
      videoProfile = MMAL_VIDEO_PROFILE_H264_BASELINE;
      break;
    case H264EncoderConfig::Profile::MAIN:
      videoProfile = MMAL_VIDEO_PROFILE_H264_MAIN;
      break;
    case H264EncoderConfig::Profile::HIGH:
      videoProfile = MMAL_VIDEO_PROFILE_H264_HIGH;
      break;
  }

  if (videoProfile == MMAL_VIDEO_PROFILE_DUMMY) {
    return MMAL_EINVAL;
  }

  // TODO: does the hardware even support all these levels?
  MMAL_VIDEO_LEVEL_T videoLevel = MMAL_VIDEO_LEVEL_DUMMY;
  switch (level) {
    case H264EncoderConfig::Level::H264_2:
      videoLevel = MMAL_VIDEO_LEVEL_H264_2;
      break;
    case H264EncoderConfig::Level::H264_21:
      videoLevel = MMAL_VIDEO_LEVEL_H264_21;
      break;
    case H264EncoderConfig::Level::H264_22:
      videoLevel = MMAL_VIDEO_LEVEL_H264_22;
      break;
    case H264EncoderConfig::Level::H264_3:
      videoLevel = MMAL_VIDEO_LEVEL_H264_3;
      break;
    case H264EncoderConfig::Level::H264_31:
      videoLevel = MMAL_VIDEO_LEVEL_H264_31;
      break;
    case H264EncoderConfig::Level::H264_32:
      videoLevel = MMAL_VIDEO_LEVEL_H264_32;
      break;
    case H264EncoderConfig::Level::H264_4:
      videoLevel = MMAL_VIDEO_LEVEL_H264_4;
      break;
    case H264EncoderConfig::Level::H264_41:
      videoLevel = MMAL_VIDEO_LEVEL_H264_41;
      break;
    case H264EncoderConfig::Level::H264_42:
      videoLevel = MMAL_VIDEO_LEVEL_H264_42;
      break;
    case H264EncoderConfig::Level::H264_5:
      videoLevel = MMAL_VIDEO_LEVEL_H264_5;
      break;
    case H264EncoderConfig::Level::H264_51:
      videoLevel = MMAL_VIDEO_LEVEL_H264_51;
      break;
  }

  if (videoLevel == MMAL_VIDEO_LEVEL_DUMMY) {
    return MMAL_EINVAL;
  }

  MMAL_PARAMETER_VIDEO_PROFILE_T profileParam = {
    .hdr = { MMAL_PARAMETER_PROFILE,
      sizeof(MMAL_PARAMETER_VIDEO_PROFILE_T) },
    .profile = {
      { videoProfile, videoLevel },
    },
  };

  status = mmal_port_parameter_set(output, &profileParam.hdr);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  status = mmal_port_parameter_set_boolean(input,
      MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT,
      immutableInputEnabled ?  MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  status = mmal_port_parameter_set_boolean(output,
      MMAL_PARAMETER_VIDEO_ENCODE_INLINE_HEADER,
      inlineHeaderEnabled ? MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  status = mmal_port_parameter_set_boolean(output,
      MMAL_PARAMETER_VIDEO_ENCODE_SPS_TIMING,
      SPSTimingEnabled ?  MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  status = mmal_port_parameter_set_boolean(output,
      MMAL_PARAMETER_VIDEO_ENCODE_INLINE_VECTORS,
      inlineVectorsEnabled ? MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  status = mmal_port_format_commit(output);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  return MMAL_SUCCESS;
}

MMAL_STATUS_T PNGEncoderConfig::configure(MMAL_PORT_T* input,
    MMAL_PORT_T* output) {
  MMAL_STATUS_T status = BaseEncoderConfig::configure(input, output);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  output->format->encoding = MMAL_ENCODING_PNG;

  status = mmal_port_format_commit(output);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  return MMAL_SUCCESS;
}

MMAL_STATUS_T JPEGEncoderConfig::configure(MMAL_PORT_T* input,
    MMAL_PORT_T* output) {
  MMAL_STATUS_T status = BaseEncoderConfig::configure(input, output);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  output->format->encoding = MMAL_ENCODING_JPEG;

  status = mmal_port_format_commit(output);
  if (status != MMAL_SUCCESS) {
    return status;
  }

  return MMAL_SUCCESS;
}
