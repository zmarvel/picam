#ifndef CAMERA_CONFIG_HPP
#define CAMERA_CONFIG_HPP

/**
 * https://picamera.readthedocs.io/en/release-1.13/api_camera.html#picamera
 */
struct CameraConfig {
  /**
   * Image rotation: 0, 90, 180, or 270 degrees. Can be changed while capture
   * is ongoing.
   *
   * MMAL_PARAMETER_ROTATION_T.
   */
  MMAL_PARAMETER_INT32_T rotation;

  /**
   * Disables EXIF information (doesn't apply unless a JPEG encoding is used).
   *
   * MMAL_PARAMETER_EXIF_DISABLE.
   */
  MMAL_PARAMETER_BOOLEAN_T disableEXIF;

  /**
   * EXIF data. Doesn't apply unless a JPEG encoding is used.
   *
   * MMAL_PARAMETER_EXIF.
   */
  MMAL_PARAMETER_EXIF_T EXIF;

  /**
   * Auto-white-balance mode.
   *
   * MMAL_PARAMETER_AWB_MODE
   */
  MMAL_PARAM_AWBMODE_T AWBMode;

  /**
   * Image effect to apply.
   *
   * @see imageEffectParameters.
   *
   * MMAL_PARAMETER_IMAGE_EFFECT.
   */
  MMAL_PARAMETER_IMAGEFX_T imageEffect;

  /**
   * Color effect is a tuple of (u, v). Both values are between 0 and 255. Can
   * be changed while capture is ongoing.
   *
   * MMAL_PARAMETER_COLOUR_EFFECT.
   */
  MMAL_PARAMETER_COLOURFX_T colorEffect;

  /**
   * Flicker avoidance setting.
   *
   * MMAL_PARAMETER_FLICKER_AVOID.
   */
  MMAL_PARAMETER_FLICKERAVOID_T flickerAvoidance;

  /**
   * Flash setting, e.g. to correct redeye. Can be changed while capture is
   * ongoing.
   *
   * MMAL_PARAMETER_FLASH.
   */
  MMAL_PARAMETER_FLASH_T flash;

  /**
   * Redeye mode.
   *
   * MMAL_PARAMETER_REDEYE.
   */
  MMAL_PARAMETER_REDEYE_T redeye;

  /**
   * Focus mode.
   *
   * @note Read MMAL_PARAMETER_FOCUS_STATUS to determine if the camera thinks
   * the image is in focus.
   *
   * MMAL_PARAMETER_FOCUS.
   */
  MMAL_PARAMETER_FOCUS_T focus;

  /**
   * Exposure compensation. Between -25 and 25. A larger value means a brighter
   * image. Increments represent 1/6th of a stop. Can be changed while capture
   * is ongoing.
   *
   * MMAL_PARAMETER_EXPOSURE_COMP.
   */
  MMAL_PARAMETER_INT32_T exposureCompensation;

  /**
   * Two Q16.16 fixed point values representing scale in two dimensions. Can
   * be changed while capture is ongoing.
   *
   * MMAL_PARAMETER_ZOOM.
   */
  MMAL_PARAMETER_SCALEFACTOR_T zoom;

  /**
   * Mirror mode.
   *
   * MMAL_PARAMETER_MIRROR.
   */
  MMAL_PARAMETER_MIRROR_T mirror;

  /**
   * Camera number.
   *
   * MMAL_PARAMETER_CAMERA_NUM.
   */
  MMAL_PARAMETER_UINT32_T cameraNum;

  /**
   * Is a capture ongoing? Initialized to false.
   *
   * @note Read MMAL_PARAMETER_CAPTURE_STATUS to determine what state the camera
   * is in.
   *
   * MMAL_PARAMETER_CAPTURE.
   */
  MMAL_PARAMETER_BOOLEAN_T capture;

  /**
   * Exposure mode.
   * MMAL_PARAMETER_EXPOSURE_MODE.
   */
  MMAL_PARAMETER_EXPOSUREMODE_T exposureMode;

  /**
   * Exposure metering mode. Can be set while capture is ongoing.
   *
   * MMAL_PARAMETER_EXP_METERING_MODE.
   */
  MMAL_PARAMETER_EXPOSUREMETERINGMODE_T exposureMeteringMode;

  /**
   * Camera config (see struct definition for fields).
   *
   * MMAL_PARAMETER_CAMERA_CONFIG.
   */
  MMAL_PARAMETER_CAMERA_CONFIG_T cameraConfig;

  /**
   * Face detection and tracking.
   *
   * @see MMAL_PARAMETER_FACE_TRACK_RESULTS.
   *
   * MMAL_PARAMETER_FACE_TRACK.
   */
  MMAL_PARAMETER_FACE_TRACK_T faceTrack;

  /**
   * Apparently, draw boxes around detected faces and the area of focus.
   *
   * MMAL_PARAMETER_DRAW_BOX_FACES_AND_FOCUS.
   */
  MMAL_PARAMETER_BOOLEAN_T drawBoxFacesAndFocus;

  /**
   * JPEG quality (?) factor.
   *
   * MMAL_PARAMETER_JPEG_Q_FACTOR.
   */
  MMAL_PARAMETER_UINT32_T JPEGQFactor;

  /**
   * Frame rate.
   *
   * MMAL_PARAMETER_FRAME_RATE.
   */
  MMAL_PARAMETER_FRAME_RATE_T frameRate;

  /**
   * Determines whether the timestamp is included with the frame, and whether
   * the time base is the beginning of the capture.
   *
   * MMAL_PARAMETER_USE_STC.
   */
  MMAL_PARAMETER_CAMERA_STC_MODE_T cameraSTCMode;

  /**
   * Enable video stabilization.
   *
   * MMAL_PARAMETER_VIDEO_STABILISATION.
   */
  MMAL_PARAMETER_BOOLEAN_T videoStabilisation;

  /**
   * Enable raw capture.
   *
   * MMAL_PARAMETER_ENABLE_RAW_CAPTURE.
   */
  MMAL_PARAMETER_BOOLEAN_T enableRawCapture;

  // MMAL_PARAMETER_DPF_FILE ??
  // MMAL_PARAMETER_ENABLE_DPF_FILE ??
  // MMAL_PARAMETER_DPF_FAIL_IS_FATAL ??
  
  /**
   * Determines how quickly the preview is available after an image capture is
   * completed.
   *
   * MMAL_PARAMETER_CAPTURE_MODE.
   */
  MMAL_PARAMETER_CAPTUREMODE_T captureMode;

  /**
   * Hints for which regions should be prioritized for focus and face-tracking.
   *
   * MMAL_PARAMETER_FOCUS_REGIONS
   */
  MMAL_PARAMETER_FOCUS_REGIONS_T focusRegions;

  /**
   * Crop input image to a rectangle.
   *
   * MMAL_PARAMETER_INPUT_CROP.
   */
  MMAL_PARAMETER_INPUT_CROP_T inputCrop;

  /**
   * Hint to the camera what type of flash will be used (for flash correction).
   *
   * MMAL_PARAMETER_FLASH_SELECT.
   */
  MMAL_PARAMETER_FLASH_SELECT_T flashSelect;

  /**
   * Field of view, consisting of a horizontal and vertical component.
   *
   * MMAL_PARAMETER_FIELD_OF_VIEW.
   */
  MMAL_PARAMETER_FIELD_OF_VIEW_T fieldOfView;

  /**
   * Enable high dynamic range.
   *
   * MMAL_PARAMETER_HIGH_DYNAMIC_RANGE.
   */
  MMAL_PARAMETER_BOOLEAN_T highDynamicRange;

  /**
   * Strength of dynamic range compression.
   *
   * MMAL_PARAMETER_DYNAMIC_RANGE_COMPRESSION.
   */
  MMAL_PARAMETER_DRC_T dynamicRangeCompression;

  /**
   * Determines whether and which algorithms are enabled (e.g. face-tracking,
   * denoising, etc.).
   *
   * MMAL_PARAMETER_ALGORITHM_CONTROL.
   */
  MMAL_PARAMETER_ALGORITHM_CONTROL_T algorithmControl;

  /**
   * Sharpness. Between -100 and 100. Can be set while capture is ongoing.
   *
   * MMAL_PARAMETER_SHARPNESS.
   */
  MMAL_PARAMETER_RATIONAL_T sharpness;

  /**
   * Brightness. Between -100 and 100. Can be set while capture is ongoing.
   *
   * MMAL_PARAMETER_BRIGHTNESS.
   */
  MMAL_PARAMETER_RATIONAL_T brightness;

  /**
   * Saturation. Between -100 and 100. Can be set while capture is ongoing.
   *
   * MMAL_PARAMETER_SATURATION.
   */
  MMAL_PARAMETER_RATIONAL_T saturation;

  /**
   * Image exposure. Valid range is 0 (auto) to 1600.
   *
   * MMAL_PARAMETER_ISO.
   */
  MMAL_PARAMETER_UINT32_T ISO;

  /**
   * Enables the antishake feature.
   *
   * MMAL_PARAMETER_ANTISHAKE.
   */
  MMAL_PARAMETER_BOOLEAN_T anitshake;

  /**
   * Image effect parameters.
   *
   * @see imageEffect.
   *
   * MMAL_PARAMETER_IMAGE_EFFECT_PARAMETERS.
   */
  MMAL_PARAMETER_IMAGEFX_PARAMETERS_T imageEffectParameters;

  /**
   * Enables burst capture mode.
   *
   * MMAL_PARAMETER_CAMERA_BURST_CAPTURE.
   */
  MMAL_PARAMETER_BOOLEAN_T cameraBurstCapture;

  /**
   * Set (?) the minimum ISO. Does this only apply when exposure is not manual,
   * or is it just a getter?
   *
   * MMAL_PARAMETER_CAMERA_MIN_ISO.
   */
  MMAL_PARAMETER_UINT32_T minISO;

  /**
   * Optimize for still capture, video capture, or compromise between the two.
   *
   * MMAL_PARAMETER_CAMERA_USE_CASE.
   */
  MMAL_PARAMETER_CAMERA_USE_CASE_T cameraUseCase;

  /**
   * Enables statistics capture.
   *
   * MMAL_PARAMETER_CAPTURE_STATS_PASS.
   */
  MMAL_PARAMETER_BOOLEAN_T captureStatsPass;

  // MMAL_PARAMETER_CAMERA_CUSTOM_SENSOR_CONFIG ??
  // MMAL_PARAMETER_ENABLE_REGISTER_FILE ??
  // MMAL_PARAMETER_REGISTER_FAIL_IS_FATAL ??
  // MMAL_PARAMETER_CONFIGFILE_REGISTERS ??
  // MMAL_PARAMETER_CONFIGFILE_CHUNK_REGISTERS ??

  /**
   * Enable attaching log to JPEG output.
   *
   * MMAL_PARAMETER_JPEG_ATTACH_LOG.
   */
  MMAL_PARAMETER_BOOLEAN_T JPEGAttachLog;

  /**
   * Use preview images to permit concurrent capturing, and reduce shutter lag
   * on the camera.
   *
   * MMAL_PARAMETER_ZERO_SHUTTER_LAG.
   */
  MMAL_PARAMETER_ZEROSHUTTERLAG_T zeroShutterLag;

  /**
   * Min and max FPS.
   *
   * MMAL_PARAMETER_FPS_RANGE.
   */
  MMAL_PARAMETER_FPS_RANGE_T FPSRange;

  /**
   * Capture exposure compensation.
   *
   * MMAL_PARAMETER_CAPTURE_EXPOSURE_COMP.
   */
  MMAL_PARAMETER_INT32_T captureExposureComp;

  /**
   * Disable software sharpness.
   *
   * MMAL_PARAMETER_SW_SHARPEN_DISABLE.
   */
  MMAL_PARAMETER_BOOLEAN_T swSharpenDisable;

  // MMAL_PARAMETER_FLASH_REQUIRED ??

  /**
   * Disable software saturation.
   *
   * MMAL_PARAMETER_SW_SATURATION_DISABLE.
   */
  MMAL_PARAMETER_BOOLEAN_T swSaturationDisable;

  /**
   * Shutter speed in microseconds. 0 indicates "auto." Can be set while capture
   * is ongoing.
   *
   * MMAL_PARAMETER_SHUTTER_SPEED.
   */
  MMAL_PARAMETER_UINT32_T shutterSpeed;

  /**
   * Auto-white balance gains. Should be between 0.0 and 8.0. Typical values
   * are between 0.9 and 1.9. This corresponds to an AWB mode of "off." Can be
   * set while capture is ongoing.
   *
   * MMAL_PARAMETER_CUSTOM_AWB_GAINS.
   */
  MMAL_PARAMETER_AWB_GAINS_T custonAWBGains;

  // MMAL_PARAMETER_CAMERA_SETTINGS
  // Leaving this out because its properties are exposure, analog and digital
  // gain, AWB gains, and focus position. Focus position is the only property
  // that isn't already a member of this struct (?), and I'm not sure what it
  // does.
  
  /**
   * Configure the privacy indicator.
   *
   * MMAL_PARAMETER_PRIVACY_INDICATOR.
   */
  MMAL_PARAMETER_PRIVACY_INDICATOR_T privacyIndicator;

  /**
   * Enable video denoise.
   *
   * MMAL_PARAMETER_VIDEO_DENOISE.
   */
  MMAL_PARAMETER_BOOLEAN_T videoDenoise;

  /**
   * Enable still denoise.
   *
   * MMAL_PARAMETER_STILLS_DENOISE.
   */
  MMAL_PARAMETER_BOOLEAN_T stillsDenoise;

  /**
   * Set annotation properties of camera output, including custom text.
   *
   * MMAL_PARAMETER_ANNOTATE.
   */
  MMAL_PARAMETER_CAMERA_ANNOTATE_T annotate;

  /**
   * Enable and configure stereoscopic mode.
   *
   * MMAL_PARAMETER_STEREOSCOPIC_MODE.
   */
  MMAL_PARAMETER_STEREOSCOPIC_MODE_T stereoscopicMode;

  // MMAL_PARAMETER_CAMERA_INTERFACE ??
  // MMAL_PARAMETER_CAMERA_CLOCKING_MODE ??
  // MMAL_PARAMETER_CAMERA_RX_CONFIG ??
  // MMAL_PARAMETER_CAMERA_RX_TIMING ??
  // MMAL_PARAMETER_DPF_CONFIG ??
  // MMAL_PARAMETER_JPEG_RESTART_INTERVAL ??
  // MMAL_PARAMETER_CAMERA_ISP_BLOCK_OVERRIDE ??
  // MMAL_PARAMETER_LENS_SHADING_OVERRIDE ??
  //

  /**
   * Set the black level (?).
   * 
   * MMAL_PARAMETER_BLACK_LEVEL.
   */
  MMAL_PARAMETER_UINT32_T blackLevel;

  /**
   * MMAL_PARAMETER_RESIZE_PARAMS.
   */
  MMAL_PARAMETER_RESIZE_T resizeParams;

  /**
   * MMAL_PARAMETER_CROP.
   */
  MMAL_PARAMETER_CROP_T crop;

  /**
   * MMAL_PARAMETER_OUTPUT_SHIFT
   */
  MMAL_PARAMETER_INT32_T outputShift;

  // MMAL_PARAMETER_CCM_SHIFT ??
  // MMAL_PARAMETER_CUSTOM_CCM ??

  /**
   * MMAL_PARAMETER_ANALOG_GAIN
   */
  MMAL_PARAMETER_RATIONAL_T analogGain;

  /**
   * MMAL_PARAMETER_DIGITAL_GAIN
   */
  MMAL_PARAMETER_RATIONAL_T digitalGain;
};

#endif // CAMERA_CONFIG_HPP
