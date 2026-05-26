#include "TerminalSession.h"
#include "../../infra/shell-adapters/ShellAdapter.h"
#include <QDebug>
#include <QUrl>

namespace dante::terminal {

TerminalSession::TerminalSession(QString sessionId, QObject* parent)
    : QObject(parent)
    , sessionId_(std::move(sessionId))
    , shell_(std::make_unique<infra::ShellAdapter>(this))
{
    connect(shell_.get(), &infra::ShellAdapter::outputReceived,
            this, [this](const QString& s) {
                feedOsc(s);
                emit output(s);
            });
    connect(shell_.get(), &infra::ShellAdapter::processExited,
            this, &TerminalSession::exited);
}

TerminalSession::~TerminalSession() = default;

bool TerminalSession::start(const QString& shell, const QString& cwd, int cols, int rows) {
    if (!shell_->start(shell, cwd, cols, rows)) {
        error_ = shell_->errorString();
        return false;
    }
    return true;
}

void TerminalSession::resize(int cols, int rows) {
    shell_->resize(cols, rows);
}

void TerminalSession::write(const QByteArray& bytes) {
    shell_->write(bytes);
}

void TerminalSession::kill() {
    shell_->kill();
}

// Minimal OSC interceptor — mirrors `oscParser.ts` from the Tauri version.
// Looks for ESC ] <n> ; <payload> (BEL | ESC \) and extracts cwd / title.
void TerminalSession::feedOsc(const QString& chunk) {
    const QByteArray bytes = chunk.toUtf8();
    for (int i = 0; i < bytes.size(); ++i) {
        const char b = bytes[i];
        if (!inOsc_) {
            if (b == 0x1B && i + 1 < bytes.size() && bytes[i + 1] == ']') {
                inOsc_ = true;
                oscBuffer_.clear();
                ++i; // skip ']'
            }
            continue;
        }
        if (b == 0x07) { // BEL
            // Process oscBuffer_.
            const int sep = oscBuffer_.indexOf(';');
            if (sep > 0) {
                const QByteArray code = oscBuffer_.left(sep);
                const QByteArray payload = oscBuffer_.mid(sep + 1);
                if (code == "7") {
                    QString p = QString::fromUtf8(payload);
                    // file://host/path  → strip scheme + decode.
                    if (p.startsWith("file://")) {
                        const auto slash = p.indexOf('/', 7);
                        if (slash >= 0) p = p.mid(slash);
                    }
                    emit cwdChanged(QUrl::fromPercentEncoding(p.toUtf8()));
                } else if (code == "0" || code == "1" || code == "2") {
                    emit titleChanged(QString::fromUtf8(payload));
                }
            }
            inOsc_ = false;
            oscBuffer_.clear();
            continue;
        }
        if (b == 0x1B && i + 1 < bytes.size() && bytes[i + 1] == '\\') {
            // ST — same handling as BEL, just skip the following backslash.
            const int sep = oscBuffer_.indexOf(';');
            if (sep > 0) {
                const QByteArray code = oscBuffer_.left(sep);
                const QByteArray payload = oscBuffer_.mid(sep + 1);
                if (code == "7") {
                    QString p = QString::fromUtf8(payload);
                    if (p.startsWith("file://")) {
                        const auto slash = p.indexOf('/', 7);
                        if (slash >= 0) p = p.mid(slash);
                    }
                    emit cwdChanged(QUrl::fromPercentEncoding(p.toUtf8()));
                } else if (code == "0" || code == "1" || code == "2") {
                    emit titleChanged(QString::fromUtf8(payload));
                }
            }
            inOsc_ = false;
            oscBuffer_.clear();
            ++i;
            continue;
        }
        oscBuffer_.append(b);
        if (oscBuffer_.size() > 4096) { // sanity
            inOsc_ = false;
            oscBuffer_.clear();
        }
    }
}

} // namespace dante::terminal
