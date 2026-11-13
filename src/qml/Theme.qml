// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

pragma Singleton
import QtQuick

// Catppuccin (https://catppuccin.com): Mocha for the dark theme, Latte for the light one. The
// names on the left are ours; the values are the palette's, untouched. State colours mean one
// thing each (see doc/01. gui-design.md) and are never used for decoration.
QtObject {
    id: theme

    readonly property bool dark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    // Surfaces, from the back of the window to the front: crust, mantle, base, surface0..2.
    readonly property color crust: dark ? "#11111b" : "#dce0e8"
    readonly property color mantle: dark ? "#181825" : "#e6e9ef"
    readonly property color base: dark ? "#1e1e2e" : "#eff1f5"
    readonly property color surface0: dark ? "#313244" : "#ccd0da"
    readonly property color surface1: dark ? "#45475a" : "#bcc0cc"
    readonly property color surface2: dark ? "#585b70" : "#acb0be"
    readonly property color overlay0: dark ? "#6c7086" : "#9ca0b0"
    readonly property color overlay1: dark ? "#7f849c" : "#8c8fa1"
    readonly property color subtext0: dark ? "#a6adc8" : "#6c6f85"
    readonly property color subtext1: dark ? "#bac2de" : "#5c5f77"

    readonly property color canvas: mantle
    readonly property color surface: base
    readonly property color surfaceAlt: base
    readonly property color rail: crust
    readonly property color border: surface0
    readonly property color hairline: surface0
    readonly property color text: dark ? "#cdd6f4" : "#4c4f69"
    readonly property color muted: subtext0
    readonly property color subtle: overlay1

    // Accents.
    readonly property color blue: dark ? "#89b4fa" : "#1e66f5"
    readonly property color lavender: dark ? "#b4befe" : "#7287fd"
    readonly property color mauve: dark ? "#cba6f7" : "#8839ef"
    readonly property color peach: dark ? "#fab387" : "#fe640b"
    readonly property color yellow: dark ? "#f9e2af" : "#df8e1d"
    readonly property color green: dark ? "#a6e3a1" : "#40a02b"
    readonly property color red: dark ? "#f38ba8" : "#d20f39"

    readonly property color accent: blue
    readonly property color accentSoft: surface1
    readonly property color accentText: dark ? "#cdd6f4" : "#1e66f5"

    readonly property color pending: yellow
    readonly property color signing: blue
    readonly property color signed: green
    readonly property color refused: red
    readonly property color expired: overlay0
    readonly property color autoSign: peach

    // A state colour as a wash behind text in that colour (dark) or in the text colour (light).
    function tint(colour, alpha) {
        return Qt.rgba(colour.r, colour.g, colour.b, alpha);
    }

    readonly property color bannerAutoBackground: tint(peach, dark ? 0.18 : 0.16)
    readonly property color bannerAutoForeground: dark ? peach : "#9a3d06"
    readonly property color bannerWarningBackground: tint(yellow, dark ? 0.16 : 0.18)
    readonly property color bannerWarningForeground: dark ? yellow : "#8a5a0d"
    readonly property color bannerErrorBackground: tint(red, dark ? 0.18 : 0.12)
    readonly property color bannerErrorForeground: dark ? red : "#a30c2e"

    readonly property string mono: "Cascadia Mono"
}
