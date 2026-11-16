# This file is part of the MicroPython project, http://micropython.org/
#
# The MIT License (MIT)
#
# The port's sources, apart from how they are built: for CMakeLists.txt, and for an application that
# builds the port its own way. Needs MICROPY_PORT_DIR and MICROPY_DIR.

set(MICROPY_SOURCE_PORT
    main.c
    gccollect.c
    help.c
    machine_i2c.c
    machine_spi.c
    machine_pin.c
    machine_timer.c
    modbluetooth_zephyr.c
    modsocket.c
    modzephyr.c
    modzsensor.c
    mphalport.c
    uart_core.c
    usb.c
    zephyr_device.c
    zephyr_storage.c
    zephyr_filesystem.c
    mpthreadport.c
)
list(TRANSFORM MICROPY_SOURCE_PORT PREPEND ${MICROPY_PORT_DIR}/)
if(CONFIG_LVGL)
list(APPEND MICROPY_SOURCE_PORT
    ${ZEPHYR_LV_BINDING_MICROPYTHON_MODULE_DIR}/driver/zephyr/modlvzephyr.c
    ${ZEPHYR_LVGL_MODULE_DIR}/src/stdlib/micropython/lv_mem_core_micropython.c
)
endif()

set(MICROPY_SOURCE_SHARED
    libc/printf.c
    readline/readline.c
    runtime/interrupt_char.c
    runtime/mpirq.c
    runtime/pyexec.c
    runtime/stdout_helpers.c
    runtime/sys_stdio_mphal.c
    timeutils/timeutils.c
)
if (NOT CONFIG_IDF_TARGET_ARCH_XTENSA)
list(APPEND MICROPY_SOURCE_SHARED
    runtime/gchelper_generic.c
)
endif()
list(TRANSFORM MICROPY_SOURCE_SHARED PREPEND ${MICROPY_DIR}/shared/)

set(MICROPY_SOURCE_DRIVERS
    bus/softspi.c
)
list(TRANSFORM MICROPY_SOURCE_DRIVERS PREPEND ${MICROPY_DIR}/drivers/)

set(MICROPY_QSTRDEFS_PORT
    ${MICROPY_PORT_DIR}/qstrdefsport.h
)

if (CONFIG_MICROPY_VFS_FAT)

list(APPEND MICROPY_SOURCE_LIB
    oofatfs/ff.c
    oofatfs/ffunicode.c
)

endif()

if (CONFIG_MICROPY_VFS_LFS1)

list(APPEND MICROPY_SOURCE_LIB
    littlefs/lfs1.c
    littlefs/lfs1_util.c
)

endif()

if (CONFIG_MICROPY_VFS_LFS2)

list(APPEND MICROPY_SOURCE_LIB
    littlefs/lfs2.c
    littlefs/lfs2_util.c
)

endif()

list(TRANSFORM MICROPY_SOURCE_LIB PREPEND ${MICROPY_DIR}/lib/)
