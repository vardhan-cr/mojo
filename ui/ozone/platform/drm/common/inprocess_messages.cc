#include "ui/ozone/platform/drm/common/inprocess_messages.h"

namespace ui {

OzoneHostMsg_UpdateNativeDisplays::OzoneHostMsg_UpdateNativeDisplays(
  const std::vector<DisplaySnapshot_Params>& _displays)
  : Message(OZONE_HOST_MSG__UPDATE_NATIVE_DISPLAYS),
    displays(_displays) {
}

OzoneHostMsg_UpdateNativeDisplays::~OzoneHostMsg_UpdateNativeDisplays() {
}

} // namespace
