find_path(DUKTAPE_INCLUDE_DIRS duktape.h)

find_library(DUKTAPE_LIBRARY duktape)
find_library(DUKTAPE_MODULE_LIBRARY duktape_module)

set(DUKTAPE_LIBRARIES "${DUKTAPE_LIBRARY}" "${DUKTAPE_MODULE_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Duktape DEFAULT_MSG
    DUKTAPE_INCLUDE_DIRS DUKTAPE_LIBRARY DUKTAPE_MODULE_LIBRARY)

mark_as_advanced(DUKTAPE_INCLUDE_DIRS DUKTAPE_LIBRARY DUKTAPE_MODULE_LIBRARY)
