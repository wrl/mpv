AC_DEFUN([OS_WINDOWS_CHECKS],[
  AC_MSG_CHECKING([if building for Windows or Cygwin])
  AM_CONDITIONAL([OS_WINDOWS], [test "x$windows" = "xyes"])
  AM_COND_IF([OS_WINDOWS],[],[windows=no])
  AC_MSG_RESULT([$windows])

  AM_COND_IF([OS_WINDOWS],[
    dnl ao_wasapi uses COM to load symbols, only needs -lole32 and headers
    AX_CC_CHECK_LIBS([-lole32],[WASAPI],[WASAPI],[
#define COBJMACROS 1
#define _WIN32_WINNT 0x600
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>

int main(void) {
    const GUID *check[[]] = {
      &IID_IAudioClient,
      &IID_IAudioRenderClient,
      &IID_IAudioEndpointVolume,
    };
    (void)check[[0]];

    CoInitialize(NULL);
    IMMDeviceEnumerator *e;
    CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                     &IID_IMMDeviceEnumerator, (void **)&e);
    IMMDeviceEnumerator_Release(e);
    CoUninitialize();
}
    ])
    AM_COND_IF([HAVE_WASAPI],[AC_DEFINE([CONFIG_WASAPI],[1],[Define to 1 if WASAPI is enabled (compat with old build system)])])

    dnl ao_dsound uses LoadLibrary to load symbols, only needs the header
    AX_CHECK_STATEMENT([DSOUND],[DirectSound],[dsound.h])
    AM_COND_IF([HAVE_DSOUND],[AC_DEFINE([CONFIG_DSOUND],[1],[Define to 1 if DirectSound is enabled (compat)])])

    AX_CC_CHECK_LIBS(["-lopengl32 -lgdi32"],[OPENGL],[OpenGL],[
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>

int main(void) {
  HDC dc;
  wglCreateContext(dc);
  glFinish();
  return !GL_INVALID_FRAMEBUFFER_OPERATION;
}
    ])

    dnl vo_direct3d uses LoadLibrary to load symbols, only needs the header
    AX_CHECK_STATEMENT([DIRECT3D],[Direct3D 9],[d3d9.h])\
    AM_COND_IF([HAVE_DIRECT3D],
      [AC_DEFINE([CONFIG_DIRECT3D],[1],[Define to 1 if Direct3D 9 is enabled (compat)])])
  ])

  AM_CONDITIONAL([HAVE_WASAPI],[test "x$have_wasapi" = "xyes"])
  AM_CONDITIONAL([HAVE_DSOUND],[test "x$have_dsound" = "xyes"])
  AM_CONDITIONAL([HAVE_DIRECT3D],[test "x$have_direct3d" = "xyes"])

  AM_CONDITIONAL([HAVE_OPENGL],[test "x$have_opengl" = "xyes"])
  AM_COND_IF([HAVE_OPENGL],[
    AC_DEFINE([CONFIG_GL], [1], [Define 1 if OpenGL is enabled (compat)])
    AC_DEFINE([CONFIG_GL_WIN32], [1], [Define 1 if OpenGL on Win32 is enabled (compat)])
    AC_DEFINE([HAVE_OPENGL], [1], [Define 1 if OpenGL is enabled])
    AC_DEFINE([HAVE_OPENGL_WIN32], [1], [Define 1 if OpenGL on Win32 is enabled])])
])
