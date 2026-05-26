// SPEC-130 — full emoji picker mirroring the Swift EmojiCatalog.
// Categories tab strip (Recentes / Dev / Office / AI / Bichinhos /
// Natureza / Comida / Esportes / Veículos / Pessoas / Sentimentos /
// Símbolos / Bandeiras), search input, grid of clickable cells, recents
// auto-populated via AppState.pushRecentEmoji.
//
// Catalogue data is duplicated verbatim from
// `TestaTerminal/Models/EmojiCatalog.swift` — same order, same dedup
// pre-applied, so the visible grid matches macOS.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import ".."

MovablePopup {
    id: root
    width: 480
    height: 460
    minWidth: 360
    minHeight: 320
    title: qsTr("Escolher emoji")
    icon: "😀"

    signal selected(string emoji)

    property int  currentCategory: 0
    property string query: ""

    readonly property var categories: [
        { label: qsTr("Recentes"),    icon: "🕒", emojis: appState.recentEmojis },
        { label: qsTr("Dev"),         icon: "💻", emojis: [
            "💻","🖥️","⌨️","🖱️","🖲️","💾","💿","📀","🧮","🔌","🔋","🪫","📱","☎️","📞","📟","📠",
            "📷","📸","📹","🎥","📽️","🎞️","📺","📻","🎙️","🎚️","🎛️","🧭","⏱️","⏲️","⏰","🕰️",
            "🛠️","🧰","🔨","⛏️","⚒️","🪓","🔩","🪛","⚙️","🗜️","⚖️","🔗","⛓️","🪝","🧲","🪜",
            "📦","🗂️","📁","📂","🗃️","🗄️","🗑️","📋","📑","📜","📝","📌","📍","📎","🖇️",
            "📃","📄","🧾","📊","📈","📉","🗞️","📰","🔖","🏷️","✉️","📧","📨","📩","📤","📥","📭","📬","📫","📪",
            "🚀","🛸","🪐","✨","💡","🔥","⚡️","☄️","🌟","🌠","💥","🐛","🪲","🔧","🛡️",
            "🔒","🔓","🔑","🗝️","🔐","🛎️","🔔","🔕","🔍","🔎","🔬","🧪","🧬","🧫","🦠",
            "🎯","🎲","🎮","🕹️","🪄","📡","📶","📲","🤖","👾","🧠","🦾","🌐","♾️","🔮","🧿"
        ]},
        { label: qsTr("Office"),      icon: "💼", emojis: [
            "💼","🎒","👔","👕","🧥","🥼","🦺","👖","👗","🎓","👨‍💻","👩‍💻","👨‍🔧","👩‍🔧","👨‍🔬","👩‍🔬",
            "👨‍🏫","👩‍🏫","👨‍⚖️","👩‍⚖️","👨‍💼","👩‍💼","🧑‍💻","🧑‍🎨","🧑‍🔧","🧑‍🚀","🧑‍🍳","🧑‍🎓","🧑‍🏫",
            "📅","📆","🗓️","🗒️","📇","🗃️","🔖","💳","🧾","💴","💵","💶","💷","💰","💸",
            "🏦","🏧","🪙","💲","💹","🎫","🎟️","📒","📕","📗","📘","📙","📚","📖","🗨️","🗯️","💬","💭"
        ]},
        { label: qsTr("AI & Tech"),   icon: "✨", emojis: [
            "🤖","👾","🛸","🛰️","✨","💫","🪄","🌠","🎩","🧠","🦾","🦿","🌐","♾️","🔮","🧿",
            "🎭","🃏","🎲","🎯","🪐","🌍","🌎","🌏","🌑","🌒","🌓","🌔","🌕","🌖","🌗","🌘",
            "💎","🪩","🎰","🎮","🕹️","📡","📶","📲","🔋","🔌","💡","🔦","🕯️","🪔",
            "🛡️","⚔️","🗡️","🔱","🪬","🎁","🎀","🪅","🪆","🪞","🪟"
        ]},
        { label: qsTr("Bichinhos"),   icon: "🐾", emojis: [
            "🐶","🐱","🐭","🐹","🐰","🦊","🐻","🐼","🐻‍❄️","🐨","🐯","🦁","🐮","🐷","🐽","🐸","🐵",
            "🙈","🙉","🙊","🐒","🦍","🦧","🐺","🐗","🐴","🦄","🦓","🦌","🦒","🐘","🦣","🦏","🦛","🐄",
            "🐂","🐃","🐎","🐖","🐏","🐑","🐐","🐪","🐫","🦙","🐕","🐩","🐈","🐇","🐀","🐁","🐿️","🦫","🦔","🦇",
            "🐔","🐧","🐦","🐤","🐣","🐥","🦆","🦢","🦉","🦅","🪶","🐓","🦃","🦤","🦚","🦜","🦩",
            "🐝","🪱","🐛","🦋","🐌","🐞","🐜","🪰","🪲","🪳","🦟","🦗","🕷️","🕸️","🦂",
            "🐢","🐍","🦎","🦖","🦕","🐊",
            "🐙","🦑","🦐","🦞","🦀","🐡","🐠","🐟","🐬","🐳","🐋","🦈","🐚"
        ]},
        { label: qsTr("Natureza"),    icon: "🌿", emojis: [
            "🌱","🌿","☘️","🍀","🎋","🎍","🌵","🌴","🌳","🌲","🪵","🍂","🍁","🍃","🌾","🪨","🪺","🪹",
            "🌷","🌹","🥀","🪻","🪷","💐","🌺","🌸","🌼","🌻","🌞","🌝","🌛","🌜","🌚","🌕","🌖","🌗","🌘",
            "🌑","🌒","🌓","🌔","🌙","🌎","🌍","🌏","💫","⭐️","🌟","🌠","☄️","☀️","🌤","⛅️","🌥","☁️","🌦",
            "🌧","⛈","🌩","🌨","❄️","☃️","⛄️","🌬","💨","💧","💦","☔️","☂️","🌊","🌫","🌈","🔥","💥",
            "🌋","⛰️","🏔️","🗻","🏕️","🏖️","🏝️","🏜️","🏞️"
        ]},
        { label: qsTr("Comida"),      icon: "🍔", emojis: [
            "🍎","🍊","🍋","🍌","🍉","🍇","🍓","🫐","🍈","🍒","🍑","🥭","🍍","🥥","🥝","🍅",
            "🍆","🥑","🥦","🥬","🥒","🌶️","🫑","🌽","🥕","🫒","🧄","🧅","🥔","🍠","🥐","🥯",
            "🍞","🥖","🥨","🧀","🥚","🍳","🧈","🥞","🧇","🥓","🥩","🍗","🍖","🦴","🌭","🍔",
            "🍟","🍕","🥪","🥙","🧆","🌮","🌯","🫔","🥗","🥘","🫕","🍝","🍜","🍲","🍛","🍣",
            "🍱","🥟","🦪","🍤","🍙","🍚","🍘","🍥","🥠","🥮","🍢","🍡","🍧","🍨","🍦","🥧",
            "🧁","🍰","🎂","🍮","🍭","🍬","🍫","🍿","🍩","🍪","🌰","🥜","🫘","🍯","🥛","🫗","🍵",
            "🍶","🍾","🥂","🍷","🥃","🍸","🍹","🧉","🍺","🍻","☕️","🧋","🥤","🧃","🧊"
        ]},
        { label: qsTr("Esportes"),    icon: "⚽", emojis: [
            "⚽️","🏀","🏈","⚾️","🥎","🎾","🏐","🏉","🥏","🎱","🪀","🏓","🏸","🏒","🏑","🥍",
            "🏏","🪃","🥅","⛳️","🪁","🏹","🎣","🤿","🥊","🥋","🎽","🛹","🛼","🛷","⛸️","🥌",
            "🎿","⛷️","🏂","🪂","🏋️","🤼","🤸","🤺","⛹️","🤾","🏌️","🏇","🧘","🏄","🏊","🤽",
            "🚣","🧗","🚵","🚴","🏆","🥇","🥈","🥉","🏅","🎖️","🏵️","🎗️","🎫","🎟️","🎪","🤹",
            "🎭","🩰","🎨","🎬","🎤","🎧","🎼","🎹","🥁","🪘","🎷","🎺","🎸","🪕","🎻","🪗",
            "🎲","♟️","🎯","🎳","🎰","🧩","🎮","🕹️"
        ]},
        { label: qsTr("Veículos"),    icon: "🚗", emojis: [
            "🚗","🚕","🚙","🚌","🚎","🏎️","🚓","🚑","🚒","🚐","🛻","🚚","🚛","🚜","🏍️","🛵","🚲","🛴","🛹","🛼",
            "🚂","🚆","🚉","🚄","🚅","🚈","🚇","🚊","🚝","🚞","🚋","🚃","🚟","🚠","🚡","⛵️","🚤","🛥️","🛳️","⛴",
            "🚢","✈️","🛩️","🛫","🛬","🪂","💺","🚁","⚓️","🛟","⛽️","🚦","🚥","🚧","🪧","🚏",
            "🗺️","🗿","🗽","🗼","🏰","🏯","🏟️","🎡","🎢","🎠","⛲️","⛱️","🏖️","🏝️","🏜️","🌋","⛰️","🏔️",
            "🗻","🏕️","⛺️","🏠","🏡","🏘️","🏚️","🏗️","🏭","🏢","🏬","🏣","🏤","🏥","🏦","🏨","🏪","🏫","🏩",
            "💒","⛪️","🕌","🕍","🛕","🕋"
        ]},
        { label: qsTr("Pessoas"),     icon: "👋", emojis: [
            "👋","🤚","🖐","✋","🖖","👌","🤌","🤏","✌️","🤞","🫰","🤟","🤘","🤙","🫵","👈","👉","👆",
            "🖕","👇","☝️","🫳","🫴","🫱","🫲","👍","👎","👊","✊","🤛","🤜","👏","🙌","🫶","👐","🤲","🤝","🙏","✍️",
            "💅","🤳","💪","🦾","🦵","🦿","🦶","👂","🦻","👃","🧠","🫀","🫁","🦷","🦴","👀",
            "👁","👅","👄","🫦","💋","👶","🧒","👦","👧","🧑","👱","👨","🧔","👩","🧓","👴","👵",
            "🙅","🙆","💁","🙋","🧏","🙇","🤦","🤷"
        ]},
        { label: qsTr("Sentimentos"), icon: "😀", emojis: [
            "😀","😃","😄","😁","😆","😅","🤣","😂","🙂","🙃","🫠","😉","😊","😇","🥰","😍","🤩",
            "😘","😗","☺️","😚","😙","🥲","😋","😛","😜","🤪","😝","🤑","🤗","🫡","🤭","🫢","🫣","🤫","🤔",
            "🫥","🤐","🤨","😐","😑","😶","🫨","🙄","😏","😒","😬","😮‍💨","🤥","😌","😔","😪","🤤",
            "😴","😷","🤒","🤕","🤢","🤮","🤧","🥵","🥶","🥴","😵","😵‍💫","🤯","🤠","🥳","🥸",
            "😎","🤓","🧐","😕","🫤","😟","🙁","☹️","😮","😯","😲","😳","🥺","🥹","😦","😧","😨","😰",
            "😥","😢","😭","😱","😖","😣","😞","😓","😩","😫","🥱","😤","😡","😠","🤬","😈",
            "👿","💀","☠️","💩","🤡","👹","👺","👻","👽","👾","🤖","😺","😸","😹","😻","😼",
            "😽","🙀","😿","😾"
        ]},
        { label: qsTr("Símbolos"),    icon: "❤", emojis: [
            "❤️","🧡","💛","💚","💙","💜","🖤","🤍","🤎","🩷","🩵","🩶","💔","❣️","💕","💞","💓","💗","💖","💘","💝","💟",
            "☮️","✝️","☪️","🕉","☸️","✡️","🔯","🕎","☯️","☦️","🛐",
            "⛎","♈️","♉️","♊️","♋️","♌️","♍️","♎️","♏️","♐️","♑️","♒️","♓️",
            "❌","⭕️","🛑","⛔️","📛","🚫","💯","💢","♨️","🚷","🚯","🚳","🚱","🔞","📵","🚭",
            "❗️","❕","❓","❔","‼️","⁉️","💤","💥","💫","💦","💨",
            "▶️","⏸","⏯","⏹","⏺","⏭","⏮","⏩","⏪","⏫","⏬","◀️","🔼","🔽","➡️","⬅️","⬆️",
            "⬇️","↗️","↘️","↙️","↖️","↕️","↔️","↪️","↩️","⤴️","⤵️","🔀","🔁","🔂","🔄","🔃",
            "✅","☑️","✔️","✖️","❎","➕","➖","➗","✳️","✴️","❇️","🆗","🆖","🆕","🆒","🆓","🆙",
            "0️⃣","1️⃣","2️⃣","3️⃣","4️⃣","5️⃣","6️⃣","7️⃣","8️⃣","9️⃣","🔟","🔢","#️⃣","*️⃣","⏏️",
            "🔅","🔆","🔇","🔈","🔉","🔊","📢","📣","🔔","🔕"
        ]},
        { label: qsTr("Bandeiras"),   icon: "🇧🇷", emojis: [
            "🇧🇷",
            "🇵🇹","🇦🇴","🇲🇿","🇨🇻","🇬🇼","🇸🇹","🇹🇱",
            "🇺🇸","🇨🇦","🇲🇽","🇦🇷","🇨🇱","🇨🇴","🇵🇪","🇺🇾","🇵🇾","🇻🇪","🇪🇨","🇧🇴","🇨🇺",
            "🇩🇴","🇵🇦","🇨🇷","🇬🇹","🇭🇳","🇸🇻","🇳🇮","🇯🇲","🇭🇹","🇵🇷","🇧🇸","🇧🇧","🇹🇹","🇸🇷","🇬🇾",
            "🇪🇸","🇫🇷","🇮🇹","🇩🇪","🇬🇧","🇮🇪","🇳🇱","🇧🇪","🇨🇭","🇦🇹","🇵🇱","🇨🇿","🇸🇰","🇭🇺",
            "🇸🇪","🇳🇴","🇩🇰","🇫🇮","🇮🇸","🇬🇷","🇷🇴","🇧🇬","🇭🇷","🇷🇸","🇲🇪","🇸🇮","🇪🇪","🇱🇻",
            "🇱🇹","🇺🇦","🇧🇾","🇲🇩","🇲🇰","🇦🇱","🇽🇰","🇧🇦","🇲🇹","🇱🇺","🇲🇨","🇦🇩","🇸🇲","🇻🇦","🇬🇮","🇪🇺",
            "🇯🇵","🇨🇳","🇰🇷","🇰🇵","🇮🇳","🇵🇰","🇧🇩","🇱🇰","🇲🇲","🇹🇭","🇻🇳","🇰🇭","🇱🇦","🇲🇾",
            "🇸🇬","🇮🇩","🇵🇭","🇧🇳","🇲🇳","🇰🇿","🇺🇿","🇰🇬","🇹🇯","🇹🇲","🇦🇫","🇮🇷","🇮🇶","🇸🇾",
            "🇱🇧","🇯🇴","🇮🇱","🇵🇸","🇸🇦","🇦🇪","🇶🇦","🇰🇼","🇧🇭","🇴🇲","🇾🇪","🇹🇷","🇨🇾","🇬🇪",
            "🇦🇲","🇦🇿","🇭🇰","🇹🇼","🇲🇴","🇳🇵","🇧🇹","🇲🇻",
            "🇪🇬","🇿🇦","🇳🇬","🇰🇪","🇲🇦","🇩🇿","🇹🇳","🇱🇾","🇸🇩","🇪🇹","🇪🇷","🇸🇴","🇩🇯","🇺🇬",
            "🇹🇿","🇷🇼","🇧🇮","🇨🇩","🇨🇬","🇨🇫","🇨🇲","🇬🇦","🇬🇶","🇸🇨","🇲🇺","🇰🇲","🇲🇬","🇿🇲",
            "🇿🇼","🇲🇼","🇧🇼","🇳🇦","🇱🇸","🇸🇿","🇸🇳","🇲🇱","🇧🇫","🇳🇪","🇹🇩","🇲🇷","🇬🇲","🇬🇳",
            "🇸🇱","🇱🇷","🇨🇮","🇬🇭","🇹🇬","🇧🇯",
            "🇦🇺","🇳🇿","🇫🇯","🇵🇬","🇸🇧","🇻🇺","🇼🇸","🇹🇴","🇰🇮","🇲🇭","🇵🇼","🇫🇲","🇳🇷",
            "🇹🇻","🇨🇰","🇳🇺",
            "🇷🇺","🏳️","🏴","🏁","🚩","🏳️‍🌈","🏳️‍⚧️","🏴‍☠️"
        ]}
    ]

    /// Computed: emojis to render. If a search query is active we ignore
    /// the category and flatten the catalogue.
    readonly property var visibleEmojis: {
        if (root.query.trim().length > 0) {
            let pool = []
            for (let i = 1; i < categories.length; ++i)
                pool = pool.concat(categories[i].emojis)
            const q = root.query.trim().toLowerCase()
            return pool.filter(e => e.toLowerCase().indexOf(q) >= 0)
        }
        const c = categories[root.currentCategory] || categories[0]
        return c.emojis || []
    }

    function pick(emoji) {
        if (!emoji || emoji.length === 0) return
        appState.pushRecentEmoji(emoji)
        root.selected(emoji)
        root.close()
    }

    /* ─── Body ─── */
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            radius: 6
            color: Theme.surfaceLow
            border.color: searchField.activeFocus ? Theme.accent : Theme.borderSoft
            TextField {
                id: searchField
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                placeholderText: qsTr("Buscar… (e.g. heart, rocket)")
                color: Theme.fg
                background: null
                onTextChanged: root.query = text
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: "transparent"
            visible: root.query.trim().length === 0
            ListView {
                anchors.fill: parent
                orientation: ListView.Horizontal
                spacing: 4
                clip: true
                model: root.categories
                delegate: Rectangle {
                    width: 42
                    height: parent ? parent.height - 6 : 32
                    y: 3
                    radius: 6
                    color: root.currentCategory === index
                              ? Theme.accentDim
                              : (catMa.containsMouse ? Theme.surfaceHigh : "transparent")
                    border.color: root.currentCategory === index ? Theme.accentSoft : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    ToolTip.text: modelData.label
                    ToolTip.visible: catMa.containsMouse
                    ToolTip.delay: 400
                    Text {
                        anchors.centerIn: parent
                        text: modelData.icon
                        font.pixelSize: 18
                    }
                    MouseArea {
                        id: catMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentCategory = index
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSoft
            opacity: 0.5
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GridView {
                id: grid
                model: root.visibleEmojis
                cellWidth: 38
                cellHeight: 38
                anchors.fill: parent
                delegate: Rectangle {
                    width: 36; height: 36
                    radius: 6
                    color: cellMa.containsMouse ? Theme.surfaceHigh : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 22
                    }
                    MouseArea {
                        id: cellMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.pick(modelData)
                    }
                }
                Text {
                    visible: grid.count === 0
                    anchors.centerIn: parent
                    text: qsTr("Nada encontrado")
                    color: Theme.fgFaint
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("%1 emojis").arg(grid.count)
            color: Theme.fgFaint
            font.family: Theme.fontSans
            font.pixelSize: 10
        }
    }
}
