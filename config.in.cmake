
# Find dependencies
include(CMakeFindDependencyMacro)
@FIND_DEPENDENCIES@

# Import the targets
include("${CMAKE_CURRENT_LIST_DIR}/@EXPORT_SET@.cmake")