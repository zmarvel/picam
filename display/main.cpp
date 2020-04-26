

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QIcon>

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  QGuiApplication app{argc, argv};

  QIcon::setThemeName("tango");

  QQmlApplicationEngine engine{};
  engine.load(QUrl{"qrc:/main.qml"});

  return app.exec();
}
