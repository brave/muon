#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"

namespace metrics {

DesktopSessionDurationTracker* DesktopSessionDurationTracker::Get() {
  return NULL;
}

void DesktopSessionDurationTracker::AddObserver(Observer* observer) {}
void DesktopSessionDurationTracker::RemoveObserver(Observer* observer) {}

}
