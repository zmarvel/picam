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

#include <ios>
#include <fstream>

#include "camera.hpp"
#include "logging.hpp"



const unsigned int SENSOR_MODE_WIDTH[NUM_SENSOR_MODES] {
  0,
  1920,
  3280,
  3280,
  1640,
  1640,
  1282,
  640,
};

const unsigned int SENSOR_MODE_HEIGHT[NUM_SENSOR_MODES] {
  0,
  1080,
  2464,
  2464,
  1232,
  922,
  720,
  480,
};

static const std::string CAMERA_NS = "Camera: ";


Camera::Camera(int mCameraNum)
  : mCameraNum{mCameraNum}
  , mSensorMode{}
  , mCaptureMode{}
  , mCamera{nullptr}
  , mEncoder{nullptr}
  , mPreview{nullptr}
  , mEncoderPool{nullptr}
  , mVideoEncoderConnection{nullptr}
{
}

Camera::~Camera() {
  // Disable ports so callbacks won't try to use resources that will be freed later
  if ((mCamera != nullptr) && (mmal_port_disable(encoderOutputPort()) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to disable encoder output\n");
  }

  if ((mCamera != nullptr) && (mmal_port_disable(getCamera()->control) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to disable encoder output\n");
  }

  // Clean up connections
  if ((mVideoEncoderConnection != nullptr) && (mmal_connection_destroy(mVideoEncoderConnection) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy video -> encoder connection\n");
  }

  // Clean up pools
  if (mEncoderPool != nullptr) {
    mmal_pool_destroy(mEncoderPool);
  }

  // Clean up components
  if ((mCamera != nullptr) && (mmal_component_destroy(mCamera) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy camera component\n");
  }

  if ((mEncoder != nullptr) && (mmal_component_destroy(mEncoder) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy encoder component\n");
  }

  if ((mPreview != nullptr) && (mmal_component_destroy(mPreview) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy null preview component\n");
  }
}

MMAL_STATUS_T Camera::open(SensorMode sensorMode, CaptureMode captureMode) {
  MMAL_STATUS_T status;

  //
  // Set up the camera
  //

  // Allocate the camera component
  status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &mCamera);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "failed to allocate camera resources\n");
    return status;
  }

  // Specify the camera number
  status = mmal_port_parameter_set_uint32(mCamera->control,
                                          MMAL_PARAMETER_CAMERA_NUM,
                                          mCameraNum);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "failed to set camera number\n");
    return status;
  }

  if (mCamera->output_num < 3) {
    Logger::error(__func__, "invalid camera output number\n");
    return MMAL_ENOSYS;
  }

  // Configure the resolution
  status = setSensorMode(sensorMode);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "failed to set sensor mode\n");
    return status;
  }

  status = mmal_port_enable(getCamera()->control, Camera::controlCallback);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "failed to enable control port\n");
    return status;
  }

  //
  // Set up the encoder
  //

  status = setCaptureMode(captureMode);

  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "failed to allocate encoder resources\n");
    return status;
  }

  if ((mEncoder->input_num < 1) || (mEncoder->output_num < 1)) {
    Logger::error(__func__, "invalid encoder input/output number\n");
    return MMAL_ENOSYS;
  }

  return status;
}

MMAL_STATUS_T Camera::configurePreview() {
  MMAL_STATUS_T status;

  status = mmal_component_create("vc.null_sink", &mPreview);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to create null preview component\n");
    return status;
  }

  status = mmal_component_enable(mPreview);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to enable null preview component\n");
    return status;
  }

  return status;
}

MMAL_STATUS_T Camera::setCaptureMode(CaptureMode mode) {
  MMAL_STATUS_T status = MMAL_SUCCESS;
  if (mEncoder != nullptr) {
    status = mmal_component_destroy(mEncoder);
    if (status != MMAL_SUCCESS) {
      Logger::error(__func__, "Failed to destroy previous encoder\n");
      return status;
    }
  }

  switch (mode) {
    case CaptureMode::STILL:
      status = mmal_component_create(MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER,
                                     &mEncoder);
      break;
    case CaptureMode::VIDEO:
      status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER,
                                     &mEncoder);
      break;
    default:
      status = MMAL_EINVAL;
  }

  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to create \n");
  }

  mCaptureMode = mode;

  return status;
}

SensorMode Camera::sensorMode() const {
  return mSensorMode;
}

MMAL_STATUS_T Camera::setSensorMode(SensorMode mode) {
  mSensorMode = mode;
  return mmal_port_parameter_set_uint32(mCamera->control,
                                        MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG,
                                        mode);
}

uint32_t Camera::width() const {
  return SENSOR_MODE_WIDTH[mSensorMode];
}

uint32_t Camera::height() const {
  return SENSOR_MODE_HEIGHT[mSensorMode];
}

// Building with GCC 6.3.0, [[maybe_unused]] is not yet supported, even with
// -std=c++17.
__attribute__((unused)) static void printBufferFlags(const char* indent,
    const char* sep, MMAL_BUFFER_HEADER_T* buffer) {
  if (indent == nullptr) {
    indent = "";
  }
  if (sep == nullptr) {
    sep = "\n";
  }

  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_EOS) {
    printf("%sEOS%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_START) {
    printf("%sFRAME_START%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_END) {
    printf("%sFRAME_END%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME) {
    printf("%sFRAME%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_KEYFRAME) {
    printf("%sKEYFRAME%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_DISCONTINUITY) {
    printf("%sDISCONTINUITY%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_CONFIG) {
    printf("%sCONFIG%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_ENCRYPTED) {
    printf("%sENCRYPTED%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_CODECSIDEINFO) {
    printf("%sCODECSIDEINFO%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAGS_SNAPSHOT) {
    printf("%sSNAPSHOT%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_CORRUPTED) {
    printf("%sCORRUPTED%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED) {
    printf("%sTRANSMISSION_FAILED%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_DECODEONLY) {
    printf("%sDECODEONLY%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_NAL_END) {
    printf("%sNAL_END%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_USER0) {
    printf("%sUSER0%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_USER1) {
    printf("%sUSER1%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_USER2) {
    printf("%sUSER2%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_USER3) {
    printf("%sUSER3%s", indent, sep);
  }

  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FORMAT_SPECIFIC_START_BIT) {
    printf("%sSPECIFIC_START_BIT%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FORMAT_SPECIFIC_START) {
    printf("%sSPECIFIC_START%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_VIDEO_FLAG_INTERLACED) {
    printf("%sINTERLACED%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_VIDEO_FLAG_TOP_FIELD_FIRST) {
    printf("%sTOP_FIELD_FIRST%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_VIDEO_FLAG_DISPLAY_EXTERNAL) {
    printf("%sDISPLAY_EXTERNAL%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_VIDEO_FLAG_PROTECTED) {
    printf("%sPROTECTED%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_VIDEO_FLAG_COLUMN_LOG2_SHIFT) {
    printf("%sCOLUMN_LOG2_SHIFT%s", indent, sep);
  }
  if (buffer->flags & MMAL_BUFFER_HEADER_VIDEO_FLAG_COLUMN_LOG2_MASK) {
    printf("%sCOLUMN_LOG2_MASK%s", indent, sep);
  }
}

//Camera::encoderCallbackType encoderCallback() {
//  return std::move(mEncoderCallback);
//}

void Camera::controlCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
  Logger::debug(CAMERA_NS, "Camera::controlCallback called with cmd=0x%x\n", buffer->cmd);
}

static std::string imageBuffer{};
void Camera::encoderCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
  //Logger::debug(CAMERA_NS, "Camera::encoderCallback called\n");
  Camera* pCamera = reinterpret_cast<Camera*>(port->userdata);

  size_t nBytes = 0;
  static size_t nRcvd = 0;
  mmal_buffer_header_mem_lock(buffer);
  nBytes = buffer->length;
  imageBuffer.append(reinterpret_cast<char*>(buffer->data + buffer->offset), nBytes);
  mmal_buffer_header_mem_unlock(buffer);

  nRcvd += nBytes;
  if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED) {
    Logger::warning("Buffer transmission failed\n");
    nRcvd = 0;
  } else if (buffer->flags & MMAL_BUFFER_HEADER_FLAG_FRAME_END) {
    pCamera->mEncoderCallback(*pCamera, imageBuffer);
    imageBuffer.clear();
    Logger::info("Frame received: %u bytes\n", nRcvd);
    nRcvd = 0;
  }

  mmal_buffer_header_release(buffer);

  if (port->is_enabled) {
    MMAL_BUFFER_HEADER_T* newBuffer =
      mmal_queue_get(pCamera->getEncoderBufferPool()->queue);
    if (newBuffer == nullptr) {
      Logger::warning(__func__, "Failed to get new buffer\n");
    } else if (mmal_port_send_buffer(port, newBuffer) != MMAL_SUCCESS) {
      Logger::warning(__func__, "Failed to send new buffer to port\n");
    }
  }
}

enum {
  PREVIEW_PORT = 0,
  VIDEO_PORT = 1,
  STILL_PORT = 2,
};

MMAL_STATUS_T Camera::setVideoFormat(MMAL_FOURCC_T encoding,
                                     MMAL_FOURCC_T encodingVariant,
                                     const MMAL_VIDEO_FORMAT_T& videoFormat) {
  MMAL_PORT_T* videoPort = captureOutputPort();
  MMAL_ES_FORMAT_T* format = videoPort->format;
  //format->type = MMAL_ES_TYPE_VIDEO;
  format->encoding = encoding;
  format->encoding_variant = encodingVariant;
  memcpy(&format->es->video, &videoFormat, sizeof(MMAL_VIDEO_FORMAT_T));

  //format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;

  return mmal_port_format_commit(videoPort);
}

MMAL_STATUS_T Camera::setPreviewFormat(MMAL_FOURCC_T encoding,
                                       MMAL_FOURCC_T encodingVariant,
                                       const MMAL_VIDEO_FORMAT_T& videoFormat) {
  MMAL_PORT_T* port = getCamera()->output[PREVIEW_PORT];
  MMAL_ES_FORMAT_T* format = port->format;
  //format->type = MMAL_ES_TYPE_VIDEO;
  format->encoding = encoding;
  format->encoding_variant = encodingVariant;
  memcpy(&format->es->video, &videoFormat, sizeof(MMAL_VIDEO_FORMAT_T));

  //format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;

  return mmal_port_format_commit(port);
}

MMAL_STATUS_T Camera::setStillFormat(MMAL_FOURCC_T encoding,
                                     MMAL_FOURCC_T encodingVariant,
                                     const MMAL_VIDEO_FORMAT_T& videoFormat) {
  MMAL_PORT_T* port = getCamera()->output[STILL_PORT];
  MMAL_ES_FORMAT_T* format = port->format;
  format->encoding = encoding;
  format->encoding_variant = encodingVariant;
  memcpy(&format->es->video, &videoFormat, sizeof(MMAL_VIDEO_FORMAT_T));

  //format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;

  return mmal_port_format_commit(port);
}

MMAL_PORT_T* Camera::captureOutputPort() const {
  switch (mCaptureMode) {
    case CaptureMode::VIDEO:
      return getCamera()->output[VIDEO_PORT];
    case CaptureMode::STILL:
      return getCamera()->output[STILL_PORT];
    default:
      return nullptr;
  }
}

MMAL_PORT_T* Camera::encoderInputPort() const {
  return getEncoder()->input[0];
}

MMAL_PORT_T* Camera::encoderOutputPort() const {
  return getEncoder()->output[0];
}

MMAL_STATUS_T Camera::createBufferPools() {
  {
    MMAL_PORT_T* encoderOutput = encoderOutputPort();
    mEncoderPool = mmal_port_pool_create(encoderOutput, encoderOutput->buffer_num,
                                        encoderOutput->buffer_size);
    if (mEncoderPool == nullptr) {
      Logger::error(__func__, "Failed to allocate encoder output buffer pool\n");
      return MMAL_ENOMEM;
    }
    Logger::info(__func__, "Created encoder output buffer pool with %d "
                 "buffers of size %d B\n",
                 encoderOutput->buffer_num, encoderOutput->buffer_size);
  }

  return MMAL_SUCCESS;
}

MMAL_POOL_T* Camera::getEncoderBufferPool() {
  return mEncoderPool;
}

MMAL_STATUS_T Camera::enableCamera() {
  MMAL_STATUS_T status;
  status = mmal_component_enable(mCamera);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "Failed to enable camera component\n");
    return status;
  }

  return status;
}

MMAL_STATUS_T Camera::enableEncoder() {
  MMAL_STATUS_T status = mmal_component_enable(mEncoder);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "Failed to enable encoder component\n");
    return status;
  }

  return status;
}

MMAL_STATUS_T Camera::enableCallbacks(encoderCallbackType encoderCallback) {
  MMAL_STATUS_T status;

  encoderOutputPort()->userdata = reinterpret_cast<MMAL_PORT_USERDATA_T*>(this);
  status = mmal_port_enable(encoderOutputPort(), Camera::encoderCallback);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to set up encoder output callback\n");
    return status;
  }
  mEncoderCallback = std::move(encoderCallback);

  int n = mmal_queue_length(getEncoderBufferPool()->queue);
  for (int q = 0; q < n; q++) {
    MMAL_BUFFER_HEADER_T* buf = mmal_queue_get(getEncoderBufferPool()->queue);
    if (mmal_port_send_buffer(encoderOutputPort(), buf) != MMAL_SUCCESS) {
      Logger::warning(__func__, "Failed to send buffer to encoder output port\n");
    }
  }

  return status;
}

MMAL_STATUS_T Camera::disableCallbacks() {
  return mmal_port_disable(encoderOutputPort());
}

MMAL_STATUS_T Camera::setUpConnections() {
  {
    MMAL_PORT_T* encoderInput = encoderInputPort();
    MMAL_PORT_T* captureOutput = captureOutputPort();
    MMAL_STATUS_T status;
    // video -> encoder
    status = mmal_connection_create(&mVideoEncoderConnection,
                                    captureOutput, encoderInput,
                                    MMAL_CONNECTION_FLAG_TUNNELLING |
                                    MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
    if (status != MMAL_SUCCESS) {
      Logger::error(__func__, "Failed to connect video output to encoder input[0]\n");
      return status;
    }

    status = mmal_connection_enable(mVideoEncoderConnection);
    if (status != MMAL_SUCCESS) {
      Logger::error(__func__, "Failed to enable video -> encoder connection\n");
      return status;
    }
  }

  {
    // NOTE re: encoder broken
    // this doesn't change anything??
    MMAL_PORT_T
      * previewOutput = getCamera()->output[PREVIEW_PORT],
      * nullInput = mPreview->input[0];
    MMAL_STATUS_T status;
    status = mmal_connection_create(&mPreviewNullConnection,
                                    previewOutput, nullInput,
                                    MMAL_CONNECTION_FLAG_TUNNELLING |
                                    MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
    if (status != MMAL_SUCCESS) {
      Logger::error(__func__, "Failed to connect preview output[0] to null input\n");
      return status;
    }

    status = mmal_connection_enable(mPreviewNullConnection);
    if (status != MMAL_SUCCESS) {
      Logger::error(__func__, "Failed to enable preview -> null connection\n");
      return status;
    }
  }

  return MMAL_SUCCESS;
}

MMAL_STATUS_T Camera::setAWBMode(MMAL_PARAM_AWBMODE_T mode) {
  MMAL_PARAMETER_AWBMODE_T param = {
    .hdr = { MMAL_PARAMETER_AWB_MODE, sizeof(MMAL_PARAMETER_AWBMODE_T) },
    .value = mode,
  };
  return mmal_port_parameter_set(mCamera->control, &param.hdr);
}

MMAL_STATUS_T Camera::getAWBMode(MMAL_PARAM_AWBMODE_T& mode) {
  MMAL_PARAMETER_AWBMODE_T param = {
    .hdr = { MMAL_PARAMETER_AWB_MODE, sizeof(MMAL_PARAMETER_AWBMODE_T) },
  };
  MMAL_STATUS_T status = mmal_port_parameter_get(mCamera->control, &param.hdr);
  mode = param.value;
  return status;
}

MMAL_STATUS_T Camera::setColorEffect(bool enabled, uint32_t u, uint32_t v) {
  MMAL_PARAMETER_COLOURFX_T param = {
    .hdr = { MMAL_PARAMETER_COLOUR_EFFECT, sizeof(MMAL_PARAMETER_COLOURFX_T) },
    .enable = enabled,
    .u = u,
    .v = v,
  };
  return mmal_port_parameter_set(mCamera->control, &param.hdr);
}

MMAL_STATUS_T Camera::getColorEffect(bool& enable, uint32_t& u, uint32_t& v) {
  MMAL_PARAMETER_COLOURFX_T param = {
    .hdr = { MMAL_PARAMETER_COLOUR_EFFECT, sizeof(MMAL_PARAMETER_COLOURFX_T) },
  };

  MMAL_STATUS_T status = mmal_port_parameter_get(mCamera->control, &param.hdr);
  enable = param.enable;
  u = param.u;
  v = param.v;
  return status;
}

MMAL_STATUS_T Camera::enableCapture() {
  return mmal_port_parameter_set_boolean(captureOutputPort(),
                                         MMAL_PARAMETER_CAPTURE, MMAL_TRUE);
}

MMAL_STATUS_T Camera::disableCapture() {
  return mmal_port_parameter_set_boolean(captureOutputPort(),
                                         MMAL_PARAMETER_CAPTURE, MMAL_FALSE);
}

MMAL_STATUS_T Camera::setExposureMode(MMAL_PARAM_EXPOSUREMODE_T mode) {
  MMAL_PARAMETER_EXPOSUREMODE_T param = {
    .hdr = { MMAL_PARAMETER_EXPOSURE_MODE, sizeof(MMAL_PARAMETER_EXPOSUREMODE_T) },
    .value = mode,
  };
  return mmal_port_parameter_set(mCamera->control, &param.hdr);
}

MMAL_STATUS_T Camera::getExposureMode(MMAL_PARAM_EXPOSUREMODE_T& mode) {
  MMAL_PARAMETER_EXPOSUREMODE_T param = {
    .hdr = { MMAL_PARAMETER_EXPOSURE_MODE, sizeof(MMAL_PARAMETER_EXPOSUREMODE_T) },
  };
  MMAL_STATUS_T status = mmal_port_parameter_set(mCamera->control, &param.hdr);
  mode = param.value;
  return status;
}

MMAL_STATUS_T Camera::setCameraConfig(const MMAL_PARAMETER_CAMERA_CONFIG_T& config) {
  return mmal_port_parameter_set(mCamera->control, &config.hdr);
}

MMAL_STATUS_T Camera::setSharpness(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(mCamera->control,
                                          MMAL_PARAMETER_SHARPNESS,
                                          { num, den });
}

MMAL_STATUS_T Camera::setContrast(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(mCamera->control,
                                          MMAL_PARAMETER_CONTRAST,
                                          { num, den });
}

MMAL_STATUS_T Camera::setBrightness(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(mCamera->control,
                                          MMAL_PARAMETER_BRIGHTNESS,
                                          { num, den });
}

MMAL_STATUS_T Camera::setSaturation(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(mCamera->control,
                                          MMAL_PARAMETER_SATURATION,
                                          { num, den });
}

MMAL_STATUS_T Camera::setISO(uint32_t iso) {
  return mmal_port_parameter_set_uint32(mCamera->control, MMAL_PARAMETER_ISO,
                                        iso);
}

MMAL_STATUS_T Camera::setShutterSpeed(uint32_t speed) {
  return mmal_port_parameter_set_uint32(mCamera->control,
                                        MMAL_PARAMETER_SHUTTER_SPEED, speed);
}

MMAL_STATUS_T Camera::setCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_T useCase) {
  MMAL_PARAMETER_CAMERA_USE_CASE_T param = {
    .hdr = {
      MMAL_PARAMETER_CAMERA_USE_CASE,
      sizeof(MMAL_PARAMETER_CAMERA_USE_CASE_T),
    },
    .use_case = useCase,
  };
  return mmal_port_parameter_set(mCamera->control, &param.hdr);
}
