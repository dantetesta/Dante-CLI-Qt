// SPEC-081 — Generator catalog entry.
//
// A generator is a named, categorized command template. The template uses the
// AutoFillEngine placeholder grammar so that picking a generator routes the
// text through AutoFillDialog before execution. Generators are loaded from
// two sources in priority order: the embedded seed JSON shipped with the
// app, plus an optional user file under <AppData>/Dante Testa/Dante CLI/
// generators.json (so power users can extend without editing source).
#pragma once

#include <QString>

namespace dante::generators {

struct Generator {
    QString id;          // unique slug, e.g. "git.commit"
    QString category;    // grouping bucket, e.g. "git"
    QString name;        // short title shown on the card
    QString description; // one-liner under the title
    QString templateText;// command string with placeholders
    QString icon;        // optional unicode glyph
};

} // namespace dante::generators
