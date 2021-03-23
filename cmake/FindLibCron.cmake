find_path(LIBCRON_INCLUDE_DIR libcron/Cron.h)
find_path(DATE_INCLUDE_DIR date/date.h)

find_library(LIBCRON_LIBRARY libcron)

set(LIBCRON_LIBRARIES "${LIBCRON_LIBRARY}")
set(LIBCRON_INCLUDE_DIRS "${LIBCRON_INCLUDE_DIR} ${DATE_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibCron DEFAULT_MSG
    LIBCRON_INCLUDE_DIRS LIBCRON_LIBRARY)

mark_as_advanced(LIBCRON_INCLUDE_DIRS)
