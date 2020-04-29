
import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4


ColumnLayout {
    RowLayout {
        id: playerControlBar
        Layout.fillWidth: true

        Button {
            icon.name: "media-playback-start"
        }

        Button {
            icon.name: "media-playback-pause"
        }

        Button {
            icon.name: "media-playback-stop"
        }

        Slider {
        }
    }
}
