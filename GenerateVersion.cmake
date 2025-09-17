find_package(Git)


if(GIT_EXECUTABLE)

  get_filename_component(WORKING_DIR ${SRC} DIRECTORY)

  execute_process(

    COMMAND ${GIT_EXECUTABLE} describe --always --dirty

    WORKING_DIRECTORY ${WORKING_DIR}

    OUTPUT_VARIABLE BASS_VERSION

    RESULT_VARIABLE ERROR_CODE

    OUTPUT_STRIP_TRAILING_WHITESPACE

    )

endif()


if(BASS_VERSION STREQUAL "")

  set(BASS_VERSION 0.0.0-unknown)

  message(WARNING "Failed to determine version from Git tags. Using default version \"${BASS_VERSION}\".")

endif()


configure_file(${SRC} ${DST} @ONLY)