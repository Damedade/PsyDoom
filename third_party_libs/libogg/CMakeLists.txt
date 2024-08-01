set(BASE_DIR        "libogg")
set(SRC_DIR         "${BASE_DIR}/src")
set(INCLUDE_DIR     "${BASE_DIR}/include")

set(INCLUDE_PATHS
    ${INCLUDE_DIR}
)

set(EXPORTED_INCLUDE_PATHS
    ${INCLUDE_DIR}
)

set(SOURCE_FILES
    "${INCLUDE_DIR}/ogg/ogg.h"
    "${INCLUDE_DIR}/ogg/os_types.h"
    "${SRC_DIR}/bitwise.c"
    "${SRC_DIR}/crctable.h"
    "${SRC_DIR}/framing.c"
)

set(OTHER_FILES
)

add_library(${LIBOGG_TGT_NAME} ${SOURCE_FILES} ${OTHER_FILES})
setup_source_groups("${SOURCE_FILES}" "${OTHER_FILES}")

add_common_target_compile_options(${LIBOGG_TGT_NAME})
include_directories(${INCLUDE_PATHS})
target_include_directories(${LIBOGG_TGT_NAME} INTERFACE ${EXPORTED_INCLUDE_PATHS})
