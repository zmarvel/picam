

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QIcon>

#include "VideoSource.hpp"

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  QGuiApplication app{argc, argv};

  qmlRegisterType<VideoSource>("com.zackmarvel.picam", 1, 0, "VideoSource");

  QIcon::setThemeName("tango");

  QQmlApplicationEngine engine{};
  engine.load(QUrl{"qrc:/main.qml"});

  return app.exec();
}
