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


#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <ctime>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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

#include "picam.pb.h"


/**
 * Represents a TCP connection to the receiver service. Call connect() to
 * establish the connection.
 */
class ImageSender {
  public:
    struct Config {
      std::string serverHostname;
      int serverPort;
    };

    ImageSender(const Config& config)
      : mConfig{config},
      mConnected{false},
      mSocket{-1}
    { }

    ~ImageSender() {
      disconnect();
    }

    bool connect() {
      if (mConnected) {
        return true;
      }

      struct addrinfo hints;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = 0;
      hints.ai_protocol = 0;

      struct addrinfo* result = nullptr;
      auto port = std::to_string(mConfig.serverPort);
      int rc = getaddrinfo(mConfig.serverHostname.c_str(), port.c_str(),
          &hints, &result);
      if (rc != 0) {
        Logger::error(__func__, "%s\n", gai_strerror(rc));
        return false;
      }

      struct addrinfo* rp = nullptr;
      int sfd;
      for (rp = result; rp != nullptr; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
          continue;
        }

        if (::connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
          break;
        }

        close(sfd);
      }

      if (rp == nullptr) {
        Logger::error(__func__, "Failed to connect\n");
        return false;
      }

      freeaddrinfo(result);

      mSocket = sfd;
      mConnected = true;

      Logger::info(__func__, "Successfully connected to %s:%s\n",
          mConfig.serverHostname.c_str(), port.c_str());

      return true;
    }

    /**
     * Close the socket (close the TCP connection).
     */
    void disconnect() {
      if (!mConnected) {
        return;
      }

      close(mSocket);
    }

    ssize_t send(std::string& buffer) {
      if (!mConnected) {
        return -1;
      }

      // First send the size so the receiver knows what to expect
      uint32_t size = htonl(buffer.size());
      if (::send(mSocket, &size, sizeof(size), 0) < static_cast<ssize_t>(sizeof(size))) {
        Logger::error(__func__, "Failed to send size\n");
        return -1;
      }

      ssize_t sent = 0;
      while (sent < static_cast<ssize_t>(buffer.size())) {
        ssize_t rc = ::send(mSocket, buffer.data() + sent, buffer.size() - sent, 0);
        if (rc < 0) {
          Logger::error(__func__, "Send failed\n");
          return -1;
        }

        sent += rc;
      }

      return sent;
    }

  private:
    Config mConfig;
    bool mConnected;
    int mSocket;
};


static inline uint32_t align_up(uint32_t n, uint32_t alignment) {
  return ((n + alignment - 1) / alignment) * alignment;
}

static std::unique_ptr<ImageSender> gImageSender{nullptr};
static Camera* gCamera{nullptr};
static int gFrameCount = 0;
static bool gFrameCaptured = false;
size_t encoderCallback(Camera& camera, std::string& data) {
  if (!gImageSender) {
    Logger::warning(__func__, "ImageSender not initialized\n");
    return 0;
  }

  // TODO assuming we always get a whole image--this is not a given
  auto imageMessage = std::make_unique<Image>();
  auto imageMeta = imageMessage->mutable_metadata();
  struct timespec now{};
  // Ignore return value
  clock_gettime(CLOCK_REALTIME, &now);

  imageMeta->set_time_s(now.tv_sec);
  imageMeta->set_time_us(now.tv_nsec / 1000);
  imageMeta->set_width(camera.width());
  imageMeta->set_height(camera.height());

  imageMessage->set_data(data);

  std::string buffer{};
  imageMessage->SerializeToString(&buffer);

  gImageSender->send(buffer);
  gFrameCount++;
  gFrameCaptured = true;

  return data.size();
}

static const int CAMERA_NUM = 0;
static const SensorMode SENSOR_MODE = SM_3280x2464_1;
//static const SensorMode SENSOR_MODE = SM_1920x1080;
// 17 Mbits

int main(int argc, char* argv[]) {

  if (argc < 2) {
    std::cout << "USAGE: " << argv[0] << " <output file> [<record time in s>]"
      << std::endl;
    return 1;
  }

  //unsigned int time = 1;
  //if (argc > 2) {
  //  time = std::atoi(argv[2]);
  //  // If we can't parse the time or it's invalid, reset to 1
  //  if (time < 1) {
  //    time = 1;
  //  }
  //}

  Logger::setLogLevel(LogLevel::DEBUG);

  const auto senderConfig = ImageSender::Config{
    "seadra",
    9000,
  };
  gImageSender = std::make_unique<ImageSender>(senderConfig);
  if (!gImageSender->connect()) {
    Logger::error("ImageSender failed to connect\n");
    return 1;
  }

  bcm_host_init();
  vcos_log_register("picam", VCOS_LOG_CATEGORY);
  Logger::info("bcm_host_init complete\n");

  Camera camera{CAMERA_NUM};
  gCamera = &camera;

  unsigned int width = SENSOR_MODE_WIDTH[SENSOR_MODE];
  unsigned int height = SENSOR_MODE_HEIGHT[SENSOR_MODE];

  //const std::pair<int, int> fps = {1, 10};
  const std::pair<int, int> fps = {0, 1};
  //const std::pair<int, int> fps = {1, 6};


  if (camera.open(SENSOR_MODE, CaptureMode::STILL) != MMAL_SUCCESS) {
    Logger::error("Failed to open camera device\n");
    return 1;
  }
  Logger::info("Camera opened in still mode\n");

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
      .one_shot_stills = 1, // continuous
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
    Logger::info("Camera configured. width=%u, height=%u\n", width, height);
  }

  if (0) {
    // Configure preview encoding
    const MMAL_VIDEO_FORMAT_T formatIn = {
      .width = align_up(width, 32),
      .height = align_up(height, 16),
      .crop = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) },
      .frame_rate = { fps.first, fps.second },
    };

    if (camera.setPreviewFormat(MMAL_ENCODING_OPAQUE, MMAL_ENCODING_I420, formatIn)
        != MMAL_SUCCESS) {
      Logger::error("Failed to set preview format\n");
      return 1;
    }

    Logger::info("Preview configured. width=%u, height=%u @ %u fps\n",
                 width, height,
                 formatIn.frame_rate.num / formatIn.frame_rate.den);
  }

  {
    // Configure still encoding
    const MMAL_VIDEO_FORMAT_T formatIn = {
      .width = align_up(width, 32),
      .height = align_up(height, 16),
      .crop = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) },
      .frame_rate = { 0, 1 },
    };

    if (camera.setStillFormat(MMAL_ENCODING_OPAQUE, MMAL_ENCODING_I420, formatIn)
        != MMAL_SUCCESS) {
      Logger::error("Failed to set still format\n");
      return 1;
    }
  }

  Logger::info("Still output configured. width=%u, height=%u\n", width, height);

  if (0) {
    // Configure the video encoding
    const MMAL_VIDEO_FORMAT_T formatInVideo = {
      .width = align_up(width, 32),
      .height = align_up(height, 16),
      .crop = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) },
      .frame_rate = { fps.first, fps.second },
    };

    // Make sure we have enough buffers
    //if (camera.videoOutputPort()->buffer_num < 3) {
    //  camera.videoOutputPort()->buffer_num = 3;
    //}

    if (camera.setVideoFormat(MMAL_ENCODING_OPAQUE, MMAL_ENCODING_I420,
                              formatInVideo)
        != MMAL_SUCCESS) {
      Logger::error("Failed to set video format\n");
      return 1;
    }

    Logger::info("Video format set. width=%u, height=%u\n",
                  formatInVideo.width, formatInVideo.height);
  }

  if (camera.configurePreview() != MMAL_SUCCESS) {
    return 1;
  }

  // Set some parameters
  // TODO set more parameters
  //setColorEffect
  //setFocus
  if (
      //(camera.setAWBMode(MMAL_PARAM_AWBMODE_OFF) != MMAL_SUCCESS)
      (camera.setAWBMode(MMAL_PARAM_AWBMODE_AUTO) != MMAL_SUCCESS)
      || (camera.setExposureMode(MMAL_PARAM_EXPOSUREMODE_NIGHT) != MMAL_SUCCESS)
      //|| (camera.setExposureMode(MMAL_PARAM_EXPOSUREMODE_AUTO) != MMAL_SUCCESS)
      || (camera.setSharpness(0, 1) != MMAL_SUCCESS)
      || (camera.setContrast(0, 1) != MMAL_SUCCESS)
      || (camera.setBrightness(50, 100) != MMAL_SUCCESS)
      || (camera.setSaturation(0, 1) != MMAL_SUCCESS)
      || (camera.setISO(800) != MMAL_SUCCESS)
      || (camera.setShutterSpeed(60000000) != MMAL_SUCCESS)
      || (camera.setCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_STILLS_CAPTURE) != MMAL_SUCCESS)) {
    Logger::error("Failed to set camera parameters\n");
    return 1;
  }

  // Set up the encoder
  {
    PNGEncoderConfig encoderConfig{};
    if (encoderConfig.configure(camera.encoderInputPort(),
                                camera.encoderOutputPort()) != MMAL_SUCCESS)
    {
      Logger::error("Failed to configure encoder\n");
      return 1;
    }
  }

  if (camera.enableCamera() != MMAL_SUCCESS) {
    Logger::error("Failed to enable camera\n");
    return 1;
  }

  // Now set up the buffers
  // If we do this before creating connections, we get errors when we try to
  // send splitter output buffers to the port
  if (camera.createBufferPools() != MMAL_SUCCESS) {
    Logger::error("Failed to create video port buffer pool\n");
    return 1;
  }

  // Connect all the ports
  if (camera.setUpConnections() != MMAL_SUCCESS) {
    return 1;
  }

  {
    auto* encoderInput = camera.encoderInputPort();
    auto* encoderOutput = camera.encoderOutputPort();
    float frameRateIn = encoderInput->format->es->video.frame_rate.num /
                  encoderInput->format->es->video.frame_rate.den;
    float frameRateOut = encoderOutput->format->es->video.frame_rate.num /
                  encoderOutput->format->es->video.frame_rate.den;
    Logger::debug("Encoder input format: %ux%u @ %f fps, crop=%u %u %u %u\n",
                  encoderInput->format->es->video.width,
                  encoderInput->format->es->video.height,
                  frameRateIn,
                  encoderInput->format->es->video.crop.x,
                  encoderInput->format->es->video.crop.y,
                  encoderInput->format->es->video.crop.width,
                  encoderInput->format->es->video.crop.height
                  );
    Logger::debug("Encoder output format: %ux%u @ %f fps, crop=%u %u %u %u\n",
                  encoderOutput->format->es->video.width,
                  encoderOutput->format->es->video.height,
                  frameRateOut,
                  encoderInput->format->es->video.crop.x,
                  encoderInput->format->es->video.crop.y,
                  encoderInput->format->es->video.crop.width,
                  encoderInput->format->es->video.crop.height
                  );
  }

  //
  // Now that all the ports are set up, let's capture video
  //

  // Wait 30 seconds, then start capture
  for (int i = 0; i < 30; i++) {
    vcos_sleep(1000);
  }

  Logger::debug("Beginning capture\n");

  if (camera.enableCallbacks(encoderCallback) != MMAL_SUCCESS) {
    Logger::error("Failed to enable callbacks\n");
    return 1;
  }
  Logger::debug("Enabled callbacks\n");

  //gFrameCaptured = true;
  while (gFrameCount < 30) {
    // Start the next capture
    if (camera.enableCapture() != MMAL_SUCCESS) {
      Logger::error("Failed to enable capture\n");
      return 1;
    }
    Logger::debug("Enabled capture\n");

    gFrameCaptured = false;

    // Wait for the callback to be called, indicated a frame has been captured
    while (!gFrameCaptured) {
      vcos_sleep(100);
    }

    // Wait for the camera to settle
    vcos_sleep(1000);
  }

  if (camera.disableCapture() != MMAL_SUCCESS) {
    Logger::error("Failed to disable capture\n");
    return 1;
  }


  Logger::debug("Done\n");
  return 0;
}
