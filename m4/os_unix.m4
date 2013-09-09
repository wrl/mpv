AC_DEFUN([OS_UNIX_CHECKS],[
  AC_MSG_CHECKING([if building for a Unix-like system])
  AM_CONDITIONAL([OS_UNIX], [test "x$os_darwin" = "xno" -a "x$windows" = "xno"])
  AS_IF([test "x$windows" = "xyes"],
      [AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#ifndef __CYGWIN__
#error Not Cygwin
#endif
      ])],[AM_CONDITIONAL([OS_UNIX], [true])])]
    )
  AM_COND_IF([OS_UNIX],[os_unix=yes],[os_unix=no])
  AC_MSG_RESULT([$os_unix])

  AM_COND_IF([OS_UNIX],[
    AC_CHECK_HEADERS([sys/soundcard.h soundcard.h], [have_oss=yes; break])
    AS_IF([test "x$have_oss" = "xyes"],[
      AC_MSG_CHECKING([OSS characteristics])

      AC_DEFINE([HAVE_OSS_AUDIO],[1],[Define to 1 if using OSS Audio])

      AC_DEFINE([PATH_DEV_MIXER],["/dev/mixer"],[Define to the path to the OSS mixer device])

      # NetBSD and OpenBSD have a weird OSS
      AS_IF([test -c "/dev/sound"],[
        AC_DEFINE([PATH_DEV_DSP],["/dev/sound"],[Define to the path to the OSS DSP device])
        LIBS="$LIBS -lossaudio"
      ],[
        AC_DEFINE([PATH_DEV_DSP],["/dev/dsp"],[Define to the path to the OSS DSP device])
      ])

      # Check for oss.conf on Linux (OSSv4 installation)
      AS_IF([test -f "/etc/oss.conf"],[
        . "/etc/oss.conf"
        AS_IF([test -f "$OSSLIBDIR/include/sys/soundcard.h"],
          [CPPFLAGS="$CPPFLAGS -I$OSSLIBDIR/include"])
      ])

      AC_MSG_RESULT([done])
    ])

    AX_PKG_ADD([X11],[x11])

    AX_PKG_ADD([GL], [gl], [build with X11 OpenGL support])
    AM_COND_IF([HAVE_GL], [
      AC_DEFINE([HAVE_OPENGL_X11],[1],[Define to 1 when building with X11 OpenGL support])
    ])

    AX_PKG_ADD([XV],[xv])

    AX_PKG_ADD([WAYLAND], [wayland-client >= 1.2.0 wayland-cursor xkbcommon >= 0.3.0])
    AM_COND_IF([HAVE_GL], [
        AX_PKG_ADD([WAYLAND_OPENGL], [wayland-egl >= 9.0.0 egl >= 9.0.0])
      AC_DEFINE([HAVE_OPENGL_WAYLAND],[1],[Define to 1 when building with X11 OpenGL support])
    ])

  ])
  AM_CONDITIONAL([HAVE_OSS_AUDIO], [test "x$have_oss" = "xyes"])
  AM_CONDITIONAL([HAVE_X11],       [test "x$with_x11" = "xyes"])
  AM_CONDITIONAL([HAVE_OPENGL_X11],[test "x$with_gl"  = "xyes"])
  AM_CONDITIONAL([HAVE_XV],        [test "x$with_xv"  = "xyes"])
  AM_CONDITIONAL([HAVE_WAYLAND],   [test "x$with_wayland" = "xyes"])
  AM_CONDITIONAL([HAVE_OPENGL_WAYLAND], [test "x$with_gl" = "xyes"])
  AM_COND_IF([HAVE_OPENGL_X11],[AM_CONDITIONAL([HAVE_OPENGL],[true])])
  AM_COND_IF([HAVE_OPENGL_WAYLAND],[AM_CONDITIONAL([HAVE_OPENGL],[true])])
])
