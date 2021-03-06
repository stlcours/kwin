/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright 2017 Roman Gilg <subdiff@gmail.com>
Copyright 2015 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "drm_backend.h"
#include "drm_buffer_gbm.h"

#include "logging.h"

// system
#include <sys/mman.h>
#include <errno.h>
// drm
#include <xf86drm.h>
#include <gbm.h>

namespace KWin
{

// DrmSurfaceBuffer
DrmSurfaceBuffer::DrmSurfaceBuffer(DrmBackend *backend, gbm_surface *surface)
    : DrmBuffer(backend)
    , m_surface(surface)
{
    m_bo = gbm_surface_lock_front_buffer(surface);
    if (!m_bo) {
        qCWarning(KWIN_DRM) << "Locking front buffer failed";
        return;
    }
    m_size = QSize(gbm_bo_get_width(m_bo), gbm_bo_get_height(m_bo));
    if (drmModeAddFB(m_backend->fd(), m_size.width(), m_size.height(), 24, 32, gbm_bo_get_stride(m_bo), gbm_bo_get_handle(m_bo).u32, &m_bufferId) != 0) {
        qCWarning(KWIN_DRM) << "drmModeAddFB failed";
    }
    gbm_bo_set_user_data(m_bo, this, nullptr);
}

DrmSurfaceBuffer::~DrmSurfaceBuffer()
{
    if (m_bufferId) {
        drmModeRmFB(m_backend->fd(), m_bufferId);
    }
    releaseGbm();
}

void DrmSurfaceBuffer::releaseGbm()
{
    if (m_bo) {
        gbm_surface_release_buffer(m_surface, m_bo);
        m_bo = nullptr;
    }
}

}
