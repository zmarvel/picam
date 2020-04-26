
import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4


ColumnLayout {
    RowLayout {
        id: playerControlBar
        Layout.fillWidth: true

        Button {
            icon.name: "qrc:/tango/icons/media-playback-start.svg"
        }

        Button {
            icon.name: "qrc:/tango/icons/media-playback-pause.svg"
        }

        Button {
            icon.name: "qrc:/tango/icons/media-playback-stop.svg"
        }

        Slider {
        }
    }
}
