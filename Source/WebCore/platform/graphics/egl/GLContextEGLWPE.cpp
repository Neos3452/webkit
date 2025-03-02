/*
 * Copyright (C) 2017 Igalia, S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GLContextEGL.h"

#if USE(EGL) && PLATFORM(WPE)

#include "PlatformDisplayWPE.h"
// FIXME: For now default to the GBM EGL platform, but this should really be
// somehow deducible from the build configuration.
#define __GBM__ 1
#include <EGL/egl.h>
#include <wpe/renderer-backend-egl.h>

namespace WebCore {

GLContextEGL::GLContextEGL(PlatformDisplay& display, EGLContext context, EGLSurface surface, struct wpe_renderer_backend_egl_offscreen_target* target)
    : GLContext(display)
    , m_context(context)
    , m_surface(surface)
    , m_type(WindowSurface)
    , m_wpeTarget(target)
{
}

EGLSurface GLContextEGL::createWindowSurfaceWPE(EGLDisplay display, EGLConfig config, GLNativeWindowType window)
{
    return eglCreateWindowSurface(display, config, reinterpret_cast<EGLNativeWindowType>(window), nullptr);
}

std::unique_ptr<GLContextEGL> GLContextEGL::createWPEContext(PlatformDisplay& platformDisplay, EGLContext sharingContext)
{
    EGLDisplay display = platformDisplay.eglDisplay();
    EGLConfig config;
    if (!getEGLConfig(display, &config, WindowSurface))
        return nullptr;

    static const EGLint contextAttributes[] = {
#if USE(OPENGL_ES_2)
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, sharingContext, contextAttributes);
    if (context == EGL_NO_CONTEXT)
        return nullptr;

    auto* target = wpe_renderer_backend_egl_offscreen_target_create();
    wpe_renderer_backend_egl_offscreen_target_initialize(target, downcast<PlatformDisplayWPE>(platformDisplay).backend());
    EGLNativeWindowType window = wpe_renderer_backend_egl_offscreen_target_get_native_window(target);
    if (!window) {
        wpe_renderer_backend_egl_offscreen_target_destroy(target);
        return nullptr;
    }

    EGLSurface surface = eglCreateWindowSurface(display, config, static_cast<EGLNativeWindowType>(window), nullptr);
    if (surface == EGL_NO_SURFACE) {
        eglDestroyContext(display, context);
        wpe_renderer_backend_egl_offscreen_target_destroy(target);
        return nullptr;
    }

    return std::unique_ptr<GLContextEGL>(new GLContextEGL(platformDisplay, context, surface, target));
}

void GLContextEGL::destroyWPETarget()
{
    if (m_wpeTarget)
        wpe_renderer_backend_egl_offscreen_target_destroy(m_wpeTarget);
}

} // namespace WebCore

#endif // USE(EGL) && PLATFORM(WPE)
