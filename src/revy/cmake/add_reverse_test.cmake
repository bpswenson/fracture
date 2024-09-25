function(add_reverse_test TARGET_NAME)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
    message(STATUS "Running add_reverse_test for target ${TARGET_NAME}")

    # Step 1: Compile all the source files to LLVM bitcode
    foreach(SRC ${ARGN})
        get_filename_component(FILE_WE ${SRC} NAME_WE)  # Get the filename without extension
        set(BC_FILE "${FILE_WE}.bc")
        
        add_custom_command(
            OUTPUT ${BC_FILE}
            COMMAND clang++ -emit-llvm -c ${SRC} -o ${BC_FILE} 
            DEPENDS ${SRC} reverse_pass
            COMMENT "Compiling ${SRC} to LLVM bitcode (${BC_FILE})"
        )
        list(APPEND BC_FILES ${BC_FILE})
    endforeach()

    # Step 2: Link all the generated bitcode files into one
    set(MERGED_BC "${TARGET_NAME}_merged.bc")
    add_custom_command(
        OUTPUT ${MERGED_BC}
        COMMAND llvm-link ${BC_FILES} -o ${MERGED_BC}
        DEPENDS ${BC_FILES}
        COMMENT "Linking bitcode files into ${MERGED_BC}"
    )

    # Step 3: Apply reverse pass to the merged bitcode file
    set(OPT_BC "${TARGET_NAME}_opt.bc")
    add_custom_command(
        OUTPUT ${OPT_BC}
        COMMAND opt -load-pass-plugin ${REVERSE_PASS_LIB} -passes=reverse-pass ${MERGED_BC} -o ${OPT_BC}
        DEPENDS ${MERGED_BC} ${REVERSE_PASS_LIB}
        COMMENT "Applying reverse pass to ${MERGED_BC} (${OPT_BC})"
    )

    # Step 4: Generate the final executable from the optimized bitcode
    add_custom_command(
        OUTPUT ${TARGET_NAME}
        COMMAND clang++ ${OPT_BC} -o ${TARGET_NAME} -rdynamic
        DEPENDS ${OPT_BC}
        COMMENT "Linking final executable (${TARGET_NAME})"
    )

    # Step 5: Create the custom target for building
    add_custom_target(build_${TARGET_NAME} ALL
        DEPENDS ${TARGET_NAME}
        COMMENT "Building the final ${TARGET_NAME} executable"
    )
endfunction()
