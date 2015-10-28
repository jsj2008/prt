AC_INIT([prt], [0.1.0])
AC_CONFIG_SRCDIR([src/shared/basic.c])
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4]) 

LT_INIT

GL_REQUIRED_VERSION=10.6.9
SDL2_REQUIRED_VERSION=2.0.3
ASSIMP_REQUIRED_VERSION=3.0.0

PKG_CHECK_MODULES(gl, gl >= $GL_REQUIRED_VERSION)
AC_SUBST(gl_CFLAGS)
AC_SUBST(gl_LIBS)

PKG_CHECK_MODULES(sdl2, sdl2 >= $SDL2_REQUIRED_VERSION)
AC_SUBST(sdl2_CFLAGS)
AC_SUBST(sdl2_LIBS)

PKG_CONFIG_PATH=/usr/lib64
PKG_CHECK_MODULES(assimp, assimp >= $ASSIMP_REQUIRED_VERSION)
AC_SUBST(assimp_CFLAGS)
AC_SUBST(assimp_LIBS)

AC_CHECK_HEADERS([GLES3/gl31.h], [], AC_MSG_ERROR([*** OpenGL ES 3.1 headers not found]))

AC_PROG_CXX

# Neither one works
#AC_CHECK_HEADERS([glm/glm.hpp], [], AC_MSG_ERROR([*** GLM headers not found]))
#AC_CHECK_HEADERS([/usr/include/glm/glm.hpp], [], AC_MSG_ERROR([*** GLM headers not found]))

AC_ARG_ENABLE([examples],
              [--enable-examples    Build code examples],
              [case "${enableval}" in
                 yes) examples=true ;;
                 no)  examples=false ;;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-examples]) ;;
              esac], [examples=false])
AM_CONDITIONAL([EXAMPLES], [test x$examples = xtrue])
AM_COND_IF([EXAMPLES], [AC_CONFIG_FILES(examples/Makefile)])

AC_OUTPUT(Makefile prt/Makefile tests/Makefile)
