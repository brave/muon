#include "chrome/browser/permissions/permission_uma_util.h"

PermissionReportInfo::PermissionReportInfo(
    const PermissionReportInfo& other) = default;

void PermissionUmaUtil::PermissionRevoked(ContentSettingsType permission,
                                          PermissionSourceUI source_ui,
                                          const GURL& revoked_origin,
                                          Profile* profile) {
  return;
}
