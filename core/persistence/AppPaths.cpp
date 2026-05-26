#include "AppPaths.h"
#include <QStandardPaths>

namespace dante::persistence {

QDir appDataDir() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir d(base);
    if (!d.exists()) d.mkpath(".");
    return d;
}

QString file(QString name) {
    return appDataDir().filePath(name);
}

} // namespace dante::persistence
