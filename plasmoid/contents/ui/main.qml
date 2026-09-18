import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root
    toolTipMainText: i18n("WaySaver")
    toolTipSubText: i18n("Click to open screensaver controls")
    Plasmoid.icon: "preferences-desktop-screensaver"
    preferredRepresentation: compactRepresentation

    compactRepresentation: MouseArea {
        Kirigami.Icon {
            anchors.fill: parent
            source: "preferences-desktop-screensaver"
            active: parent.containsMouse
        }
        onClicked: root.expanded = !root.expanded
    }

    fullRepresentation: ColumnLayout {
        implicitWidth: Kirigami.Units.gridUnit * 16
        implicitHeight: Kirigami.Units.gridUnit * 9
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Heading {
            text: i18n("WaySaver")
            level: 2
            Layout.margins: Kirigami.Units.smallSpacing
        }
        Controls.Button {
            text: i18n("Start Screensaver")
            icon.name: "media-playback-start"
            Layout.fillWidth: true
            onClicked: Qt.openUrlExternally("waysaver://activate")
        }
        Controls.Button {
            text: i18n("Open Settings")
            icon.name: "configure"
            Layout.fillWidth: true
            onClicked: Qt.openUrlExternally("waysaver://settings")
        }
        Item { Layout.fillHeight: true }
        Controls.Label {
            text: i18n("Media playback and Wayland idle inhibition are respected.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            opacity: 0.7
        }
    }
}

