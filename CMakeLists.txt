project(intercharts)

add_library(${PROJECT_NAME} INTERFACE)

# TODO: create a ls-cmake function for interface libraries

set(sources chart.cpp)
set(headers)
set(public_headers chart.h)

foreach(src ${sources})
    set(src_fp "${PROJECT_SOURCE_DIR}/src/${src}")
    list(APPEND hdrs_fp "${src_fp}")
    target_sources(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${src_fp}>
        $<INSTALL_INTERFACE:src/${PROJECT_NAME}/${src}>
    )
endforeach()

foreach(hdr ${headers})
    set(hdr_fp "${PROJECT_SOURCE_DIR}/src/${hdr}")
    list(APPEND hdrs_fp "${hdr_fp}")
    target_sources(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${hdr_fp}>
        $<INSTALL_INTERFACE:src/${PROJECT_NAME}/${hdr}>
    )
endforeach()

foreach(pub_hdr ${public_headers})
    set(pubhdr_fp "${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/${pub_hdr}")
    list(APPEND pubhdrs_fp "${hdr_fp}")
    target_sources(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${pubhdr_fp}>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}/${pub_hdr}>
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "include/${PROJECT_NAME}/${pub_hdr}")
endforeach()

target_include_directories(${PROJECT_NAME}
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    INTERFACE "$<INSTALL_INTERFACE:include/>"
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
    PUBLIC_HEADER DESTINATION "include/${PROJECT_NAME}/"
)

cmake_policy(SET CMP0022 NEW)
install(EXPORT ${PROJECT_NAME}
    DESTINATION "cmake"
    FILE "${PROJECT_NAME}.cmake"
    EXPORT_LINK_INTERFACE_LIBRARIES
)

install(FILES ${src_fp} ${hdr_fp} DESTINATION "src/${PROJECT_NAME}/")

# Generate package configuration file (adapted from LSModule.cmake)

set(required_pkgs)
foreach(pkg ${required_pkgs})
    set(FIND_DEPENDENCIES "${FIND_DEPENDENCIES}\nfind_dependency(${pkg})")
endforeach()
set(EXPORT_SET ${PROJECT_NAME})
configure_file("${PROJECT_SOURCE_DIR}/config.in.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" DESTINATION "cmake/")
