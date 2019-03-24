

#include <stdio.h>
#include <unistd.h>


#include "interface/mmal/mmal.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/mmal/mmal_parameters_camera.h"

#include "logging.hpp"
#include "camera.hpp"




static const int CAMERA_NUM = 0;
static const int SENSOR_MODE = SM_1920x1080;


int main(int argc, char* argv[]) {
  Camera camera{CAMERA_NUM};

  unsigned int width = SENSOR_MODE_WIDTH[SENSOR_MODE];
  unsigned int height = SENSOR_MODE_HEIGHT[SENSOR_MODE];

  if (camera.open() != MMAL_SUCCESS) {
    Logger::error("Failed to open camera device\n");
    return 1;
  }

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
    .num_preview_video_frames = 30, // ??
    .stills_capture_circular_buffer_height = 0, // ??
    .fast_preview_resume = 0,
    .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RAW_STC, // is this what we want??
  };

  if (camera.setCameraConfig(cameraConfig) != MMAL_SUCCESS) {
    Logger::error("Failed to set cameraConfig\n");
    return 1;
  }


  // Configure the video encoding
  const MMAL_VIDEO_FORMAT_T formatInVideo = {
    .width = width,
    .height = height,
    .crop = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) },
    .frame_rate = { 30, 1 },
  };

  if (camera.setVideoFormat(MMAL_ENCODING_I420, MMAL_ENCODING_I420, formatInVideo)
      != MMAL_SUCCESS) {
    Logger::error("Failed to set video format\n");
    return 1;
  }


  // Set the parameters
  camera.setAWBMode(MMAL_PARAM_AWBMODE_OFF);
  //setColorEffect
  //setFocus
  camera.setExposureMode(MMAL_PARAM_EXPOSUREMODE_NIGHT);

  camera.setSharpness(0, 1);
  camera.setContrast(0, 1);
  camera.setBrightness(0, 1);
  camera.setSaturation(0, 1);
  camera.setISO(0);
  camera.setCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_VIDEO_CAPTURE);




  Logger::debug("Done\n");
  return 0;
}
