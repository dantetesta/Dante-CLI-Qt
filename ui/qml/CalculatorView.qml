// SPEC-024 — Calculator pane.
// Premium dark UI driven by a C++ controller (`calculator` context property).
// Keyboard + click input share the same Q_INVOKABLE surface so behaviour is
// identical regardless of entry method.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true
    Keys.forwardTo: [keySink]

    property bool historyOpen: true

    function appendChar(s) { calculator.setDisplay(calculator.displayText + s) }

    Item {
        id: keySink
        focus: true
        Keys.onPressed: function(e) {
            const k = e.key
            if (k === Qt.Key_Return || k === Qt.Key_Enter || k === Qt.Key_Equal) {
                calculator.evaluate(); e.accepted = true; return
            }
            if (k === Qt.Key_Backspace) {
                calculator.backspace(); e.accepted = true; return
            }
            if (k === Qt.Key_Delete || k === Qt.Key_Escape) {
                calculator.clear(); e.accepted = true; return
            }
            if (e.text && e.text.length > 0) {
                const ch = e.text
                if ("0123456789.".indexOf(ch) >= 0) {
                    calculator.appendDigit(ch); e.accepted = true; return
                }
                if ("+-*/^%()".indexOf(ch) >= 0) {
                    if (ch === "(" || ch === ")") {
                        root.appendChar(ch)
                    } else {
                        calculator.appendOperator(ch)
                    }
                    e.accepted = true; return
                }
                if (ch.match(/^[a-zA-Z]$/)) {
                    root.appendChar(ch); e.accepted = true; return
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ── Main pad ──
        ColumnLayout {
            id: pad
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // Header bar — title + toggles.
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: "Calculadora"
                    color: Theme.fgStrong
                    font.family: Theme.fontSans
                    font.pixelSize: 16
                    font.bold: true
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: root.historyOpen ? "Histórico ›" : "‹ Histórico"
                    onClicked: root.historyOpen = !root.historyOpen
                    contentItem: Text {
                        text: parent.text
                        color: Theme.fgMuted
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceHigh : "transparent"
                        radius: Theme.radiusSm
                        border.color: Theme.borderSoft
                    }
                }
            }

            // Display.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                color: Theme.surfaceLow
                radius: Theme.radiusLg
                border.color: calculator.lastError.length > 0 ? Theme.danger : Theme.borderSoft

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 4

                    Text {
                        Layout.fillWidth: true
                        text: calculator.lastError.length > 0
                              ? calculator.lastError
                              : (calculator.memory !== 0 ? "M = " + calculator.memory : " ")
                        color: calculator.lastError.length > 0 ? Theme.danger : Theme.fgFaint
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideLeft
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: calculator.displayText.length > 0 ? calculator.displayText : "0"
                        color: Theme.fgStrong
                        font.family: Theme.fontMono
                        font.pixelSize: 28
                        font.bold: true
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignBottom
                        elide: Text.ElideLeft
                    }
                }
            }

            // Memory row.
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CalcButton { text: "MC"; kind: "memory"; onClicked: calculator.memoryClear(); Layout.fillWidth: true }
                CalcButton { text: "MR"; kind: "memory"; onClicked: calculator.memoryRecall(); Layout.fillWidth: true }
                CalcButton { text: "M-"; kind: "memory"; onClicked: calculator.memorySub();    Layout.fillWidth: true }
                CalcButton { text: "M+"; kind: "memory"; onClicked: calculator.memoryAdd();    Layout.fillWidth: true }
            }

            // Function row.
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CalcButton { text: "sin"; kind: "func"; onClicked: root.appendChar("sin("); Layout.fillWidth: true }
                CalcButton { text: "cos"; kind: "func"; onClicked: root.appendChar("cos("); Layout.fillWidth: true }
                CalcButton { text: "tan"; kind: "func"; onClicked: root.appendChar("tan("); Layout.fillWidth: true }
                CalcButton { text: "√";   kind: "func"; onClicked: root.appendChar("sqrt("); Layout.fillWidth: true }
                CalcButton { text: "ln";  kind: "func"; onClicked: root.appendChar("ln(");  Layout.fillWidth: true }
                CalcButton { text: "log"; kind: "func"; onClicked: root.appendChar("log("); Layout.fillWidth: true }
            }

            // Bracket + extra row.
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CalcButton { text: "(";  kind: "func"; onClicked: root.appendChar("(");  Layout.fillWidth: true }
                CalcButton { text: ")";  kind: "func"; onClicked: root.appendChar(")");  Layout.fillWidth: true }
                CalcButton { text: "π";  kind: "func"; onClicked: root.appendChar("pi"); Layout.fillWidth: true }
                CalcButton { text: "e";  kind: "func"; onClicked: root.appendChar("e");  Layout.fillWidth: true }
                CalcButton { text: "x^y";kind: "func"; onClicked: calculator.appendOperator("^"); Layout.fillWidth: true }
                CalcButton { text: "%";  kind: "func"; onClicked: calculator.appendOperator("%"); Layout.fillWidth: true }
            }

            // Number grid + operators.
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                GridLayout {
                    columns: 3
                    rowSpacing: 8
                    columnSpacing: 8
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    CalcButton { text: "C";  kind: "danger"; onClicked: calculator.clear();      Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "CE"; kind: "func";   onClicked: calculator.clearEntry();  Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "⌫";  kind: "func";   onClicked: calculator.backspace();   Layout.fillWidth: true; Layout.fillHeight: true }

                    CalcButton { text: "7"; kind: "digit"; onClicked: calculator.appendDigit("7"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "8"; kind: "digit"; onClicked: calculator.appendDigit("8"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "9"; kind: "digit"; onClicked: calculator.appendDigit("9"); Layout.fillWidth: true; Layout.fillHeight: true }

                    CalcButton { text: "4"; kind: "digit"; onClicked: calculator.appendDigit("4"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "5"; kind: "digit"; onClicked: calculator.appendDigit("5"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "6"; kind: "digit"; onClicked: calculator.appendDigit("6"); Layout.fillWidth: true; Layout.fillHeight: true }

                    CalcButton { text: "1"; kind: "digit"; onClicked: calculator.appendDigit("1"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "2"; kind: "digit"; onClicked: calculator.appendDigit("2"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "3"; kind: "digit"; onClicked: calculator.appendDigit("3"); Layout.fillWidth: true; Layout.fillHeight: true }

                    CalcButton { text: "±"; kind: "digit"; onClicked: calculator.toggleSign();      Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "0"; kind: "digit"; onClicked: calculator.appendDigit("0"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "."; kind: "digit"; onClicked: calculator.appendDigit("."); Layout.fillWidth: true; Layout.fillHeight: true }
                }

                ColumnLayout {
                    spacing: 8
                    Layout.preferredWidth: 88
                    Layout.fillHeight: true
                    CalcButton { text: "÷"; kind: "op"; onClicked: calculator.appendOperator("/"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "×"; kind: "op"; onClicked: calculator.appendOperator("*"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "−"; kind: "op"; onClicked: calculator.appendOperator("-"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "+"; kind: "op"; onClicked: calculator.appendOperator("+"); Layout.fillWidth: true; Layout.fillHeight: true }
                    CalcButton { text: "="; kind: "accent"; onClicked: calculator.evaluate();      Layout.fillWidth: true; Layout.fillHeight: true }
                }
            }
        }

        // ── History pane ──
        Rectangle {
            id: historyPane
            Layout.preferredWidth: root.historyOpen ? 260 : 0
            Layout.fillHeight: true
            color: Theme.surfaceLow
            radius: Theme.radiusLg
            border.color: Theme.borderSoft
            visible: root.historyOpen
            clip: true

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: Theme.motionStd; easing.type: Theme.motionEasing }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Histórico"
                        color: Theme.fgStrong
                        font.family: Theme.fontSans
                        font.pixelSize: 13
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    ToolButton {
                        text: "Limpar"
                        onClicked: calculator.clearHistory()
                        contentItem: Text {
                            text: parent.text
                            color: Theme.fgMuted
                            font.family: Theme.fontSans
                            font.pixelSize: 11
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceHigh : "transparent"
                            radius: Theme.radiusSm
                        }
                    }
                }

                ListView {
                    id: historyList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: calculator.historyModel

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 44
                        radius: Theme.radiusMd
                        color: hover.containsMouse ? Theme.surfaceHigh : Theme.surface
                        border.color: Theme.borderSoft

                        Behavior on color { ColorAnimation { duration: 100 } }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 0
                            Text {
                                Layout.fillWidth: true
                                text: expression
                                color: Theme.fgMuted
                                font.family: Theme.fontMono
                                font.pixelSize: 11
                                elide: Text.ElideLeft
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "= " + formattedValue
                                color: Theme.fgStrong
                                font.family: Theme.fontMono
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideLeft
                            }
                        }

                        MouseArea {
                            id: hover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { calculator.pickHistory(index); calculator.evaluate() }
                        }
                    }

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }
            }
        }
    }

    component CalcButton: Rectangle {
        id: btn
        property string text: ""
        property string kind: "digit"   // digit | op | accent | func | memory | danger
        signal clicked()

        property color baseColor: {
            if (kind === "accent") return Theme.accent
            if (kind === "op")     return Theme.surfaceHigh
            if (kind === "func")   return Theme.surface
            if (kind === "memory") return Theme.surface
            if (kind === "danger") return Theme.surface
            return Theme.surfaceHigh
        }
        property color baseFg: {
            if (kind === "accent") return Theme.fgStrong
            if (kind === "danger") return Theme.danger
            if (kind === "func" || kind === "memory") return Theme.fgMuted
            return Theme.fgStrong
        }

        Layout.preferredHeight: 52
        Layout.minimumHeight: 44
        radius: Theme.radiusMd
        color: ma.pressed
               ? Qt.darker(baseColor, 1.2)
               : (ma.containsMouse ? Qt.lighter(baseColor, 1.15) : baseColor)
        border.color: kind === "accent" ? Theme.accent : Theme.borderSoft

        Behavior on color { ColorAnimation { duration: 100 } }

        Text {
            anchors.centerIn: parent
            text: btn.text
            color: btn.baseFg
            font.family: btn.kind === "func" ? Theme.fontMono : Theme.fontSans
            font.pixelSize: btn.kind === "func" || btn.kind === "memory" ? 13 : 16
            font.bold: btn.kind === "accent" || btn.kind === "op"
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: btn.clicked()
        }
    }

    Component.onCompleted: keySink.forceActiveFocus()
}
