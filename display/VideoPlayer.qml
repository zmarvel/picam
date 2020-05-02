
import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtMultimedia 5.11

import com.zackmarvel.picam 1.0


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

    VideoSource {
      width: 640
      height: 480
      id: videoSource
    }

    VideoOutput {
      id: display
      source: videoSource
    }
}
