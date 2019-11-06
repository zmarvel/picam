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

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <ostream>
#include <functional>

#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_parameters_camera.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_connection.h>

#include "encoder_config.hpp"

enum class PortType {
  PREVIEW,
  VIDEO,
  STILL,
};


// Assuming camera module v2
// TODO
// See https://picamera.readthedocs.io/en/release-1.13/fov.html
typedef enum {
  SM_INVALID = 0,
  SM_1920x1080 = 1,
  SM_3280x2464_0 = 2,
  SM_3280x2464_1 = 3,
  SM_1640x1232 = 4,
  SM_1640x922 = 5,
  SM_1282x720 = 6,
  SM_640x480 = 7,
  NUM_SENSOR_MODES,
} SensorMode;

enum class CaptureMode {
  VIDEO,
  STILL,
};

extern const unsigned int SENSOR_MODE_WIDTH[NUM_SENSOR_MODES];
extern const unsigned int SENSOR_MODE_HEIGHT[NUM_SENSOR_MODES];



const int CAMERA_MAX_STILL_WIDTH = 3280;
const int CAMERA_MAX_STILL_HEIGHT = 2464;



/**
 * Represents a Pi Camera (v2 for now).
 *
 * @note Getters and setters are named after corresponding MMAL_PARAMETERs.
 */
class Camera {

  public:
    typedef std::function<size_t(std::string& data)> encoderCallbackType;

    explicit Camera(int cameraNum);
    ~Camera();

    // TODO. Consumer-provided callbacks would probably be a better abstraction.
    // Maybe it doesn't make a lot of sense for this class to manage files.

    MMAL_COMPONENT_T* getCamera() const {
      return mCamera;
    }

    MMAL_COMPONENT_T* getEncoder() const {
      return mEncoder;
    }

    MMAL_PORT_T* captureOutputPort() const;
    MMAL_PORT_T* encoderInputPort() const;
    MMAL_PORT_T* encoderOutputPort() const;

    /**
     * Create buffer pools for the video output, the two splitter outputs, and
     * the encoder output.
     */
    MMAL_STATUS_T createBufferPools();
    MMAL_POOL_T* getVideoBufferPool();
    MMAL_POOL_T* getEncoderBufferPool();

    /**
     * Enable the camera, splitter, and encoder components.
     */
    MMAL_STATUS_T enableCamera();
    MMAL_STATUS_T enableEncoder();

    /**
     * Enable the callbacks for the second splitter output (the first splitter
     * output will be connected to the encoder input) and the encoder output.
     */
    MMAL_STATUS_T enableCallbacks(encoderCallbackType encoderCallback);

    MMAL_STATUS_T disableCallbacks();

    /**
     * Set up and enable connections. By default,
     * - camera video output -> splitter input
     * - splitter output[1] -> encoder input
     */
    MMAL_STATUS_T setUpConnections();

    /**
     * Allocate resources for the camera, and set it up.
     */
    MMAL_STATUS_T open(SensorMode mode, CaptureMode captureMode);

    MMAL_STATUS_T configurePreview();

    MMAL_STATUS_T setCaptureMode(CaptureMode mode);
    MMAL_STATUS_T setSensorMode(SensorMode mode);

    MMAL_STATUS_T setVideoFormat(MMAL_FOURCC_T encoding,
                                 MMAL_FOURCC_T encodingVariant,
                                 const MMAL_VIDEO_FORMAT_T& es);
    MMAL_STATUS_T setPreviewFormat(MMAL_FOURCC_T encoding,
                                   MMAL_FOURCC_T encodingVariant,
                                   const MMAL_VIDEO_FORMAT_T& es);
    MMAL_STATUS_T setStillFormat(MMAL_FOURCC_T encoding,
                                 MMAL_FOURCC_T encodingVariant,
                                 const MMAL_VIDEO_FORMAT_T& es);

    MMAL_STATUS_T getInputFormat(PortType t, MMAL_ES_FORMAT_T*& es);
    MMAL_STATUS_T setInputFormat(PortType t, MMAL_ES_FORMAT_T& es);
    MMAL_STATUS_T getOutputFormat(PortType t, MMAL_ES_FORMAT_T*& es);
    MMAL_STATUS_T setOutputFormat(PortType t, MMAL_ES_FORMAT_T& es);

    /**
     * Set the auto-white balance mode.
     */
    MMAL_STATUS_T setAWBMode(const MMAL_PARAM_AWBMODE_T mode);
    MMAL_STATUS_T getAWBMode(MMAL_PARAM_AWBMODE_T& mode);

    /**
     * Set or disable/enable the color effect. This is specified as a (u, v) pair
     * where u and v are between 0 and 255.
     */
    MMAL_STATUS_T setColorEffect(bool enabled, uint32_t u, uint32_t v);
    MMAL_STATUS_T getColorEffect(bool& enabled, uint32_t& u, uint32_t& v);

    /**
     * Set the focus mode.
     */
    MMAL_STATUS_T setFocus(MMAL_PARAM_FOCUS_T focus);
    MMAL_STATUS_T getFocus(MMAL_PARAM_FOCUS_T& focus);

    /**
     * Set exposure compensation. Integer and rational number variants are
     * available.
     */
    MMAL_STATUS_T setExposureComp(int32_t comp);
    MMAL_STATUS_T setExposureComp(int32_t num, int32_t den);
    MMAL_STATUS_T getExposureComp(int32_t& comp);
    MMAL_STATUS_T getExposureComp(int32_t& num, int32_t& den);

    MMAL_STATUS_T enableCapture();
    MMAL_STATUS_T disableCapture();

    /**
     * Set the exposure mode.
     */
    MMAL_STATUS_T setExposureMode(MMAL_PARAM_EXPOSUREMODE_T mode);
    MMAL_STATUS_T getExposureMode(MMAL_PARAM_EXPOSUREMODE_T& mode);

    /**
     * Set the exposure metering mode.
     */
    MMAL_STATUS_T setExpMeteringMode(MMAL_PARAM_EXPOSUREMETERINGMODE_T mode);
    MMAL_STATUS_T getExpMeteringMode(MMAL_PARAM_EXPOSUREMETERINGMODE_T& mode);

    /**
     * Get the focus status from the camera.
     */
    MMAL_PARAM_FOCUS_STATUS_T getFocusStatus();

    /**
     * Set the camera config.
     *
     * @see MMAL_PARAMETER_CAMERA_CONFIG_T.
     */
    MMAL_STATUS_T setCameraConfig(const MMAL_PARAMETER_CAMERA_CONFIG_T& config);

    /**
     * Determine whether a capture is onging, hasn't been started, or has ended.
     */
    MMAL_STATUS_T getCaptureStatus(MMAL_PARAM_CAPTURE_STATUS_T& status);

    /**
     * Set the frame rate (as a rational number).
     */
    MMAL_STATUS_T setFrameRate(int32_t num, int32_t den);
    MMAL_STATUS_T getFrameRate(int32_t& num, int32_t& den);

    /**
     * Set the STC mode, one of
     * - Don't include STC in frames
     * - Use clock as STC
     * - STC starts at the beginning of the capture
     */
    void setUseSTC(MMAL_CAMERA_STC_MODE_T mode);
    MMAL_CAMERA_STC_MODE_T getUseSTC();

    /**
     * Get the sensor information.
     */
    void getSensorInformation(MMAL_PARAMETER_SENSOR_INFORMATION_T& info);

    /**
     * Set the sharpness (between -100 and 100).
     */
    MMAL_STATUS_T setSharpness(int32_t num, int32_t den);

    /**
     * Set the contrast (between -100 and 100).
     */
    MMAL_STATUS_T setContrast(int32_t num, int32_t den);

    /**
     * Set the brightness (between -100 and 100).
     */
    MMAL_STATUS_T setBrightness(int32_t num, int32_t den);

    /**
     * Set the saturation (between -100 and 100).
     */
    MMAL_STATUS_T setSaturation(int32_t num, int32_t den);

    /**
     * Set the ISO. Range is 0 to 1600, where 0 indicates "auto."
     */
    MMAL_STATUS_T setISO(uint32_t iso);

    MMAL_STATUS_T setCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_T useCase);
    MMAL_STATUS_T getCameraUseCase(MMAL_PARAM_CAMERA_USE_CASE_T& useCase);

    /**
     * Set the shutter speed in microseconds.
     */
    MMAL_STATUS_T setShutterSpeed(uint32_t speed);
    MMAL_STATUS_T getShutterSpeed(uint32_t& speed);

    /**
     * Configure privacy indicator policy.
     */
    void setPrivacyIndicator(MMAL_PARAM_PRIVACY_INDICATOR_T mode);
    MMAL_PARAM_PRIVACY_INDICATOR_T getPrivacyIndicator();

    /**
     * Set analog gain. Valid range is 0.0 to 8.0, with typical values between
     * 0.9 and 1.9
     */
    void setAnalogGain(int32_t num, int32_t den);
    void getAnalogGain(int32_t& num, int32_t& den);

    /**
     * Set digital gain. Valid range is 0.0 to 8.0, with typical values between
     * 0.9 and 1.9
     */
    void setDigitalGain(int32_t num, int32_t den);
    void getDigitalGain(int32_t& num, int32_t& den);
    
    encoderCallbackType encoderCallback();

  private:
    /**
     * Static method that the constructor will set up as a callback for control
     * messages from the underlying driver.
     */
    static void controlCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer);
    static void encoderCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer);

    int mCameraNum;
    SensorMode mSensorMode;
		CaptureMode mCaptureMode;

    // Components
    MMAL_COMPONENT_T* mCamera;
    MMAL_COMPONENT_T* mEncoder;
    MMAL_COMPONENT_T* mPreview;

    // Buffer pools
    MMAL_POOL_T* mEncoderPool;

    // Connections
    MMAL_CONNECTION_T* mVideoEncoderConnection;
    MMAL_CONNECTION_T* mPreviewNullConnection;

    encoderCallbackType mEncoderCallback;
};

#endif // CAMERA_HPP
