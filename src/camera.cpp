
#include <ios>

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
  : cameraNum{cameraNum}
  , camera{nullptr}
  , encoder{nullptr}
  , preview{nullptr}
  , encoderPool{nullptr}
  , videoEncoderConnection{nullptr}
  , encodedOutput{}
  , encodedOutputOpen{false}
{
}

Camera::~Camera() {
  // Clean up connections
  if ((videoEncoderConnection != nullptr) && (mmal_connection_destroy(videoEncoderConnection) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy video -> encoder connection\n");
  }


  // Clean up pools
  if (encoderPool != nullptr) {
    mmal_pool_destroy(encoderPool);
  }


  // Clean up components
  if ((camera != nullptr) && (mmal_component_destroy(camera) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy camera component\n");
  }

  if ((encoder != nullptr) && (mmal_component_destroy(encoder) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy encoder component\n");
  }

  if ((preview != nullptr) && (mmal_component_destroy(preview) != MMAL_SUCCESS)) {
    Logger::warning(__func__, "failed to destroy null preview component\n");
  }
}

MMAL_STATUS_T Camera::open(SensorMode mode) {
  MMAL_STATUS_T status;

  //
  // Set up the camera
  //

  // Allocate the camera component
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

  if (camera->output_num < 1) {
    Logger::error(CAMERA_NS, "invalid camera output number\n");
    return MMAL_ENOSYS;
  }

  // Configure the resolution
  status = mmal_port_parameter_set_uint32(camera->control,
                                          MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG,
                                          mode);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "failed to set sensor mode\n");
    return status;
  }

  status = mmal_port_enable(getCamera()->control, Camera::controlCallback);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "failed to enable control port\n");
    return status;
  }

  //
  // Set up the encoder
  //

  status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER, &encoder);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "failed to allocate encoder resources\n");
    return status;
  }

  if ((encoder->input_num < 1) || (encoder->output_num < 1)) {
    Logger::error(CAMERA_NS, "invalid encoder input/output number\n");
    return MMAL_ENOSYS;
  }

  return status;
}

MMAL_STATUS_T Camera::configurePreview() {
  MMAL_STATUS_T status;

  status = mmal_component_create("vc.null_sink", &preview);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to create null preview component\n");
    return status;
  }

  status = mmal_component_enable(preview);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to enable null preview component\n");
    return status;
  }

  return status;
}

MMAL_STATUS_T Camera::configureEncoder(H264EncoderConfig& cfg) {
  // Now let's set up the encoder
  MMAL_PORT_T* encoderOutput = getEncoderOutputPort();
  MMAL_PORT_T* encoderInput = getEncoderInputPort();
  MMAL_STATUS_T status;

  mmal_format_copy(encoderOutput->format, encoderInput->format);
  encoderOutput->format->encoding = MMAL_ENCODING_H264;

  encoderOutput->format->bitrate = cfg.bitrate;

  encoderOutput->buffer_size = encoderOutput->buffer_size_recommended;
  if (encoderOutput->buffer_size < encoderOutput->buffer_size_min) {
    encoderOutput->buffer_size = encoderOutput->buffer_size_min;
  }

  encoderOutput->buffer_num = encoderOutput->buffer_num_recommended;
  if (encoderOutput->buffer_num < encoderOutput->buffer_num_min) {
    encoderOutput->buffer_num = encoderOutput->buffer_num_min;
  }



  // According to RaspiVid.c, we should set the framerate to 0 to make sure
  // connecting it to the input port causes it to get set accordingly
  //encoderOutput->format->es->video.frame_rate.num = cfg.framerate.first;
  //encoderOutput->format->es->video.frame_rate.den = cfg.framerate.second;
  encoderOutput->format->es->video.frame_rate.num = 0;
  encoderOutput->format->es->video.frame_rate.den = 1;

  status = mmal_port_format_commit(encoderOutput);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to commit encoder output format\n");
    return status;
  }


  MMAL_VIDEO_PROFILE_T profile = MMAL_VIDEO_PROFILE_DUMMY;
  switch (cfg.profile) {
    case H264EncoderConfig::Profile::BASELINE:
      profile = MMAL_VIDEO_PROFILE_H264_BASELINE;
      break;
    case H264EncoderConfig::Profile::MAIN:
      profile = MMAL_VIDEO_PROFILE_H264_MAIN;
      break;
    case H264EncoderConfig::Profile::HIGH:
      profile = MMAL_VIDEO_PROFILE_H264_HIGH;
      break;
  }

  if (profile == MMAL_VIDEO_PROFILE_DUMMY) {
    Logger::error(__func__, "Invalid video profile\n");
    return MMAL_EINVAL;
  }

  MMAL_VIDEO_LEVEL_T level = MMAL_VIDEO_LEVEL_DUMMY;
  switch (cfg.level) {
    case H264EncoderConfig::Level::H264_2:
      level = MMAL_VIDEO_LEVEL_H264_2;
      break;
    case H264EncoderConfig::Level::H264_21:
      level = MMAL_VIDEO_LEVEL_H264_21;
      break;
    case H264EncoderConfig::Level::H264_22:
      level = MMAL_VIDEO_LEVEL_H264_22;
      break;
    case H264EncoderConfig::Level::H264_3:
      level = MMAL_VIDEO_LEVEL_H264_3;
      break;
    case H264EncoderConfig::Level::H264_31:
      level = MMAL_VIDEO_LEVEL_H264_31;
      break;
    case H264EncoderConfig::Level::H264_32:
      level = MMAL_VIDEO_LEVEL_H264_32;
      break;
    case H264EncoderConfig::Level::H264_4:
      level = MMAL_VIDEO_LEVEL_H264_4;
      break;
    case H264EncoderConfig::Level::H264_41:
      level = MMAL_VIDEO_LEVEL_H264_41;
      break;
    case H264EncoderConfig::Level::H264_42:
      level = MMAL_VIDEO_LEVEL_H264_42;
      break;
    case H264EncoderConfig::Level::H264_5:
      level = MMAL_VIDEO_LEVEL_H264_5;
      break;
    case H264EncoderConfig::Level::H264_51:
      level = MMAL_VIDEO_LEVEL_H264_51;
      break;
  }

  if (level == MMAL_VIDEO_LEVEL_DUMMY) {
    Logger::error(__func__, "Invalid video profile level\n");
    return MMAL_EINVAL;
  }

  MMAL_PARAMETER_VIDEO_PROFILE_T profileParam = {
    .hdr = { MMAL_PARAMETER_PROFILE, sizeof(MMAL_PARAMETER_VIDEO_PROFILE_T) },
    .profile = {
      { profile, level },
    },
  };

  status = mmal_port_parameter_set(encoderOutput, &profileParam.hdr);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to apply H264 profile\n");
    return status;
  }


  status = mmal_port_parameter_set_boolean(encoderInput,
                                           MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT,
                                           cfg.immutableInputEnabled ? MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to configure encoder input as immutable\n");
    return status;
  }

  status = mmal_port_parameter_set_boolean(encoderOutput,
                                           MMAL_PARAMETER_VIDEO_ENCODE_INLINE_HEADER,
                                           cfg.inlineHeaderEnabled ? MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to configure encoder to use inline header\n");
    return status;
  }

  status = mmal_port_parameter_set_boolean(encoderOutput,
                                           MMAL_PARAMETER_VIDEO_ENCODE_SPS_TIMING,
                                           cfg.SPSTimingEnabled ? MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to configure encoder SPS timing\n");
    return status;
  }

  status = mmal_port_parameter_set_boolean(encoderOutput,
                                           MMAL_PARAMETER_VIDEO_ENCODE_INLINE_VECTORS,
                                           cfg.inlineVectorsEnabled ? MMAL_TRUE : MMAL_FALSE);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to configure encoder inline motion vectors\n");
    return status;
  }

  return MMAL_SUCCESS;
}

MMAL_STATUS_T Camera::setSensorMode(SensorMode mode) {
  return mmal_port_parameter_set_uint32(camera->control,
                                        MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG,
                                        mode);
}

static void printBufferFlags(const char* indent, const char* sep,
                             MMAL_BUFFER_HEADER_T* buffer) {
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

void Camera::controlCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
  Logger::debug(CAMERA_NS, "Camera::controlCallback called with cmd=0x%x\n", buffer->cmd);
}

void Camera::encoderCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer) {
  //Logger::debug(CAMERA_NS, "Camera::encoderCallback called\n");
  Camera* pCamera = reinterpret_cast<Camera*>(port->userdata);

  if (pCamera->isOutputFileOpen()) {
    mmal_buffer_header_mem_lock(buffer);
    pCamera->writeOutput(reinterpret_cast<char*>(buffer->data + buffer->offset),
                         buffer->length);
    mmal_buffer_header_mem_unlock(buffer);
    Logger::debug(__func__, "Wrote %u B to output file\n", buffer->length);
  } else {
    Logger::debug(__func__, "Got called but output file is closed\n");
  }
  printBufferFlags(nullptr, " ", buffer);
  puts("");

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


MMAL_STATUS_T Camera::getInputFormat(PortType t, MMAL_ES_FORMAT_T*& es) {
  MMAL_PORT_T* port = nullptr;
  switch (t) {
    case PortType::VIDEO:
      port = getCamera()->input[VIDEO_PORT];
      break;
    case PortType::PREVIEW:
      port = getCamera()->input[PREVIEW_PORT];
      break;
    case PortType::STILL:
      port = getCamera()->input[STILL_PORT];
      break;
    default:
      return MMAL_EINVAL;
  }
  es = port->format;
  //mmal_format_copy(&es, port->format);
  return MMAL_SUCCESS;
}

MMAL_STATUS_T Camera::setInputFormat(PortType t, MMAL_ES_FORMAT_T& es) {
  MMAL_PORT_T* port = nullptr;
  switch (t) {
    case PortType::VIDEO:
      port = getCamera()->input[VIDEO_PORT];
      break;
    case PortType::PREVIEW:
      port = getCamera()->input[PREVIEW_PORT];
      break; case PortType::STILL:
      port = getCamera()->input[STILL_PORT];
      break;
    default:
      return MMAL_EINVAL;
  }
  mmal_format_copy(port->format, &es);
  return mmal_port_format_commit(port);
}

MMAL_STATUS_T Camera::getOutputFormat(PortType t, MMAL_ES_FORMAT_T*& es) {
  MMAL_PORT_T* port = nullptr;
  switch (t) {
    case PortType::VIDEO:
      port = getCamera()->output[VIDEO_PORT];
      break;
    case PortType::PREVIEW:
      port = getCamera()->output[PREVIEW_PORT];
      break;
    case PortType::STILL:
      port = getCamera()->output[STILL_PORT];
      break;
    default:
      return MMAL_EINVAL;
  }
  es = port->format;
  //mmal_format_copy(&es, port->format);
  return MMAL_SUCCESS;
}

MMAL_STATUS_T Camera::setOutputFormat(PortType t, MMAL_ES_FORMAT_T& es) {
  MMAL_PORT_T* port = nullptr;
  switch (t) {
    case PortType::VIDEO:
      port = getCamera()->output[VIDEO_PORT];
      break;
    case PortType::PREVIEW:
      port = getCamera()->output[PREVIEW_PORT];
      break;
    case PortType::STILL:
      port = getCamera()->output[STILL_PORT];
      break;
    default:
      return MMAL_EINVAL;
  }
  mmal_format_copy(port->format, &es);
  return mmal_port_format_commit(port);
}

MMAL_STATUS_T Camera::setVideoFormat(MMAL_FOURCC_T encoding,
                                     MMAL_FOURCC_T encodingVariant,
                                     const MMAL_VIDEO_FORMAT_T& videoFormat) {
  MMAL_PORT_T* videoPort = getVideoOutputPort();
  MMAL_ES_FORMAT_T* format = videoPort->format;
  format->type = MMAL_ES_TYPE_VIDEO;
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
  format->type = MMAL_ES_TYPE_VIDEO;
  format->encoding = encoding;
  format->encoding_variant = encodingVariant;
  memcpy(&format->es->video, &videoFormat, sizeof(MMAL_ES_FORMAT_T));

  format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;

  return mmal_port_format_commit(port);
}

MMAL_STATUS_T Camera::setStillFormat(MMAL_FOURCC_T encoding,
                                     MMAL_FOURCC_T encodingVariant,
                                     const MMAL_VIDEO_FORMAT_T& videoFormat) {
  MMAL_PORT_T* port = getCamera()->output[STILL_PORT];
  MMAL_ES_FORMAT_T* format = port->format;
  format->encoding = encoding;
  format->encoding_variant = encodingVariant;
  memcpy(&format->es->video, &videoFormat, sizeof(MMAL_ES_FORMAT_T));

  format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;

  return mmal_port_format_commit(port);
}

MMAL_PORT_T* Camera::getVideoOutputPort() const {
  return getCamera()->output[VIDEO_PORT];
}

MMAL_PORT_T* Camera::getEncoderInputPort() const {
  return getEncoder()->input[0];
}

MMAL_PORT_T* Camera::getEncoderOutputPort() const {
  return getEncoder()->output[0];
}

MMAL_STATUS_T Camera::createBufferPools() {
#if 0
  {
    // video output
    MMAL_PORT_T* videoOutput = getVideoOutputPort();
    pool = mmal_port_pool_create(videoOutput,
                                 videoOutput->buffer_num,
                                 videoOutput->buffer_size);
    if (pool == nullptr) {
      Logger::error(__func__, "Failed to allocate video port buffer pool\n");
      return MMAL_ENOMEM;
    }
    Logger::info(__func__, "Created video output buffer pool with %d "
                 "buffers of size %d B\n",
                 videoOutput->buffer_num, videoOutput->buffer_size);
  }
#endif

#if 0
  {
    // output to encoder
    MMAL_PORT_T* splitterOutput = getSplitterEncodedOutputPort();
    splitterEncodedPool = mmal_port_pool_create(splitterOutput,
                                                splitterOutput->buffer_num,
                                                splitterOutput->buffer_size);
    if (splitterRawPool == nullptr) {
      Logger::error(__func__, "Failed to allocate splitter output[1] buffer pool\n");
      return MMAL_ENOMEM;
    }
    Logger::info(__func__, "Created splitter output[1] buffer pool with %d "
                 "buffers of size %d B\n",
                 splitterOutput->buffer_num, splitterOutput->buffer_size);
  }
#endif

  {
    MMAL_PORT_T* encoderOutput = getEncoderOutputPort();
    encoderPool = mmal_port_pool_create(encoderOutput, encoderOutput->buffer_num,
                                        encoderOutput->buffer_size);
    if (encoderPool == nullptr) {
      Logger::error(__func__, "Failed to allocate encoder output buffer pool\n");
      return MMAL_ENOMEM;
    }
    Logger::info(__func__, "Created encoder output buffer pool with %d "
                 "buffers of size %d B\n",
                 encoderOutput->buffer_num, encoderOutput->buffer_size);
  }

  return MMAL_SUCCESS;
}

#if 0
MMAL_POOL_T* Camera::getVideoBufferPool() {
  return pool;
}
#endif

#if 0
MMAL_POOL_T* Camera::getSplitterEncodedBufferPool() {
  return splitterEncodedPool;
}
#endif

MMAL_POOL_T* Camera::getEncoderBufferPool() {
  return encoderPool;
}

MMAL_STATUS_T Camera::enableCamera() {
  MMAL_STATUS_T status;
  status = mmal_component_enable(camera);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "Failed to enable camera component\n");
    return status;
  }

  return status;
}

MMAL_STATUS_T Camera::enableEncoder() {
  MMAL_STATUS_T status = mmal_component_enable(encoder);
  if (status != MMAL_SUCCESS) {
    Logger::error(CAMERA_NS, "Failed to enable encoder component\n");
    return status;
  }

  return status;
}

MMAL_STATUS_T Camera::enableCallbacks() {
  MMAL_STATUS_T status;

  {
    MMAL_PORT_T* encoderOutput = getEncoderOutputPort();
    encoderOutput->userdata = reinterpret_cast<MMAL_PORT_USERDATA_T*>(this);
    status = mmal_port_enable(encoderOutput, Camera::encoderCallback);
    if (status != MMAL_SUCCESS) {
      Logger::error(CAMERA_NS, "Failed to set up encoder output callback\n");
      return status;
    }
  }

  // Although this doesn't strictly set up any callbacks, go ahead and enable 
  // any ports that are connected to other ports here. These aren't allowed to
  // have callbacks (see mmal_port_enable), but it keeps things consistent.

  //status = mmal_port_enable(getVideoOutputPort(), nullptr);
  //if (status != MMAL_SUCCESS) {
  //  Logger::error(__func__, "Failed to enable video output port\n");
  //  return status;
  //}

  //status = mmal_port_enable(getSplitterInputPort(0), nullptr);
  //if (status != MMAL_SUCCESS) {
  //  Logger::error(__func__, "Failed to enable splitter input port\n");
  //  return status;
  //}

  //status = mmal_port_enable(getSplitterRawOutputPort(), nullptr);
  //if (status != MMAL_SUCCESS) {
  //  Logger::error(__func__, "Failed to enable splitter output[0] port\n");
  //  return status;
  //}

  //status = mmal_port_enable(getEncoderInputPort(), nullptr);
  //if (status != MMAL_SUCCESS) {
  //  Logger::error(__func__, "Failed to enable encoder input port\n");
  //  return status;
  //}

  // Apparently we don't have to enable these.

  return status;
}

MMAL_STATUS_T Camera::setUpConnections() {
  MMAL_PORT_T
    * videoOutput = getVideoOutputPort(),
    * encoderInput = getEncoderInputPort();
  MMAL_STATUS_T status;
  // video -> encoder
  status = mmal_connection_create(&videoEncoderConnection,
                                  videoOutput, encoderInput,
                                  MMAL_CONNECTION_FLAG_TUNNELLING |
                                  MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to connect video output to encoder input[0]\n");
    return status;
  }

  status = mmal_connection_enable(videoEncoderConnection);
  if (status != MMAL_SUCCESS) {
    Logger::error(__func__, "Failed to enable video -> encoder connection\n");
    return status;
  }

  return status;
}

MMAL_STATUS_T Camera::openOutputFile(std::string filename) {
  // If another file is already open, make sure it gets closed.
  if (encodedOutputOpen) {
    closeOutputFile();
  }

  encodedOutput.open(filename,
                     std::ios_base::out | std::ios_base::trunc);

  if (encodedOutput.fail()) {
    Logger::error(__func__, "Failed to open file %s for writing\n", filename.c_str());
    return MMAL_ENOENT;
  }
  encodedOutputOpen = true;

  return MMAL_SUCCESS;
}

MMAL_STATUS_T Camera::closeOutputFile() {
  if (encodedOutputOpen) {
    encodedOutputOpen = false;
    encodedOutput.close();
  }
 
  return MMAL_SUCCESS;
}

MMAL_STATUS_T Camera::writeOutput(char* data, size_t len) {
  if (!encodedOutputOpen) {
    Logger::error(__func__, "Output file is not open\n");
    return MMAL_ENOENT;
  }

  encodedOutput.write(data, len);

  return MMAL_SUCCESS;
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
  MMAL_STATUS_T status = mmal_port_parameter_get(camera->control, &param.hdr); enable = param.enable; u = param.u; v = param.v; return status; }

#if 0
void Camera::setFocus(MMAL_PARAM_FOCUS_T focus);
void Camera::getFocus(MMAL_PARAM_FOCUS_T& focus);

void Camera::setExposureComp(int32_t comp);
void Camera::setExposureComp(int32_t num, int32_t den);
int32_t Camera::getExposureComp();
void Camera::getExposureComp(int32_t& num, int32_t& den);
#endif

MMAL_STATUS_T Camera::enableCapture() {
  return mmal_port_parameter_set_boolean(getVideoOutputPort(),
                                         MMAL_PARAMETER_CAPTURE, MMAL_TRUE);
}

MMAL_STATUS_T Camera::disableCapture() {
  return mmal_port_parameter_set_boolean(getVideoOutputPort(),
                                         MMAL_PARAMETER_CAPTURE, MMAL_FALSE);
}

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
