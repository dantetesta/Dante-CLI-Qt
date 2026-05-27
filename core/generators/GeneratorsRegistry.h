// SPEC-081 — Registry of command generators.
//
// Loaded once at construction. The seed catalog is hardcoded in the .cpp so
// no QRC plumbing is required (the JSON would round-trip to the same data
// anyway). On top of the seed, the registry reads an optional user file:
//
//   <AppData>/Dante Testa/Dante CLI/generators.json
//
// where the JSON is an array of objects matching Generator fields. User
// entries OVERRIDE seed entries with the same id, so users can tweak ours
// without losing the rest.
#pragma once

#include "Generator.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace dante::generators {

class GeneratorsRegistry {
public:
    GeneratorsRegistry();

    QVector<Generator> all() const { return items_; }
    QVector<Generator> byCategory(const QString& category) const;
    QStringList        categories() const;
    Generator          findById(const QString& id) const;

    // Forces a reload (useful when the user file has changed). Not wired to
    // a watcher today — power users restart the app, which is fine.
    void reload();

private:
    void loadSeed();
    void loadUserOverrides();

    QVector<Generator> items_;
};

} // namespace dante::generators
