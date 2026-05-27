#include "AutoFillEngine.h"

#include <QChar>
#include <QHash>
#include <QRegularExpression>

namespace dante::autofill {

namespace {

// Slugify a label into a safe placeholder name. Lowercase, ASCII letters/
// digits, separators collapsed to single '_'. Used to deduplicate $(prompt)
// placeholders that share the same label — we want them resolved once.
QString slugify(const QString& s) {
    QString out;
    out.reserve(s.size());
    bool lastSep = true;
    for (const QChar& c : s) {
        if (c.isLetterOrNumber()) {
            out.append(c.toLower());
            lastSep = false;
        } else if (!lastSep) {
            out.append(QLatin1Char('_'));
            lastSep = true;
        }
    }
    while (out.endsWith(QLatin1Char('_'))) out.chop(1);
    return out.isEmpty() ? QStringLiteral("prompt") : out;
}

bool isIdentChar(QChar c) {
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

// Reads `$VAR` starting at i (which must point at '$'). Returns the end index
// (one past last consumed char) and fills out. Returns -1 on no match.
int parseSimpleVar(const QString& t, int i, Placeholder* out) {
    if (i + 1 >= t.size()) return -1;
    const QChar first = t.at(i + 1);
    if (!(first.isLetter() || first == QLatin1Char('_'))) return -1;
    int j = i + 1;
    while (j < t.size() && isIdentChar(t.at(j))) ++j;
    const QString name = t.mid(i + 1, j - (i + 1));
    out->rawToken = t.mid(i, j - i);
    out->name = name;
    out->label = name;
    out->kind = QStringLiteral("text");
    out->defaultValue.clear();
    out->required = true;
    return j;
}

// Reads `${VAR}` or `${VAR:default}` starting at i (which must point at '$'
// followed by '{'). Returns end index, -1 on no match.
int parseBracedVar(const QString& t, int i, Placeholder* out) {
    if (i + 2 >= t.size() || t.at(i + 1) != QLatin1Char('{')) return -1;
    const int close = t.indexOf(QLatin1Char('}'), i + 2);
    if (close < 0) return -1;
    const QString inner = t.mid(i + 2, close - (i + 2));
    if (inner.isEmpty()) return -1;
    QString name = inner;
    QString def;
    bool hasDefault = false;
    const int colon = inner.indexOf(QLatin1Char(':'));
    if (colon >= 0) {
        name = inner.left(colon);
        def = inner.mid(colon + 1);
        hasDefault = true;
    }
    // Validate identifier.
    if (name.isEmpty()) return -1;
    if (!(name.at(0).isLetter() || name.at(0) == QLatin1Char('_'))) return -1;
    for (const QChar& c : name) if (!isIdentChar(c)) return -1;
    out->rawToken = t.mid(i, close - i + 1);
    out->name = name;
    out->label = name;
    out->kind = QStringLiteral("text");
    out->defaultValue = def;
    out->required = !hasDefault;
    return close + 1;
}

// Reads `$(label)` or `$(label|kind)` starting at i (which must point at '$'
// followed by '('). Handles nested () pairs and pipes inside `choice:...`.
int parsePromptVar(const QString& t, int i, Placeholder* out) {
    if (i + 2 >= t.size() || t.at(i + 1) != QLatin1Char('(')) return -1;
    int depth = 1;
    int j = i + 2;
    while (j < t.size() && depth > 0) {
        const QChar c = t.at(j);
        if (c == QLatin1Char('(')) ++depth;
        else if (c == QLatin1Char(')')) --depth;
        if (depth == 0) break;
        ++j;
    }
    if (depth != 0) return -1;
    const QString inner = t.mid(i + 2, j - (i + 2));
    QString label = inner;
    QString kind = QStringLiteral("text");
    QStringList choices;
    // Split on the LAST '|' so a label like "Foo | Bar" can be escaped by
    // not adding a kind — but here we keep the simple rule: a single '|'
    // separates label from kind. The kind portion is short and shell-safe.
    const int bar = inner.indexOf(QLatin1Char('|'));
    if (bar >= 0) {
        label = inner.left(bar).trimmed();
        QString rawKind = inner.mid(bar + 1).trimmed();
        if (rawKind.startsWith(QStringLiteral("choice:"), Qt::CaseInsensitive)) {
            kind = QStringLiteral("choice");
            const QString list = rawKind.mid(QStringLiteral("choice:").size());
            for (const QString& part : list.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
                choices.append(part.trimmed());
            }
        } else {
            kind = rawKind.toLower();
        }
    }
    if (label.isEmpty()) return -1;
    out->rawToken = t.mid(i, j - i + 1);
    out->name = slugify(label);
    out->label = label;
    out->kind = kind;
    out->choices = choices;
    out->defaultValue.clear();
    out->required = true;
    return j + 1;
}

} // namespace

QVector<Placeholder> AutoFillEngine::scan(const QString& templateText) {
    QVector<Placeholder> result;
    QHash<QString, int> indexByName;
    const int n = templateText.size();
    int i = 0;
    while (i < n) {
        if (templateText.at(i) != QLatin1Char('$')) { ++i; continue; }
        // Escaped dollar: "$$" → literal "$" (not a placeholder).
        if (i + 1 < n && templateText.at(i + 1) == QLatin1Char('$')) { i += 2; continue; }

        Placeholder p;
        int end = -1;
        if (i + 1 < n) {
            const QChar nx = templateText.at(i + 1);
            if (nx == QLatin1Char('{'))       end = parseBracedVar(templateText, i, &p);
            else if (nx == QLatin1Char('('))  end = parsePromptVar(templateText, i, &p);
            else                              end = parseSimpleVar(templateText, i, &p);
        }
        if (end < 0) { ++i; continue; }

        if (!indexByName.contains(p.name)) {
            indexByName.insert(p.name, result.size());
            result.append(p);
        }
        i = end;
    }
    return result;
}

QString AutoFillEngine::render(const QString& templateText,
                               const QMap<QString, QString>& values) {
    const auto placeholders = scan(templateText);
    if (placeholders.isEmpty()) {
        // Still need to collapse "$$" → "$" so escape sequence works.
        QString out = templateText;
        out.replace(QStringLiteral("$$"), QStringLiteral("$"));
        return out;
    }
    // Build a single pass: walk the string, replacing each rawToken at the
    // exact position where it was detected. We re-scan the template instead
    // of caching positions in Placeholder because callers may render with
    // an unrelated template (defensive).
    QString out;
    out.reserve(templateText.size());
    const int n = templateText.size();
    int i = 0;
    while (i < n) {
        if (templateText.at(i) != QLatin1Char('$')) {
            out.append(templateText.at(i));
            ++i;
            continue;
        }
        if (i + 1 < n && templateText.at(i + 1) == QLatin1Char('$')) {
            out.append(QLatin1Char('$'));
            i += 2;
            continue;
        }
        Placeholder p;
        int end = -1;
        if (i + 1 < n) {
            const QChar nx = templateText.at(i + 1);
            if (nx == QLatin1Char('{'))       end = parseBracedVar(templateText, i, &p);
            else if (nx == QLatin1Char('('))  end = parsePromptVar(templateText, i, &p);
            else                              end = parseSimpleVar(templateText, i, &p);
        }
        if (end < 0) {
            out.append(templateText.at(i));
            ++i;
            continue;
        }
        const auto it = values.constFind(p.name);
        const QString supplied = (it != values.constEnd()) ? it.value() : QString();
        const QString chosen = !supplied.isEmpty() ? supplied : p.defaultValue;
        out.append(chosen);
        i = end;
    }
    return out;
}

bool AutoFillEngine::isComplete(const QString& templateText,
                                const QMap<QString, QString>& values,
                                QString* missingName) {
    const auto placeholders = scan(templateText);
    for (const auto& p : placeholders) {
        if (!p.required) continue;
        const auto it = values.constFind(p.name);
        const QString v = (it != values.constEnd()) ? it.value() : QString();
        if (v.isEmpty() && p.defaultValue.isEmpty()) {
            if (missingName) *missingName = p.name;
            return false;
        }
    }
    return true;
}

} // namespace dante::autofill
