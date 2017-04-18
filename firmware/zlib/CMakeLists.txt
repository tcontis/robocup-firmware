if(${COMPILE_MBED})
# set the arm toolchain and include it so we can use it
set(CMAKE_TOOLCHAIN_FILE ${ARM_TOOLCHAIN_FILE})
include(${ARM_TOOLCHAIN_FILE})

# Set compiler and linker flags
set(CMAKE_CXX_FLAGS         ${MBED_CMAKE_CXX_FLAGS}         )
set(CMAKE_C_FLAGS           ${MBED_CMAKE_C_FLAGS}           )
set(CMAKE_EXE_LINKER_FLAGS  ${MBED_CMAKE_EXE_LINKER_FLAGS}  )
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# positon-independent-code flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

file(GLOB ZLIB_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB ZLIB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(zlib STATIC ${ZLIB_SRC} ${ZLIB_INCLUDE})