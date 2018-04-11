# set the arm toolchain and include it so we can use it
set(CMAKE_TOOLCHAIN_FILE ${ARM_TOOLCHAIN_FILE})
include(${ARM_TOOLCHAIN_FILE})

# Set compiler and linker flags
set(CMAKE_CXX_FLAGS         ${MBED_CMAKE_CXX_FLAGS}         )
set(CMAKE_C_FLAGS           ${MBED_CMAKE_C_FLAGS}           )
set(CMAKE_EXE_LINKER_FLAGS  ${MBED_CMAKE_EXE_LINKER_FLAGS}  )

file(GLOB_RECURSE firm-lib_SRC
    "drivers/*.cpp"
    "modules/*.cpp"
    "utils/*.cpp"
)

# Find the assembly source files and make sure they're compiled using the C compiler
file(GLOB_RECURSE asm_SRC "*.S")
set_property(SOURCE ${asm_SRC} PROPERTY LANGUAGE C)
list(APPEND firm-lib_SRC ${asm_SRC})

# append all of the accessory library sources
list(APPEND firm-lib_SRC ${MBED_ASSEC_LIBS_SRCS})

# get the rc-fshare lib target
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/robocup-fshare")
# require it be built after we've run mbed setup
add_dependencies(rc-fshare mbed_libraries)

# tell cmake that all of the source files aren't here yet
# set_source_files_properties(
#     ${MBED_ASSEC_LIBS_SRCS} PROPERTIES
#     GENERATED               TRUE
#     DEPENDS                 ${MBED_ASSEC_LIBS_DEPENDS}
# )

# Generage protobuf files using Nanopb
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../external/nanopb/extra")
find_package(Nanopb REQUIRED)
nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/robocup-fshare/protobuf/Packet.proto")
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)

# build the 'firm-lib' library that contains all the MBED stuff needed for
# both the base station and robot firmware
add_library(firm-lib ${firm-lib_SRC} ${PROTO_SRCS})
target_link_libraries(firm-lib rc-fshare)

target_include_directories( firm-lib PUBLIC
    utils/include
    modules/include
    drivers/include
    drivers/avr-isp
    drivers/ball-sense
    drivers/buzzer
    drivers/decawave
    drivers/ds2411
    drivers/fpga
    drivers/kicker-board
    drivers/mcp23017
    drivers/mpu-6050
    drivers/rotarySelector
    drivers/rtos-i2c
    drivers/shared-spi
    drivers/software-spi
    drivers/ws2811
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MBED_ASSEC_LIBS_INCLUDES}
    ${NANOPB_INCLUDE_DIRS}
)

target_include_directories(firm-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# depends on the mbed libraries and all of the accessory libraries
add_dependencies(firm-lib mbed_libraries ${MBED_ASSEC_LIBS_DEPENDS})
target_link_libraries(firm-lib ${MBED_LIBRARY})
set_target_properties(firm-lib PROPERTIES EXCLUDE_FROM_ALL TRUE)
