project(quickjs LANGUAGES C)

file(STRINGS VERSION version)

set(quickjs_src quickjs.c libbf.c libunicode.c libregexp.c cutils.c quickjs-libc.c)
set(quickjs_def CONFIG_VERSION="${version}" _GNU_SOURCE CONFIG_BIGNUM)

add_library(quickjs ${quickjs_src})
target_compile_definitions(quickjs PRIVATE ${quickjs_def} )
set_target_properties(quickjs PROPERTIES PUBLIC_HEADER "quickjs.h;quickjs-libc.h")

add_library(quickjs-dumpleaks ${quickjs_src})
target_compile_definitions(quickjs-dumpleaks PRIVATE ${quickjs_def} DUMP_LEAKS=1)

if(UNIX OR MINGW)
  find_package(Threads)
  target_link_libraries(quickjs ${CMAKE_DL_LIBS} m Threads::Threads)
  target_link_libraries(quickjs-dumpleaks ${CMAKE_DL_LIBS} m Threads::Threads)
endif()

# install
include(GNUInstallDirs)
install(TARGETS quickjs
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/quickjs
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs
        )
