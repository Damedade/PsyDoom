set(SOURCE_FILES
    "Alpha_0_05_Wad_Convert.cpp"
)

set(OTHER_FILES
)

add_executable(${ALPHA_0_05_WAD_CONVERT_TGT_NAME} ${SOURCE_FILES} ${OTHER_FILES})
setup_source_groups("${SOURCE_FILES}" "${OTHER_FILES}")

add_psydoom_common_target_compile_options(${ALPHA_0_05_WAD_CONVERT_TGT_NAME})
target_link_libraries(${ALPHA_0_05_WAD_CONVERT_TGT_NAME} ${BASELIB_TGT_NAME})
