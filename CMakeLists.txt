cmake_minimum_required(VERSION 2.8)
project(IRAD)
# WK
IF(WIN32)
     include (GenerateExportHeader)
ENDIF()
#

OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
# Set the LIB_TYPE variable to STATIC
SET (LIB_TYPE STATIC)
IF (BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  SET (LIB_TYPE SHARED)
ENDIF (BUILD_SHARED_LIBS)

# Compiled objects output paths
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single directory for all executables.")
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single directory for all libraries and archives.")
mark_as_advanced (LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
# Set up RPATH
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
INCLUDE(CTest)
IF(BUILD_TESTING)
  SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
  MARK_AS_ADVANCED(BUILDNAME)
ENDIF(BUILD_TESTING)

# Project-specific constructs
include_directories(include ${CMAKE_BINARY_DIR}/include )
find_package(MPI REQUIRED)

set (LIB_SOURCES src/ComLine.C src/Parameters.C src/Configuration.C src/Profiler.C src/UnixUtils.C)
# WK src/RunTest.C src/TestTCPInterface.C omitted
IF(WIN32)
    set(UTIL_SOURCES src/TestUtil.C src/TestResults.C src/CheckResults.C src/profane.C src/DiffDataFiles.C)
ELSE()
    set(UTIL_SOURCES src/TestUtil.C src/TestResults.C src/CheckResults.C src/profane.C src/DiffDataFiles.C src/RunTest.C)
ENDIF()
#

# Original part of IRAD MPI config
IF(MPI_CXX_COMPILER)
#  set (CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
  set (LIB_SOURCES ${LIB_SOURCES} src/COMM.C)
  IF(MPI_COMPILE_FLAGS)
    set_source_files_properties(${LIB_SOURCES} PROPERTIES COMPILE_FLAGS "${MPI_CXX_COMPILE_FLAGS}")
    set_source_files_properties(${UTIL_SOURCES} PROPERTIES COMPILE_FLAGS "${MPI_CXX_COMPILE_FLAGS}")
  ENDIF()
  add_definitions( -D_IRAD_MPI_ )
  include_directories(${MPI_INCLUDE_PATH})
ENDIF()
IF(APPLE)
  add_definitions(-DDARWIN)
ENDIF()

IF(NOT ENABLE_MPI)
  add_definitions( -DDUMMY_MPI )
ELSE()
  FIND_PACKAGE(MPI REQUIRED)
#  IF(MPI_C_FOUND)
#     add_definitions( -DMPICH_IGNORE_CXX_SEEK )
#     INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
#  ELSE()
     IF(WIN32)
         add_definitions( -DMPICH_IGNORE_CXX_SEEK )
         SET(MPI_C_INCLUDE_PATH MSMPI_INC)
         SET(MPI_CXX_INCLUDE_PATH MSMPI_INC)
         SET(MPI_Fortran_INCLUDE_PATH MSMPI_INC)
         SET(MPI_C_LIBRARIES MSMPI_LIB64)
         SET(MPI_CXX_LIBRARIES MSMPI_LIB64)
         SET(MPI_Fortran_LIBRARIES MSMPI_LIB64)
     ENDIF()     
#  ENDIF()
ENDIF()



FILE(GLOB INC_FILES include/*.H)

add_library(IRAD ${LIB_TYPE} ${LIB_SOURCES})

GENERATE_EXPORT_HEADER( IRAD
             BASE_NAME IRAD
             EXPORT_MACRO_NAME IRAD_EXPORTS
             EXPORT_FILE_NAME IRAD_Exports.h
             STATIC_DEFINE IRAD_EXPORTS_BUILT_AS_STATIC
)

TARGET_LINK_LIBRARIES(IRAD ${MPI_CXX_LIBRARIES})
add_executable(iradutil_test src/TestUtil.C)
# WK
IF(NOT WIN32)
add_executable(tcpinterface_test src/TestTCPInterface.C)
ENDIF()
#
add_executable(testresults src/TestResults.C)


add_executable(checkresults src/CheckResults.C)


# WK
IF(NOT WIN32)
add_executable(runtest src/RunTest.C)
ENDIF()
#
add_executable(profane src/profane.C)
add_executable(diffdatafiles src/DiffDataFiles.C)
IF(MPI_LINK_FLAGS)
#WK runtest tcpinterface_test omitted
  IF(WIN32)
  SET_TARGET_PROPERTIES(IRAD iradutil_test testresults checkresults profane diffdatafiles PROPERTIES LINK_FLAGS "${MPI_CXX_LINK_FLAGS}")
  ELSE()
  SET_TARGET_PROPERTIES(IRAD iradutil_test runtest tcpinterface_test testresults checkresults profane diffdatafiles PROPERTIES LINK_FLAGS "${MPI_CXX_LINK_FLAGS}")
  ENDIF()
ENDIF()
#

target_link_libraries(profane IRAD ${MPI_CXX_LIBRARIES})
# WK
IF(NOT WIN32)
 target_link_libraries(runtest IRAD ${MPI_CXX_LIBRARIES})
ENDIF()
#
target_link_libraries(iradutil_test IRAD ${MPI_CXX_LIBRARIES})


target_link_libraries(checkresults IRAD ${MPI_CXX_LIBRARIES})


# WK
IF(NOT WIN32)
target_link_libraries(tcpinterface_test IRAD ${MPI_CXX_LIBRARIES})
ENDIF()
#
target_link_libraries(diffdatafiles IRAD ${MPI_CXX_LIBRARIES})

ADD_TEST(IRAD::RunUtilTests ${EXECUTABLE_OUTPUT_PATH}/iradutil_test iradutil_testresults.txt)
ADD_TEST(IRAD::GetNextContent:CommentsAndWhiteSpace ${EXECUTABLE_OUTPUT_PATH}/testresults GetNextContent:CommentsAndWhiteSpace iradutil_testresults.txt)
ADD_TEST(IRAD::GetNextContent:StreamObject ${EXECUTABLE_OUTPUT_PATH}/testresults GetNextContent:StreamObject iradutil_testresults.txt)


INSTALL(FILES ${INC_FILES} DESTINATION include)
# WK runtest omitted
IF(WIN32)
INSTALL(TARGETS IRAD checkresults testresults profane diffdatafiles RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
ELSE()
INSTALL(TARGETS IRAD checkresults testresults runtest profane diffdatafiles RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
ENDIF()

INSTALL(TARGETS IRAD checkresults testresults profane diffdatafiles RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
#

# Add target to build the documentation
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" OFF)
IF(BUILD_DOCUMENTATION)

  FIND_PACKAGE(Doxygen)
  IF(NOT DOXYGEN_FOUND)
    MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
  ENDIF()
  
  SET( doxyfile_in          ${CMAKE_CURRENT_SOURCE_DIR}/doc/IRAD.cfg.in )
  SET( doxyfile             ${CMAKE_CURRENT_BINARY_DIR}/IRAD.cfg )
  SET( documentation        ${CMAKE_CURRENT_BINARY_DIR}/docs )

  CONFIGURE_FILE( ${doxyfile_in} ${doxyfile} @ONLY )

  ADD_CUSTOM_COMMAND( OUTPUT ${documentation}
                      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                      MAIN_DEPENDENCY ${doxyfile} ${doxyfile_in}
                      DEPENDS IRAD ${doxyfile} ${doxy_extra_files}
                      COMMENT "Generating documentation")

  ADD_CUSTOM_TARGET( docs DEPENDS ${documentation} )
  INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc DESTINATION share/IRAD )
ENDIF()
