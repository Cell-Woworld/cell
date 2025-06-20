cmake_minimum_required(VERSION 3.5)
project(mtDNA VERSION 1.0.0)

#########################################
## module version
########################################
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(git_version)
set(GIT_HASH "")
get_git_hash(GIT_HASH)
#message(STATUS "Git hash is ${GIT_HASH}")
set(GIT_BRANCH "")
get_git_branch(GIT_BRANCH)
#message(STATUS "Git branch is ${GIT_BRANCH}")
file(STRINGS VERSION MODULE_VERSION REGEX "[0-9]+\\.[0-9]+\\.[0-9]+")
string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" MODULE_VERSION_MAJOR "${MODULE_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" MODULE_VERSION_MINOR "${MODULE_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" MODULE_VERSION_BUILD "${MODULE_VERSION}")
message(STATUS "${PROJECT_NAME} version: ${MODULE_VERSION}.${GIT_BRANCH}_${GIT_HASH}")

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/src/CSpin_mutex.cpp" OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/src/Scope_guard.cpp")
	message(FATAL_ERROR "\"CSpin_mutex.cpp\" and \"Scope_guard.cpp\" are required in \"${CMAKE_CURRENT_SOURCE_DIR}/src/lib/src/\"")
endif()

if(WIN32)
	set(CMAKE_CXX_FLAGS "-std:c++17 -EHsc \
	-D VERSION_MAJOR=${MODULE_VERSION_MAJOR} -D VERSION_MINOR=${MODULE_VERSION_MINOR} \
	-D VERSION_BUILD=${MODULE_VERSION_BUILD} -D VERSION_REVISION=\\\"${GIT_BRANCH}_${GIT_HASH}\\\" ")
	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -MD")
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -D_DEBUG -MDd")
	include_directories(../inc/)
	file(GLOB_RECURSE ${PROJECT_NAME}_SRC "src/*.c" "src/*.cpp" "proto/*.cc")
	list(FILTER ${PROJECT_NAME}_SRC EXCLUDE REGEX ".*comparison")
	list(FILTER ${PROJECT_NAME}_SRC EXCLUDE REGEX ".*tutorial")
	IF(BUILD_SHARED_LIBS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D API_PROVIDER")
		add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SRC})
		SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND xcopy /y $<CONFIG>\\${PROJECT_NAME}.dll ..\\..\\bin\\)
	elseif(BUILD_STATIC_LIBS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D STATIC_API")
		add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRC})
		SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND xcopy /y ${PROJECT_NAME}.lib ..\\..\\lib\\)
	endif()
elseif(APPLE)
	set(CMAKE_CXX_FLAGS "-std=c++17 -g \
	-D VERSION_MAJOR=${MODULE_VERSION_MAJOR} -D VERSION_MINOR=${MODULE_VERSION_MINOR} \
	-D VERSION_BUILD=${MODULE_VERSION_BUILD} -D VERSION_REVISION=\\\"${GIT_BRANCH}_${GIT_HASH}\\\" ")
	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -D_DEBUG -O0")
	include_directories(/usr/include/ /usr/local/include/ ../inc/)
	file(GLOB_RECURSE ${PROJECT_NAME}_SRC "src/*.c" "src/*.cpp" "proto/*.cc")
	list(FILTER ${PROJECT_NAME}_SRC EXCLUDE REGEX ".*comparison")
	list(FILTER ${PROJECT_NAME}_SRC EXCLUDE REGEX ".*tutorial")
	IF(BUILD_SHARED_LIBS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D API_PROVIDER -fPIC")
		add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SRC})
		SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "lib")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND cp -f $<CONFIG>/lib${PROJECT_NAME}.dylib ../../bin/)
	endif()
	if(BUILD_STATIC_LIBS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D STATIC_API")
		add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRC})
		SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "lib")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND cp -f ${CMAKE_BUILD_TYPE}-${SDK_NAME}/lib${PROJECT_NAME}.a ../../lib/)
	endif()
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "-std=c++17 -fopenmp -g \
	-D VERSION_MAJOR=${MODULE_VERSION_MAJOR} -D VERSION_MINOR=${MODULE_VERSION_MINOR} \
	-D VERSION_BUILD=${MODULE_VERSION_BUILD} -D VERSION_REVISION=\\\"${GIT_BRANCH}_${GIT_HASH}\\\" ")
	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -D_DEBUG -O0")
	include_directories(/usr/include/ /usr/local/include/ ../inc/)
	file(GLOB_RECURSE ${PROJECT_NAME}_SRC "src/*.c" "src/*.cpp" "proto/*.cc")
	list(FILTER ${PROJECT_NAME}_SRC EXCLUDE REGEX ".*comparison")
	list(FILTER ${PROJECT_NAME}_SRC EXCLUDE REGEX ".*tutorial")
	IF(BUILD_SHARED_LIBS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D API_PROVIDER -fPIC")
		add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SRC})
		SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "lib")
		if(ANDROID)
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND copy /y lib${PROJECT_NAME}.so ..\\..\\bin\\Android\\${CMAKE_ANDROID_ARCH_ABI}\\${CMAKE_BUILD_TYPE}\\)
		else()
			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND cp -f lib${PROJECT_NAME}.so ../../bin/)
		endif()
	endif()
	if(BUILD_STATIC_LIBS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D STATIC_API")
		add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRC})
		SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "lib")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND cp -f lib${PROJECT_NAME}.a ../../lib/)
	endif()
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
