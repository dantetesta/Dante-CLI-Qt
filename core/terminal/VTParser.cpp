#include "VTParser.h"
#include <QDebug>

namespace dante::terminal {

// ---- xterm 256-color palette ----------------------------------------------
// We only need the 16 ANSI colors + a tiny lookup for 8-bit (256). 24-bit RGB
// is built inline.
static constexpr quint32 kAnsi16[16] = {
    0xFF000000, 0xFFCD3131, 0xFF0DBC79, 0xFFE5E510,
    0xFF2472C8, 0xFFBC3FBC, 0xFF11A8CD, 0xFFE5E5E5,
    0xFF666666, 0xFFF14C4C, 0xFF23D18B, 0xFFF5F543,
    0xFF3B8EEA, 0xFFD670D6, 0xFF29B8DB, 0xFFFFFFFF,
};

static quint32 xterm256(int idx) {
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    if (idx < 16) return kAnsi16[idx];
    if (idx < 232) {
        const int v = idx - 16;
        const int r = (v / 36) % 6;
        const int g = (v / 6) % 6;
        const int b = v % 6;
        auto comp = [](int c) { return c == 0 ? 0 : 55 + c * 40; };
        return 0xFF000000u
             | (quint32(comp(r)) << 16)
             | (quint32(comp(g)) << 8)
             |  quint32(comp(b));
    }
    const int v = 8 + (idx - 232) * 10;
    return 0xFF000000u
         | (quint32(v) << 16) | (quint32(v) << 8) | quint32(v);
}

VTParser::VTParser() {
    params_.reserve(16);
}

void VTParser::feed(const QByteArray& bytes) {
    feed(bytes.constData(), bytes.size());
}

void VTParser::feed(const char* data, int len) {
    if (!handler_) return;
    for (int i = 0; i < len; ++i) {
        const quint8 b = static_cast<quint8>(data[i]);

        // C0 controls (except in OSC/DCS) preempt state.
        if (state_ == SOscString) {
            if (b == 0x07) { // BEL terminator
                oscDispatch();
                state_ = SGround;
                continue;
            }
            if (b == 0x1B) {
                // Expect ST: ESC \ — peek next byte; if it's '\', consume both.
                if (i + 1 < len && static_cast<quint8>(data[i + 1]) == 0x5C) {
                    oscDispatch();
                    state_ = SGround;
                    ++i;
                    continue;
                }
                // Bare ESC inside OSC — abort.
                oscDispatch();
                state_ = SEscape;
                continue;
            }
            if (oscBuffer_.size() < 8192) oscBuffer_.append(char(b));
            continue;
        }

        // ESC always resets to escape entry (except inside OSC handled above).
        if (b == 0x1B) {
            state_ = SEscape;
            intermediates_.clear();
            params_.clear();
            currentParam_ = -1;
            privateMarker_ = false;
            continue;
        }

        // Generic C0 dispatch — works from any non-OSC state.
        if (b < 0x20) {
            switch (b) {
                case 0x07: handler_->bell(); break;
                case 0x08: handler_->backspace(); break;
                case 0x09: handler_->tab(); break;
                case 0x0A: case 0x0B: case 0x0C: handler_->lineFeed(); break;
                case 0x0D: handler_->carriageReturn(); break;
                default: break; // ignore the rest
            }
            continue;
        }
        if (b == 0x7F) continue; // DEL — discard

        switch (state_) {
            case SGround:
                utf8Feed(b);
                break;

            case SEscape:
                if (b == '[') { state_ = SCsiEntry; clearCsi(); }
                else if (b == ']') { state_ = SOscString; oscBuffer_.clear(); }
                else if (b == 'P') { state_ = SDcsIgnore; }
                else if (b == 'X' || b == '^' || b == '_') { state_ = SSosPmApcString; }
                else if (b == '(' || b == ')' || b == '*' || b == '+') {
                    state_ = SCharSet;
                    pendingEsc_ = b;
                }
                else { escDispatch(b); state_ = SGround; }
                break;

            case SCharSet:
                // Ignore the designator byte (we don't switch character sets).
                state_ = SGround;
                break;

            case SCsiEntry:
                if (b == '?' || b == '>' || b == '<' || b == '=') {
                    privateMarker_ = (b == '?');
                    intermediates_.append(char(b));
                    state_ = SCsiParam;
                } else if (b >= '0' && b <= '9') {
                    currentParam_ = b - '0';
                    state_ = SCsiParam;
                } else if (b == ';') {
                    params_.push_back(0);
                    state_ = SCsiParam;
                } else if (b >= 0x20 && b <= 0x2F) {
                    intermediates_.append(char(b));
                    state_ = SCsiIntermediate;
                } else if (b >= 0x40 && b <= 0x7E) {
                    csiDispatch(b);
                    state_ = SGround;
                } else {
                    state_ = SCsiIgnore;
                }
                break;

            case SCsiParam:
                if (b >= '0' && b <= '9') {
                    if (currentParam_ < 0) currentParam_ = 0;
                    currentParam_ = currentParam_ * 10 + (b - '0');
                    if (currentParam_ > 65535) currentParam_ = 65535;
                } else if (b == ';' || b == ':') {
                    params_.push_back(currentParam_ < 0 ? 0 : currentParam_);
                    currentParam_ = -1;
                } else if (b >= 0x20 && b <= 0x2F) {
                    if (currentParam_ >= 0) { params_.push_back(currentParam_); currentParam_ = -1; }
                    intermediates_.append(char(b));
                    state_ = SCsiIntermediate;
                } else if (b >= 0x40 && b <= 0x7E) {
                    if (currentParam_ >= 0) { params_.push_back(currentParam_); currentParam_ = -1; }
                    csiDispatch(b);
                    state_ = SGround;
                } else {
                    state_ = SCsiIgnore;
                }
                break;

            case SCsiIntermediate:
                if (b >= 0x20 && b <= 0x2F) intermediates_.append(char(b));
                else if (b >= 0x40 && b <= 0x7E) { csiDispatch(b); state_ = SGround; }
                else state_ = SCsiIgnore;
                break;

            case SCsiIgnore:
                if (b >= 0x40 && b <= 0x7E) state_ = SGround;
                break;

            case SDcsIgnore:
            case SSosPmApcString:
                // Eat until ST (ESC \). We don't need DCS for the target apps.
                if (b == 0x1B && i + 1 < len && static_cast<quint8>(data[i + 1]) == 0x5C) {
                    ++i;
                    state_ = SGround;
                }
                break;

            case SOscString:
                break; // handled above
        }
    }
}

void VTParser::clearCsi() {
    params_.clear();
    currentParam_ = -1;
    intermediates_.clear();
    privateMarker_ = false;
}

void VTParser::utf8Feed(quint8 b) {
    if (utf8Need_ == 0) {
        if (b < 0x80) {
            doPrint(b);
            return;
        }
        if ((b & 0xE0) == 0xC0) { utf8Acc_ = b & 0x1F; utf8Need_ = 1; return; }
        if ((b & 0xF0) == 0xE0) { utf8Acc_ = b & 0x0F; utf8Need_ = 2; return; }
        if ((b & 0xF8) == 0xF0) { utf8Acc_ = b & 0x07; utf8Need_ = 3; return; }
        // Invalid lead — emit replacement.
        doPrint(0xFFFD);
        return;
    }
    if ((b & 0xC0) != 0x80) {
        // Invalid continuation — restart.
        utf8Need_ = 0;
        doPrint(0xFFFD);
        utf8Feed(b);
        return;
    }
    utf8Acc_ = (utf8Acc_ << 6) | (b & 0x3F);
    if (--utf8Need_ == 0) doPrint(utf8Acc_);
}

void VTParser::doPrint(char32_t cp) {
    handler_->printChar(cp, pen_);
}

void VTParser::escDispatch(quint8 finalByte) {
    switch (finalByte) {
        case '7': // DECSC
            savedPen_ = pen_;
            hasSavedPen_ = true;
            handler_->saveCursor();
            break;
        case '8': // DECRC
            if (hasSavedPen_) pen_ = savedPen_;
            handler_->restoreCursor();
            break;
        case 'M': // Reverse Index
            handler_->reverseIndex();
            break;
        case 'D': handler_->lineFeed(); break;            // IND
        case 'E': handler_->carriageReturn(); handler_->lineFeed(); break; // NEL
        case 'c': // RIS — full reset (we just clear screen + reset pen).
            pen_ = PenState{};
            handler_->eraseInDisplay(2);
            handler_->cursorPos(0, 0);
            break;
        default: break;
    }
}

void VTParser::csiDispatch(quint8 finalByte) {
    auto p = [&](size_t idx, int dflt) {
        return idx < params_.size() ? (params_[idx] == 0 ? dflt : params_[idx]) : dflt;
    };
    auto pRaw = [&](size_t idx, int dflt) {
        return idx < params_.size() ? params_[idx] : dflt;
    };

    // Private CSI ? ... — DEC modes.
    if (privateMarker_) {
        const bool set = (finalByte == 'h');
        const bool reset = (finalByte == 'l');
        if (set || reset) {
            for (int code : params_) {
                switch (code) {
                    case 25: handler_->setCursorVisible(set); break;
                    case 1047: case 1049: case 47: handler_->setAltScreen(set); break;
                    case 2004: handler_->setBracketedPaste(set); break;
                    default: break; // 1000/1002/1003/1006 mouse — TODO
                }
            }
        }
        return;
    }

    switch (finalByte) {
        case 'A': handler_->cursorUp(p(0, 1)); break;
        case 'B': case 'e': handler_->cursorDown(p(0, 1)); break;
        case 'C': case 'a': handler_->cursorForward(p(0, 1)); break;
        case 'D': handler_->cursorBack(p(0, 1)); break;
        case 'E': handler_->carriageReturn(); handler_->cursorDown(p(0, 1)); break;
        case 'F': handler_->carriageReturn(); handler_->cursorUp(p(0, 1)); break;
        case 'G': case '`': handler_->cursorColumn(p(0, 1) - 1); break;
        case 'H': case 'f':
            handler_->cursorPos(p(0, 1) - 1, p(1, 1) - 1);
            break;
        case 'd': // VPA — line position absolute
            handler_->cursorPos(p(0, 1) - 1, /*col preserved*/-1);
            break;
        case 'J': handler_->eraseInDisplay(pRaw(0, 0)); break;
        case 'K': handler_->eraseInLine(pRaw(0, 0)); break;
        case 'L': handler_->insertLines(p(0, 1)); break;
        case 'M': handler_->deleteLines(p(0, 1)); break;
        case 'P': handler_->deleteChars(p(0, 1)); break;
        case 'm': applySgr(); break;
        case 'r':
            // CSI Ps ; Ps r — scroll region. Both default to "full screen"
            // which we encode as -1.
            handler_->setScrollRegion(p(0, 1) - 1, pRaw(1, 0) ? pRaw(1, 0) - 1 : -1);
            break;
        case 's': handler_->saveCursor(); break;
        case 'u': handler_->restoreCursor(); break;
        case 'q':
            // DECSCUSR — cursor shape (intermediates_ should contain ' ').
            handler_->setCursorStyle(pRaw(0, 0));
            break;
        case 'n':
            // Device status report. 6 = report cursor pos. We can't talk back
            // through the handler interface yet (would need write-callback);
            // most apps work without this.
            break;
        case 't':
            // Window manipulation — ignore.
            break;
        case 'X':
            // ECH — erase n chars. Approximate via deleteChars; close enough
            // for prompt redraws.
            handler_->deleteChars(p(0, 1));
            break;
        default: break;
    }
}

void VTParser::applySgr() {
    if (params_.empty()) params_.push_back(0);
    for (size_t i = 0; i < params_.size(); ++i) {
        const int v = params_[i];
        switch (v) {
            case 0:
                pen_ = PenState{};
                break;
            case 1:  pen_.attrs |= CellAttr::Bold; break;
            case 2:  pen_.attrs |= CellAttr::Faint; break;
            case 3:  pen_.attrs |= CellAttr::Italic; break;
            case 4:  pen_.attrs |= CellAttr::Underline; break;
            case 7:  pen_.attrs |= CellAttr::Inverse; break;
            case 22: pen_.attrs &= ~(CellAttr::Bold | CellAttr::Faint); break;
            case 23: pen_.attrs &= ~CellAttr::Italic; break;
            case 24: pen_.attrs &= ~CellAttr::Underline; break;
            case 27: pen_.attrs &= ~CellAttr::Inverse; break;
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                pen_.fg = kAnsi16[v - 30];
                pen_.fgDefault = false;
                break;
            case 39: pen_.fg = 0xFFE6E6EB; pen_.fgDefault = true; break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                pen_.bg = kAnsi16[v - 40];
                pen_.bgDefault = false;
                break;
            case 49: pen_.bg = 0xFF0E0E12; pen_.bgDefault = true; break;
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                pen_.fg = kAnsi16[v - 90 + 8];
                pen_.fgDefault = false;
                break;
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                pen_.bg = kAnsi16[v - 100 + 8];
                pen_.bgDefault = false;
                break;
            case 38:
            case 48: {
                if (i + 1 >= params_.size()) break;
                const int mode = params_[i + 1];
                if (mode == 5 && i + 2 < params_.size()) {
                    const quint32 c = xterm256(params_[i + 2]);
                    if (v == 38) { pen_.fg = c; pen_.fgDefault = false; }
                    else         { pen_.bg = c; pen_.bgDefault = false; }
                    i += 2;
                } else if (mode == 2 && i + 4 < params_.size()) {
                    const quint32 c = 0xFF000000u
                        | (quint32(params_[i + 2] & 0xFF) << 16)
                        | (quint32(params_[i + 3] & 0xFF) <<  8)
                        |  quint32(params_[i + 4] & 0xFF);
                    if (v == 38) { pen_.fg = c; pen_.fgDefault = false; }
                    else         { pen_.bg = c; pen_.bgDefault = false; }
                    i += 4;
                } else {
                    // unknown extended mode — bail out of this SGR group
                    i = params_.size();
                }
                break;
            }
            default: break;
        }
    }
}

void VTParser::oscDispatch() {
    const int sep = oscBuffer_.indexOf(';');
    if (sep <= 0) { oscBuffer_.clear(); return; }
    bool ok = false;
    const int code = QByteArray(oscBuffer_.left(sep)).toInt(&ok);
    if (!ok) { oscBuffer_.clear(); return; }
    handler_->osc(code, QString::fromUtf8(oscBuffer_.mid(sep + 1)));
    oscBuffer_.clear();
}

} // namespace dante::terminal
