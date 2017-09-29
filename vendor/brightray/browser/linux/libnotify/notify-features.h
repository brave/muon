/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef __NOTIFY_VERSION_H__
#define __NOTIFY_VERSION_H__

/* compile time version
 */
#define NOTIFY_VERSION_MAJOR    (0)
#define NOTIFY_VERSION_MINOR    (7)
#define NOTIFY_VERSION_MICRO    (7)

/* check whether a version equal to or greater than
 * major.minor.micro is present.
 */
#define NOTIFY_CHECK_VERSION(major,minor,micro) \
    (NOTIFY_VERSION_MAJOR > (major) || \
     (NOTIFY_VERSION_MAJOR == (major) && NOTIFY_VERSION_MINOR > (minor)) || \
     (NOTIFY_VERSION_MAJOR == (major) && NOTIFY_VERSION_MINOR == (minor) && \
      NOTIFY_VERSION_MICRO >= (micro)))


#endif /* __NOTIFY_VERSION_H__ */

