cmake_minimum_required(VERSION 3.5)
project(FSM VERSION 1.0.0)

include(CTest)
enable_testing()

add_subdirectory(StateMachine)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
