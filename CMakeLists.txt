cmake_minimum_required(VERSION 3.10.2)

FIND_PACKAGE(MPI)
IF ( MPI_FOUND )
  SET(CMAKE_REQUIRED_INCLUDES "${MPI_INCLUDE_PATH};${CMAKE_REQUIRED_INCLUDES}")
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF()
# add_executable(test_mpi test_mpi.cpp)
# message(STATUS "My exec libs ${LINK_LIBRARIES}")
# target_link_libraries(test_mpi ${MPI_LIBRARIES})

include_directories(./include)
file(GLOB SOURCES "./src/*.cpp")

add_executable(CW_mpi ${SOURCES})
target_link_libraries(CW_mpi ${MPI_LIBRARIES})
