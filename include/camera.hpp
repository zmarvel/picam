
#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <fstream>

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
  SPLITTER,
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
    explicit Camera(int cameraNum);
    ~Camera();

    // TODO. Consumer-provided callbacks would probably be a better abstraction.
    // Maybe it doesn't make a lot of sense for this class to manage files.

    /**
     * Static method that the constructor will set up as a callback for control
     * messages from the underlying driver.
     */
    static void controlCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer);
    static void splitterRawCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer);
    static void encoderCallback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer);

    MMAL_COMPONENT_T* getCamera() const {
      return camera;
    }

    MMAL_COMPONENT_T* getSplitter() const {
      return splitter;
    }

    MMAL_COMPONENT_T* getEncoder() const {
      return encoder;
    }

    MMAL_PORT_T* getVideoOutputPort() const;
    MMAL_PORT_T* getSplitterInputPort() const;
    MMAL_PORT_T* getSplitterRawOutputPort() const;
    MMAL_PORT_T* getSplitterEncodedOutputPort() const;
    MMAL_PORT_T* getEncoderInputPort() const;
    MMAL_PORT_T* getEncoderOutputPort() const;

    /**
     * Create buffer pools for the video output, the two splitter outputs, and
     * the encoder output.
     */
    MMAL_STATUS_T createBufferPools();
    MMAL_POOL_T* getVideoBufferPool();
    MMAL_POOL_T* getSplitterEncodedBufferPool();
    MMAL_POOL_T* getSplitterRawBufferPool();
    MMAL_POOL_T* getEncoderBufferPool();

    /**
     * Enable the camera, splitter, and encoder components.
     */
    MMAL_STATUS_T enableCamera();
    MMAL_STATUS_T enableSplitter();
    MMAL_STATUS_T enableEncoder();

    /**
     * Enable the callbacks for the second splitter output (the first splitter
     * output will be connected to the encoder input) and the encoder output.
     */
    MMAL_STATUS_T enableCallbacks();

    /**
     * Set up and enable connections. By default,
     * - camera video output -> splitter input
     * - splitter output[1] -> encoder input
     */
    MMAL_STATUS_T setUpConnections();

    /**
     * Set up the output file where encoded video will be written. If another
     * file is already open, it will be closed.
     */
    MMAL_STATUS_T openOutputFile(std::string filename);

    /**
     * Close the output file. If no file is open, this function will return a
     * success status, but it will have no effect. On destruction of a Camera
     * object, an open output file will be closed automatically.
     */
    MMAL_STATUS_T closeOutputFile();

    /**
     * Does the Camera class think an output file is already open?
     */
    bool isOutputFileOpen() const {
      return encodedOutputOpen;
    }

    /**
     * Write len bytes from data to the output file. If no file is open, this
     * function will fail.
     */
    MMAL_STATUS_T writeOutput(char* data, size_t len);

    /**
     * Allocate resources for the camera, and set it up.
     */
    MMAL_STATUS_T open(SensorMode mode);

    MMAL_STATUS_T configurePreview();
    MMAL_STATUS_T configureSplitter();
    MMAL_STATUS_T configureEncoder(H264EncoderConfig& cfg);

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
    void setShutterSpeed(uint32_t speed);
    uint32_t getShutterSpeed();

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
    

  private:
    int cameraNum;

    // Components
    MMAL_COMPONENT_T* camera;
    MMAL_COMPONENT_T* splitter;
    MMAL_COMPONENT_T* encoder;
    MMAL_COMPONENT_T* preview;

    // Buffer pools
    MMAL_POOL_T* splitterRawPool;
    MMAL_POOL_T* encoderPool;

    // Connections
    MMAL_CONNECTION_T* videoSplitterConnection;
    MMAL_CONNECTION_T* splitterEncoderConnection;

    std::ofstream encodedOutput;
    bool encodedOutputOpen;

};

#endif // CAMERA_HPP
