
# set the AVR toolchain
set(CMAKE_TOOLCHAIN_FILE attiny_toolchain.cmake)
include(attiny_toolchain.cmake)

set(KICKER_VERSION "v3.0")

include_directories(kicker_elf PRIVATE
#	${PROJECT_SOURCE_DIR}/lib/drivers/kicker-board
	include
	include/hal
	include/hal/${AVR_TARGET_ARCH}
	include/hal/${AVR_TARGET_ARCH}/${KICKER_VERSION}
	hal/${AVR_TARGET_ARCH}
	hal/${AVR_TARGET_ARCH}/${KICKER_VERSION}
)

set(CMAKE_C_COMPILER    ${AVR_C_COMPILER})
set(CMAKE_CXX_COMPILER  ${AVR_CXX_COMPILER})

# F_CPU specifies the frequency (in Hz) of the cpu clock so that the delay functions work appropriately
# the ATTiny13 has an internal 9.6MHz clock with the prescaler, don't scale back
# change above to match ATTINY84A =====================================================================
# to keep timing for neopixel
# The -O2 turns on optimizations, which are required to use the delay_*() functions
list(APPEND CMAKE_C_FLAGS "-std=c11 -O2 -mmcu=${AVR_TARGET_ARCH} -DF_CPU=8000000")

# Select headers

# Select the C source files
file(GLOB kicker_SRC "*.c")
file(GLOB hal_arch "hal/${AVR_TARGET_ARCH}/*.c")
file(GLOB hal_arch_v "hal/${AVR_TARGET_ARCH}/${KICKER_VERSION}/*.c")

# Select header files
file(GLOB inc "include/*.h")
file(GLOB inc_hal "include/hal/*.h")
file(GLOB inc_hal_arch "include/hal/${AVR_TARGET_ARCH}/*.h")
file(GLOB inc_hal_arch_v "include/hal/${AVR_TARGET_ARCH}/${KICKER_VERSION}/*.h")

set(all_kicker_src)
list(APPEND all_kicker_src ${kicker_SRC})
list(APPEND all_kicker_src ${hal_arch})
list(APPEND all_kicker_src ${hal_arch_v})
list(APPEND all_kicker_src ${inc})
list(APPEND all_kicker_src ${inc_hal})
list(APPEND all_kicker_src ${inc_hal_arch})
list(APPEND all_kicker_src ${inc_hal_arch_v})

message(${all_kicker_src})
add_executable(kicker_elf ${all_kicker_src})
target_sources(kicker_elf PUBLIC ${all_kicker_src})

# compile executable for kicker firmware
#add_executable(kicker_elf ${kicker_SRC} ${hal_arch} ${hal_arch_v})
#add_executable(kicker_elf)
#target_soruces(${kicker_elf} ${hal_arch} ${hal_arch_v} ${kicker_SRC})

# only build kicker firmware if specifically instructed
set_target_properties(kicker_elf PROPERTIES EXCLUDE_FROM_ALL TRUE)

# the final product is the .bin file, not the elf one.  We hide this away in the build dir
set_target_properties(kicker_elf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# custom target to convert kicker.elf to the kickerFW binary file and place it in the 'run' directory
add_custom_target(kicker
    # the -j options tell objcopy what sections to include in the output
    # the -O ihex specifies what output format to use, which is the hex format needed for flashing the AVR
    COMMAND ${AVR_OBJCOPY} -j .text -j .data -O binary ${CMAKE_CURRENT_BINARY_DIR}/kicker_elf ${PROJECT_SOURCE_DIR}/run/rj-kickr.nib
    DEPENDS kicker_elf
    COMMENT "objcopying to convert kicker.elf to rj-kickr.nib"
)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_SOURCE_DIR}/run/rj-kickr.nib)

# add command to copy the kicker firmware onto the mbed
# names are limited to 8.3 filename types since we need on-board access to this during runtime
# lookup the limitations of an 8.3 filename before changing the output binary name if that's ever needed
add_custom_target(kicker-prog
    COMMAND ${MBED_COPY_SCRIPT} ${PROJECT_SOURCE_DIR}/run/rj-kickr.nib
    DEPENDS kicker
    COMMENT "Copying the kicker board's binary over to the mbed"
)
