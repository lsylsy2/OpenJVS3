cmake_minimum_required(VERSION 3.10)

project(openjvs)

project(openjvs
        VERSION "3.1.2"
		LANGUAGES "C"
        DESCRIPTION "An emulator for arcade JVS I/O boards"
        HOMEPAGE_URL "https://github.com/bobbydilley/OpenJVS3"
)

find_package(Threads REQUIRED)

configure_file(include/version.h.in version.h)

include_directories(${THREAD_INLCUDE_DIRS} ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})

FILE(GLOB Sources src/*.c) #Assume we want to include everything from src/

add_executable(${PROJECT_NAME}
        ${Sources}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        C_STANDARD 99
)

# target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(${PROJECT_NAME} PRIVATE)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} m)

install(TARGETS ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}
		RUNTIME DESTINATION "bin/"
		LIBRARY DESTINATION "lib/"
)


install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs/openjvs
		COMPONENT ${PROJECT_NAME}
		DESTINATION "/etc/"	
)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "bobby@dilley.uk") #required
INCLUDE(CPack)
