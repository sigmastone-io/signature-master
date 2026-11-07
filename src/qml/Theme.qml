pragma Singleton
import QtQuick

// The family's zinc greys and indigo accent, in the shade each theme needs. State colours mean
// one thing each (see doc/01. gui-design.md) and are never used for decoration.
QtObject {
    readonly property bool dark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    readonly property color canvas: dark ? "#09090b" : "#fafafa"
    readonly property color surface: dark ? "#18181b" : "#ffffff"
    readonly property color surfaceAlt: dark ? "#0f0f11" : "#ffffff"
    readonly property color rail: dark ? "#18181b" : "#f4f4f5"
    readonly property color border: dark ? "#27272a" : "#e4e4e7"
    readonly property color hairline: dark ? "#18181b" : "#f4f4f5"
    readonly property color text: dark ? "#fafafa" : "#18181b"
    readonly property color muted: dark ? "#a1a1aa" : "#52525b"
    readonly property color subtle: dark ? "#71717a" : "#71717a"

    readonly property color accent: dark ? "#818cf8" : "#4f46e5"
    readonly property color accentSoft: dark ? "#312e81" : "#e0e7ff"
    readonly property color accentText: dark ? "#e0e7ff" : "#3730a3"

    readonly property color pending: dark ? "#fbbf24" : "#d97706"
    readonly property color signing: dark ? "#818cf8" : "#4f46e5"
    readonly property color signed: dark ? "#4ade80" : "#15803d"
    readonly property color refused: dark ? "#f87171" : "#b91c1c"
    readonly property color expired: dark ? "#71717a" : "#a1a1aa"
    readonly property color autoSign: dark ? "#fdba74" : "#c2410c"

    readonly property string mono: "Cascadia Mono"
}
