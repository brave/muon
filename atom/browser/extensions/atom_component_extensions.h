/** @file atom_component_extensions.h
 *  @brief FIXME
 *
 *  @author Evey Quirk
 */
#ifndef _ATOM_COMPONENT_EXTENSIONS_H
#define _ATOM_COMPONENT_EXTENSIONS_H

#include "base/files/file_path.h"

bool IsComponentExtension(const base::FilePath& extension_path, int* resource_id);

#endif /* _ATOM_COMPONENT_EXTENSIONS_H */
