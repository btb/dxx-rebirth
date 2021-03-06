/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/* OpenGL extensions:
 * We use global function pointer for any extension function we want to use.
 */

#pragma once

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/* global extension stuff (from glext.h)
 */
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

typedef int64_t GLint64;
typedef uint64_t GLuint64;

/* GL_ARB_sync */
typedef struct __GLsync *GLsync;

typedef GLsync (APIENTRYP PFNGLFENCESYNCPROC) (GLenum condition, GLbitfield flags);
typedef GLboolean (APIENTRYP PFNGLISSYNCPROC) (GLsync sync);
typedef void (APIENTRYP PFNGLDELETESYNCPROC) (GLsync sync);
typedef GLenum (APIENTRYP PFNGLCLIENTWAITSYNCPROC) (GLsync sync, GLbitfield flags, GLuint64 timeout);
typedef void (APIENTRYP PFNGLWAITSYNCPROC) (GLsync sync, GLbitfield flags, GLuint64 timeout);

#define GL_SYNC_FLUSH_COMMANDS_BIT        0x00000001
#define GL_SYNC_GPU_COMMANDS_COMPLETE     0x9117
#define GL_TIMEOUT_EXPIRED                0x911B

extern bool ogl_have_ARB_sync;
extern PFNGLFENCESYNCPROC glFenceSyncFunc;
extern PFNGLDELETESYNCPROC glDeleteSyncFunc;
extern PFNGLCLIENTWAITSYNCPROC glClientWaitSyncFunc;

/* Global initialization:
 * will need an OpenGL context and intialize all function pointers.
 */
void ogl_extensions_init();

