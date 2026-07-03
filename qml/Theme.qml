pragma Singleton
import QtQuick

QtObject {
    signal requestShowDownloads()

    // Light theme for PNG Studio
    readonly property color background:  "#f5f5f7"
    readonly property color surface:     "#ffffff"
    readonly property color surfaceAlt:  "#e5e5ea"
    readonly property color accent:      "#007aff"
    readonly property color accentLight: "#47a1ff"
    readonly property color textPrimary: "#1d1d1f"
    readonly property color textSecondary: "#86868b"
    readonly property color danger:      "#ff3b30"
    readonly property color success:     "#34c759"
    readonly property color warning:     "#ff9500"

    readonly property int radiusSmall:  8
    readonly property int radiusMedium: 12
    readonly property int radiusLarge:  16

    readonly property int paddingSmall:  8
    readonly property int paddingMedium: 12
    readonly property int paddingLarge:  16
    readonly property int paddingXL:     24

    readonly property int fontSmall:  12
    readonly property int fontMedium: 14
    readonly property int fontLarge:  18
    readonly property int fontTitle:  24

    readonly property int sidebarWidth: 220
    readonly property int iconSize:    20
}
