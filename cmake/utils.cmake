function(add_main_targets)
    # Parse function arguments
    set(LIBRARIES ${ARGN})
    add_targets("${CMAKE_CURRENT_LIST_DIR}/*.m.cpp" ${LIBRARIES})
    message(STATUS "Created targets: ${ADDED_TARGETS}")
endfunction(add_main_targets)


function(add_test_targets)
    # Parse function arguments
    set(LIBRARIES ${ARGN} gtest::gtest)
    add_targets("${CMAKE_CURRENT_LIST_DIR}/*.g.cpp" ${LIBRARIES})
    
    foreach(TARGET ${TARGETS})
        add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
    endforeach()

    message(STATUS "Created test targets: ${ADDED_TARGETS}")
endfunction(add_test_targets)


function(add_benchmark_targets)
    # Parse function arguments
    set(LIBRARIES ${ARGN} benchmark::benchmark_main)
    add_targets("${CMAKE_CURRENT_LIST_DIR}/*.b.cpp" ${LIBRARIES})
    message(STATUS "Created benchmark targets: ${ADDED_TARGETS}")
endfunction(add_benchmark_targets)

function(add_targets)
    set(NAME_TEMPLATE ${ARGV0})
    list(REMOVE_AT ARGV 0)
    set(LIBRARIES ${ARGV})

    set(CREATED_TARGETS)

    file(GLOB TARGETS ${NAME_TEMPLATE})
    file(GLOB CPP_FILES "*.cpp")
    # Exclude files with postfix ".m.cpp" or ".g.cpp"
    list(FILTER CPP_FILES EXCLUDE REGEX "\\..\\.cpp$")

    foreach(TARGET ${TARGETS})
        get_filename_component(FULL_TARGET_NAME ${TARGET} NAME)
        string(REGEX REPLACE "\\.[^.]*$" "" TARGET_NAME ${FULL_TARGET_NAME})

        add_executable(${TARGET_NAME} ${TARGET} ${CPP_FILES})

        if(NOT "${LIBRARIES}" STREQUAL "")
            target_link_libraries(${TARGET_NAME} ${LIBRARIES})
        endif()

        # Add the created target name to the list
        list(APPEND CREATED_TARGETS ${TARGET_NAME})
    endforeach()

    set(ADDED_TARGETS ${CREATED_TARGETS} PARENT_SCOPE)
endfunction(add_targets)
