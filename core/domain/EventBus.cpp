#include "EventBus.h"

namespace dante::domain {

EventBus& EventBus::instance() {
    static EventBus bus;
    return bus;
}

} // namespace dante::domain
