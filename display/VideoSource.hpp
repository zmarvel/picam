
#ifndef VIDEO_SOURCE_HPP
#define VIDEO_SOURCE_HPP

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoFrame>
#include <QVideoSurfaceFormat>

class VideoSource : public QObject {
  Q_OBJECT
  Q_PROPERTY(QAbstractVideoSurface *videoSurface
      READ videoSurface
      WRITE setVideoSurface)
  Q_PROPERTY(int width
      READ width
      WRITE setWidth)
  Q_PROPERTY(int height
      READ height
      WRITE setHeight)
  Q_PROPERTY(QVideoFrame::PixelFormat pixelFormat
      READ pixelFormat
      WRITE setPixelFormat)

  public:
    VideoSource();

    QAbstractVideoSurface *videoSurface() const;

    void setVideoSurface(QAbstractVideoSurface *psurface);

    int width() const;

    void setWidth(int newWidth);

    int height() const;

    void setHeight(int newHeight);

    QVideoFrame::PixelFormat pixelFormat() const;

    void setPixelFormat(QVideoFrame::PixelFormat newFormat);

  private:
    QAbstractVideoSurface *psurface_;
    QVideoSurfaceFormat format_;
};

#endif // VIDEO_SOURCE_HPP
