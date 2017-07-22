INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_WIMAX wimax)

FIND_PATH(
    WIMAX_INCLUDE_DIRS
    NAMES wimax/api.h
    HINTS $ENV{WIMAX_DIR}/include
        ${PC_WIMAX_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    WIMAX_LIBRARIES
    NAMES gnuradio-wimax
    HINTS $ENV{WIMAX_DIR}/lib
        ${PC_WIMAX_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WIMAX DEFAULT_MSG WIMAX_LIBRARIES WIMAX_INCLUDE_DIRS)
MARK_AS_ADVANCED(WIMAX_LIBRARIES WIMAX_INCLUDE_DIRS)

