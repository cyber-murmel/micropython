# Build the MicroPython Zephyr port as the `micropython` library, which provides main().
if(CONFIG_MICROPYTHON)
  include(${ZEPHYR_CURRENT_MODULE_DIR}/ports/zephyr/CMakeLists.txt)
endif()
