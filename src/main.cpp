

#include <cstdio>
#include <unistd.h>
#include <ctime>
#include <iostream>

#include <bcm_host.h>
#include <interface/vcos/vcos.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_logging.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_connection.h>
#include <interface/mmal/mmal_parameters_camera.h>

#include "logging.hpp"
#include "camera.hpp"
#include "encoder_config.hpp"



static inline uint32_t align_up(uint32_t n, uint32_t alignment) {
  return ((n + alignment - 1) / alignment) * alignment;
}


static const int CAMERA_NUM = 0;
static const SensorMode SENSOR_MODE = SM_1920x1080;
// 17 Mbits
static const int H264_BITRATE = 17000000;

int main(int argc, char* argv[]) {

  if (argc < 2) {
    std::cout << "USAGE: " << argv[0] << " <output file>" << std::endl;
    return 1;
  }

  bcm_host_init();
  vcos_log_register("picam", VCOS_LOG_CATEGORY);

  Camera camera{CAMERA_NUM};

  unsigned int width = SENSOR_MODE_WIDTH[SENSOR_MODE];
  unsigned int height = SENSOR_MODE_HEIGHT[SENSOR_MODE];

  Logger::setLogLevel(LogLevel::DEBUG);


  if (camera.open(SENSOR_MODE) != MMAL_SUCCESS) {
    Logger::error("Failed to open camera device\n");
    return 1;
  }

  if (camera.openOutputFile(argv[1]) != MMAL_SUCCESS) {
    Logger::error("Failed to open output file\n");
    return 1;
  }

  {
    // Set the camera config
    MMAL_PARAMETER_CAMERA_CONFIG_T cameraConfig = {
      .hdr = {
        MMAL_PARAMETER_CAMERA_CONFIG,
        sizeof(MMAL_PARAMETER_CAMERA_CONFIG_T),
      },
      .max_stills_w = width,
      .max_stills_h = height,
      .stills_yuv422 = 0,
      .one_shot_stills = 0, // continuous
      .max_preview_video_w = width,
      .max_preview_video_h = height,
      .num_preview_video_frames = 3, // ??
      .stills_capture_circular_buffer_height = 0, // ??
      .fast_preview_resume = 0,
      .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RAW_STC, // is this what we want??
    };

    if (camera.setCameraConfig(cameraConfig) != MMAL_SUCCESS) {
      Logger::error("Failed to set cameraConfig\n");
      return 1;
    }
  }

  {
    // Configure the video encoding
    const MMAL_VIDEO_FORMAT_T formatInVideo = {
      .width = align_up(width, 32),
      .height = align_up(height, 16),
      .crop = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) },
      .frame_rate = { 30, 1 },
    };

    // Make sure we have enough buffers
    if (camera.getVideoOutputPort()->buffer_num < 3) {
      camera.getVideoOutputPort()->buffer_num = 3;
    }

    if (camera.setVideoFormat(MMAL_ENCODING_OPAQUE, 0,
                              formatInVideo)
        != MMAL_SUCCESS) {
      Logger::error("Failed to set video format\n");
      return 1;
    }

    Logger::debug("Video format: width=%u, height=%u\n",
                  formatInVideo.width, formatInVideo.height);
  }

  {
    // Configure preview encoding
    const MMAL_VIDEO_FORMAT_T formatIn = {
      .width = align_up(width, 32),
      .height = align_up(height, 16),
      .crop = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) },
      .frame_rate = { 30, 1 },
    };

    if (camera.setPreviewFormat(MMAL_ENCODING_OPAQUE, 0, formatIn)
        != MMAL_SUCCESS) {
      Logger::error("Failed to set preview format\n");
      return 1;
    }
  }

  {
    // Configure still encoding
    const MMAL_VIDEO_FORMAT_T formatIn = {
      .width = align_up(width, 32),
      .height = align_up(height, 16),
      .crop = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) },
      .frame_rate = { 0, 1 },
    };

    if (camera.setStillFormat(MMAL_ENCODING_OPAQUE, 0, formatIn)
        != MMAL_SUCCESS) {
      Logger::error("Failed to set still format\n");
      return 1;
    }
  }


  if (camera.configurePreview() != MMAL_SUCCESS) {
    return 1;
  }

  if (camera.configureSplitter() != MMAL_SUCCESS) {
    Logger::error("Failed to configure splitter\n");
    return 1;
  }

  // Set some parameters
  // TODO set more parameters
  //setColorEffect
  //setFocus
  if ((camera.setAWBMode(MMAL_PARAM_AWBMODE_OFF) != MMAL_SUCCESS)
      || (camera.setExposureMode(MMAL_PARAM_EXPOSUREMODE_NIGHT) != MMAL_SUCCESS)
      || (camera.setSharpness(0, 1) != MMAL_SUCCESS)
      || (camera.setContrast(0, 1) != MMAL_SUCCESS)
      || (camera.setBrightness(0, 1) != MMAL_SUCCESS)
      || (camera.setSaturation(0, 1) != MMAL_SUCCESS)
      || (camera.setISO(0) != MMAL_SUCCESS)
      || (camera.setCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_VIDEO_CAPTURE) != MMAL_SUCCESS)) {
    Logger::error("Failed to set camera parameters\n");
    return 1;
  }

  // Set up the encoder
  {
    H264EncoderConfig encoderConfig = H264EncoderConfig::defaultConfig();
    if (camera.configureEncoder(encoderConfig) != MMAL_SUCCESS) {
      Logger::error("Failed to configure encoder\n");
      return 1;
    }
  }

  if (camera.enableCamera() != MMAL_SUCCESS) {
    Logger::error("Failed to enable camera\n");
    return 1;
  }
  if (camera.enableSplitter() != MMAL_SUCCESS) {
    Logger::error("Failed to enable splitter\n");
    return 1;
  }
  if (camera.enableEncoder() != MMAL_SUCCESS) {
    Logger::error("Failed to enable encoder\n");
    return 1;
  }

  // Connect all the ports
  if (camera.setUpConnections() != MMAL_SUCCESS) {
    return 1;
  }

  // Now set up the buffers
  // If we do this before creating connections, we get errors when we try to
  // send splitter output buffers to the port
  if (camera.createBufferPools() != MMAL_SUCCESS) {
    Logger::error("Failed to create video port buffer pool\n");
    return 1;
  }

  if (camera.enableCallbacks() != MMAL_SUCCESS) {
    Logger::error("Failed to enable camera callbacks\n");
    return 1;
  }

  {
    MMAL_PORT_T* encoderOutput = camera.getEncoderOutputPort();
    int n = mmal_queue_length(camera.getEncoderBufferPool()->queue);
    for (int q = 0; q < n; q++) {
      MMAL_BUFFER_HEADER_T* buf = mmal_queue_get(camera.getEncoderBufferPool()->queue);
      if (mmal_port_send_buffer(encoderOutput, buf) != MMAL_SUCCESS) {
        Logger::warning("Failed to send buffer to encoder output port\n");
      }
    }
  }

  {
    MMAL_PORT_T* splitterRawOutput = camera.getSplitterRawOutputPort();
    int n = mmal_queue_length(camera.getSplitterRawBufferPool()->queue);
    for (int q = 0; q < n; q++) {
      MMAL_BUFFER_HEADER_T* buf = mmal_queue_get(camera.getSplitterRawBufferPool()->queue);
      if (mmal_port_send_buffer(splitterRawOutput, buf) != MMAL_SUCCESS) {
        Logger::warning("Failed to send buffer to splitter output port\n");
      }
    }
  }


  //
  // Now that all the ports are set up, let's capture video
  //

  if (camera.enableCapture() != MMAL_SUCCESS) {
    Logger::error("Failed to enable capture\n");
    return 1;
  }

  for (int i = 0; i < 20; i++) {
    vcos_sleep(100);
  }

  if (camera.disableCapture() != MMAL_SUCCESS) {
    Logger::error("Failed to disable capture\n");
    return 1;
  }

  camera.closeOutputFile();
  

  Logger::debug("Done\n");
  return 0;
}
