#include "TerminalScheme.h"

// Currently header-only; .cpp is kept so build systems can compile a TU
// that pulls in the metatype declaration in one place.
namespace dante::themes {

// Force MOC/metatype registration to happen in this TU as well.
static const int kRegisterTerminalScheme =
    qRegisterMetaType<TerminalScheme>("dante::themes::TerminalScheme");

} // namespace dante::themes
