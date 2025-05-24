find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/doc/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_SOURCE_DIR}/doc/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
#    message(STATUS "Doxygen ${DOXYGEN_VERSION} found, generate docs with `make docs`")
else()
    message(STATUS "Doxygen not found. Documentation will not be built")
endif()
