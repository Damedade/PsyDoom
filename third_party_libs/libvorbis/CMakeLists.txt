set(BASE_DIR        "libvorbis")
set(SRC_DIR         "${BASE_DIR}/lib")
set(INCLUDE_DIR     "${BASE_DIR}/include")

set(INCLUDE_PATHS
    ${INCLUDE_DIR}
    ${SRC_DIR}
)

set(EXPORTED_INCLUDE_PATHS
    ${INCLUDE_DIR}
)

set(SOURCE_FILES
    "${INCLUDE_DIR}/vorbis/codec.h"
    "${INCLUDE_DIR}/vorbis/vorbisenc.h"
    "${INCLUDE_DIR}/vorbis/vorbisfile.h"
    "${SRC_DIR}/analysis.c"
    "${SRC_DIR}/backends.h"
    "${SRC_DIR}/bitrate.c"
    "${SRC_DIR}/bitrate.h"
    "${SRC_DIR}/block.c"
    "${SRC_DIR}/books/coupled/res_books_51.h"
    "${SRC_DIR}/books/coupled/res_books_stereo.h"
    "${SRC_DIR}/books/floor/floor_books.h"
    "${SRC_DIR}/books/uncoupled/res_books_uncoupled.h"
    "${SRC_DIR}/codebook.c"
    "${SRC_DIR}/codebook.h"
    "${SRC_DIR}/codec_internal.h"
    "${SRC_DIR}/envelope.c"
    "${SRC_DIR}/envelope.h"
    "${SRC_DIR}/floor0.c"
    "${SRC_DIR}/floor1.c"
    "${SRC_DIR}/highlevel.h"
    "${SRC_DIR}/info.c"
    "${SRC_DIR}/lookup.c"
    "${SRC_DIR}/lookup.h"
    "${SRC_DIR}/lookup_data.h"
    "${SRC_DIR}/lpc.c"
    "${SRC_DIR}/lpc.h"
    "${SRC_DIR}/lsp.c"
    "${SRC_DIR}/lsp.h"
    "${SRC_DIR}/mapping0.c"
    "${SRC_DIR}/masking.h"
    "${SRC_DIR}/mdct.c"
    "${SRC_DIR}/mdct.h"
    "${SRC_DIR}/misc.h"
    "${SRC_DIR}/modes/floor_all.h"
    "${SRC_DIR}/modes/psych_11.h"
    "${SRC_DIR}/modes/psych_16.h"
    "${SRC_DIR}/modes/psych_44.h"
    "${SRC_DIR}/modes/psych_8.h"
    "${SRC_DIR}/modes/residue_16.h"
    "${SRC_DIR}/modes/residue_44.h"
    "${SRC_DIR}/modes/residue_44p51.h"
    "${SRC_DIR}/modes/residue_44u.h"
    "${SRC_DIR}/modes/residue_8.h"
    "${SRC_DIR}/modes/setup_11.h"
    "${SRC_DIR}/modes/setup_16.h"
    "${SRC_DIR}/modes/setup_22.h"
    "${SRC_DIR}/modes/setup_32.h"
    "${SRC_DIR}/modes/setup_44.h"
    "${SRC_DIR}/modes/setup_44p51.h"
    "${SRC_DIR}/modes/setup_44u.h"
    "${SRC_DIR}/modes/setup_8.h"
    "${SRC_DIR}/modes/setup_X.h"
    "${SRC_DIR}/os.h"
    "${SRC_DIR}/psy.c"
    "${SRC_DIR}/psy.h"
    "${SRC_DIR}/registry.c"
    "${SRC_DIR}/registry.h"
    "${SRC_DIR}/res0.c"
    "${SRC_DIR}/scales.h"
    "${SRC_DIR}/sharedbook.c"
    "${SRC_DIR}/smallft.c"
    "${SRC_DIR}/smallft.h"
    "${SRC_DIR}/synthesis.c"
    "${SRC_DIR}/vorbisenc.c"
    "${SRC_DIR}/vorbisfile.c"
    "${SRC_DIR}/window.c"
    "${SRC_DIR}/window.h"
)

set(OTHER_FILES
)

add_library(${LIBVORBIS_TGT_NAME} ${SOURCE_FILES} ${OTHER_FILES})
setup_source_groups("${SOURCE_FILES}" "${OTHER_FILES}")
include_directories(${INCLUDE_PATHS})
target_include_directories(${LIBVORBIS_TGT_NAME} INTERFACE ${EXPORTED_INCLUDE_PATHS})
target_link_libraries(${LIBVORBIS_TGT_NAME} ${LIBOGG_TGT_NAME})

add_common_target_compile_options(${LIBVORBIS_TGT_NAME})

if (COMPILER_MSVC)
    # Use a lenient warning level for this as we get unwanted spam otherwise
    target_compile_options(${LIBVORBIS_TGT_NAME} PRIVATE /W1)
endif()
