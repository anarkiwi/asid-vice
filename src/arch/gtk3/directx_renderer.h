/**
 * \file directx_renderer.h
 * \brief   DirectX-based renderer for the GTK3 backend.
 *
 * \author David Hogan <david.q.hogan@gmail.com>
 */

/* This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */
#ifndef VICE_DIRECTX_RENDERER_H
#define VICE_DIRECTX_RENDERER_H

#include "videoarch.h"

#ifdef WIN32_COMPILE

/** \brief A renderer that uses DirectX to render to a something or whatever.
 *
 * Because OpenGL + GTK3 just doesn't work that well on windows, this.
 */
extern vice_renderer_backend_t vice_directx_backend;

#endif /* #ifdef WIN32_COMPILE */

#endif /* #ifndef VICE_DIRECTX_RENDERER_H */
