# Builds the Doxygen HTML documentation from the repository's Doxyfile.
# Same result as running `doxygen` in the repository root.
#
#   cmake -S docs -B build-docs/cmake
#   cmake --build build-docs/cmake
#
# Output: build-docs/html/index.html

cmake_minimum_required(VERSION 3.20)
project(rpm_docs NONE)

find_package(Doxygen REQUIRED OPTIONAL_COMPONENTS dot)
if(NOT TARGET Doxygen::dot)
    message(WARNING "Graphviz (dot) not found: graphs will be missing from the documentation.")
endif()

get_filename_component(RPM_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

add_custom_target(docs ALL
    COMMAND Doxygen::doxygen "${RPM_REPO_ROOT}/Doxyfile"
    WORKING_DIRECTORY "${RPM_REPO_ROOT}"
    COMMENT "Generating HTML documentation with Doxygen"
    VERBATIM
)
