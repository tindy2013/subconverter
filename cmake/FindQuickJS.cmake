find_path(QUICKJS_INCLUDE_DIRS quickjs/quickjs.h)

find_library(QUICKJS_LIBRARY quickjs/libquickjs.a)

set(QUICKJS_LIBRARIES "${QUICKJS_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QuickJS DEFAULT_MSG
    QUICKJS_INCLUDE_DIRS QUICKJS_LIBRARY)

mark_as_advanced(QUICKJS_INCLUDE_DIRS QUICKJS_LIBRARY)
