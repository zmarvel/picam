
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


Camera::Camera(int cameraNum)
  : camera{},
  cameraNum{cameraNum}
{
}

Camera::~Camera() {
  if (mmal_component_destroy(camera) != MMAL_SUCCESS) {
    Logger::warning(CAMERA_NS, "failed to destroy component\n");
  }
}

MMAL_STATUS_T Camera::open() {
  MMAL_STATUS_T status;

  // Allocate the component
  status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "failed to allocate camera resources\n");
    return status;
  }

  // Specify the camera number
  status = mmal_port_parameter_set_uint32(camera->control,
                                          MMAL_PARAMETER_CAMERA_NUM,
                                          cameraNum);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "failed to set camera number\n");
    return status;
  }

  // Configure the resolution
  status = mmal_port_parameter_set_uint32(camera->control,
                                          MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG,
                                          SM_1920x1080);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "failed to set capture resolution\n");
    return status;
  }

  status = mmal_port_enable(getCamera()->control,
                            Camera::controlCallback);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "failed to enable control port\n");
    return status;
  }

  return status;
}

void Camera::controlCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
  Logger::debug(CAMERA_NS, "cameraControlCallback called with cmd=0x%x\n", buffer->cmd);
}

static const int VIDEO_PORT = 1;
MMAL_STATUS_T Camera::setVideoFormat(MMAL_FOURCC_T encoding,
                                     MMAL_FOURCC_T encodingVariant,
                                     const MMAL_VIDEO_FORMAT_T& videoFormat) {
  MMAL_PORT_T* videoPort = getCamera()->output[VIDEO_PORT];
  MMAL_ES_FORMAT_T* format = videoPort->format;
  format->type = MMAL_ES_TYPE_VIDEO;
  format->encoding = encoding;
  format->encoding_variant = encodingVariant;
  memcpy(&format->es->video, &videoFormat, sizeof(MMAL_ES_FORMAT_T));

  format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;

  return mmal_port_format_commit(videoPort);
}

MMAL_STATUS_T Camera::setAWBMode(MMAL_PARAM_AWBMODE_T mode) {
  MMAL_PARAMETER_AWBMODE_T param = {
    .hdr = { MMAL_PARAMETER_AWB_MODE, sizeof(MMAL_PARAMETER_AWBMODE_T) },
    .value = mode,
  };
  return mmal_port_parameter_set(camera->control, &param.hdr);
}
MMAL_STATUS_T Camera::getAWBMode(MMAL_PARAM_AWBMODE_T& mode) {
  MMAL_PARAMETER_AWBMODE_T param = {
    .hdr = { MMAL_PARAMETER_AWB_MODE, sizeof(MMAL_PARAMETER_AWBMODE_T) },
  };
  MMAL_STATUS_T status = mmal_port_parameter_get(camera->control, &param.hdr);
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
  return mmal_port_parameter_set(camera->control, &param.hdr);
}
MMAL_STATUS_T Camera::getColorEffect(bool& enable, uint32_t& u, uint32_t& v) {
  MMAL_PARAMETER_COLOURFX_T param = {
    .hdr = { MMAL_PARAMETER_COLOUR_EFFECT, sizeof(MMAL_PARAMETER_COLOURFX_T) },
  };
  MMAL_STATUS_T status = mmal_port_parameter_get(camera->control, &param.hdr);
  enable = param.enable;
  u = param.u;
  v = param.v;
  return status;
}

#if 0
void Camera::setFocus(MMAL_PARAM_FOCUS_T focus);
void Camera::getFocus(MMAL_PARAM_FOCUS_T& focus);

void Camera::setExposureComp(int32_t comp);
void Camera::setExposureComp(int32_t num, int32_t den);
int32_t Camera::getExposureComp();
void Camera::getExposureComp(int32_t& num, int32_t& den);

void Camera::enableCapture();
void Camera::disableCapture();
#endif

MMAL_STATUS_T Camera::setExposureMode(MMAL_PARAM_EXPOSUREMODE_T mode) {
  MMAL_PARAMETER_EXPOSUREMODE_T param = {
    .hdr = { MMAL_PARAMETER_EXPOSURE_MODE, sizeof(MMAL_PARAMETER_EXPOSUREMODE_T) },
    .value = mode,
  };
  return mmal_port_parameter_set(camera->control, &param.hdr);
}

MMAL_STATUS_T Camera::getExposureMode(MMAL_PARAM_EXPOSUREMODE_T& mode) {
  MMAL_PARAMETER_EXPOSUREMODE_T param = {
    .hdr = { MMAL_PARAMETER_EXPOSURE_MODE, sizeof(MMAL_PARAMETER_EXPOSUREMODE_T) },
  };
  MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
  mode = param.value;
  return status;
}

#if 0
MMAL_STATUS_T Camera::setExpMeteringMode(MMAL_PARAM_EXPOSUREMETERINGMODE_T mode);
MMAL_STATUS_T Camera::getExpMeteringMode(MMAL_PARAM_EXPOSUREMETERINGMODE_T& mode);

MMAL_PARAM_FOCUS_STATUS_T Camera::getFocusStatus();
#endif

MMAL_STATUS_T Camera::setCameraConfig(const MMAL_PARAMETER_CAMERA_CONFIG_T& config) {
  return mmal_port_parameter_set(camera->control, &config.hdr);
}

#if 0
MMAL_PARAM_CAPTURE_STATUS_T Camera::getCaptureStatus();

void Camera::setFrameRate(int32_t num, int32_t den);
void Camera::getFrameRate(int32_t& num, int32_t& den);

void Camera::setUseSTC(MMAL_CAMERA_STC_MODE_T mode);
MMAL_CAMERA_STC_MODE_T Camera::getUseSTC();

void Camera::getSensorInformation(MMAL_PARAMETER_SENSOR_INFORMATION_T& info);

#endif
MMAL_STATUS_T Camera::setSharpness(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(camera->control,
                                          MMAL_PARAMETER_SHARPNESS,
                                          { num, den });
}

MMAL_STATUS_T Camera::setContrast(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(camera->control,
                                          MMAL_PARAMETER_CONTRAST,
                                          { num, den });
}

MMAL_STATUS_T Camera::setBrightness(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(camera->control,
                                          MMAL_PARAMETER_BRIGHTNESS,
                                          { num, den });
}

MMAL_STATUS_T Camera::setSaturation(int32_t num, int32_t den) {
  return mmal_port_parameter_set_rational(camera->control,
                                          MMAL_PARAMETER_SATURATION,
                                          { num, den });
}

MMAL_STATUS_T Camera::setISO(uint32_t iso) {
  return mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_ISO,
                                        iso);
}

MMAL_STATUS_T Camera::setCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_T useCase) {
  MMAL_PARAMETER_CAMERA_USE_CASE_T param = {
    .hdr = {
      MMAL_PARAMETER_CAMERA_USE_CASE,
      sizeof(MMAL_PARAMETER_CAMERA_USE_CASE_T),
    },
    .use_case = useCase,
  };
  return mmal_port_parameter_set(camera->control, &param.hdr);
}

#if 0
MMAL_STATUS_T Camera::getCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_T& useCase) {
}

void Camera::setShutterSpeed(uint32_t speed);
uint32_t Camera::getShutterSpeed();

void Camera::setPrivacyIndicator(MMAL_PARAM_PRIVACY_INDICATOR_T mode);
MMAL_PARAM_PRIVACY_INDICATOR_T Camera::getPrivacyIndicator();

void Camera::setAnalogGain(int32_t num, int32_t den);
void Camera::getAnalogGain(int32_t& num, int32_t& den);

void Camera::setDigitalGain(int32_t num, int32_t den);
void Camera::getDigitalGain(int32_t& num, int32_t& den);
#endif
