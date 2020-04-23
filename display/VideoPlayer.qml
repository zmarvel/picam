
import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4


ColumnLayout {
    RowLayout {
        id: playerControlBar
        Layout.fillWidth: true

        Button {
            icon.name: "tango:/playback-start"
        }

        Button {
            icon.name: "tango:/playback-pause"
        }

        Button {
            icon.name: "tango:/playback-stop"
        }

        Slider {
        }
    }
}
