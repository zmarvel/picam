
#include "VideoSource.hpp"

VideoSource::VideoSource() :
  psurface_{nullptr},
  format_{}
{ }

QAbstractVideoSurface *VideoSource::videoSurface() const {
  return psurface_;
}

void VideoSource::setVideoSurface(QAbstractVideoSurface *psurface) {
  if (psurface_ != nullptr && psurface_ != psurface && psurface_->isActive()) {
    psurface_->stop();
  }

  psurface_ = psurface;

  if (psurface_ != nullptr && format_.isValid()) {
    format_ = psurface_->nearestFormat(format_);
    psurface_->start(format_);
  }
}

int VideoSource::width() const {
  return format_.frameSize().rwidth();
}

void VideoSource::setWidth(int newWidth) {
  auto currSize = format_.frameSize();
  currSize.setWidth(newWidth);
  format_.setFrameSize(currSize);
}

int VideoSource::height() const {
  return format_.frameSize().rheight();
}

void VideoSource::setHeight(int newHeight) {
  auto currSize = format_.frameSize();
  currSize.setHeight(newHeight);
  format_.setFrameSize(currSize);
}

QVideoFrame::PixelFormat VideoSource::pixelFormat() const {
  return format_.pixelFormat();
}

void VideoSource::setPixelFormat(QVideoFrame::PixelFormat newFormat) {
  auto currSize = format_.frameSize();
  format_ = QVideoSurfaceFormat{currSize, newFormat};
}
