#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"

#include "mpconfigboard.h"

// Python internal features.
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_VFS                 (1)
#define MICROPY_READER_VFS          (MICROPY_VFS)
#define MICROPY_ENABLE_FINALISER    (MICROPY_READER_VFS)

// Fine control over Python builtins, classes, modules, etc.
#define MICROPY_PY_OS               (1)
#define MICROPY_PY_TIME             (1)

// extended modules
#define MICROPY_PY_MACHINE          (1)
#define MICROPY_PY_MACHINE_INCLUDEFILE "ports/rtl8762c/mp_helper/mods/modmachine.c"

#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
    } while (0);

// Heap size for Garbage Collector
#define MP_HEAP_SIZE                        (16 * 1024)

#define MICROPY_TASK_NAME                   "MicroPython"
#define MICROPY_TASK_STACK_DEPTH            (((20 * 1024) + 512) / sizeof(StackType_t))
#define MICROPY_TASK_PRIORITY               (tskIDLE_PRIORITY+1)

// Type definitions for the specific machine.

typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

// We need to provide a declaration/definition of alloca().
#include <alloca.h>

#define MP_STATE_PORT MP_STATE_VM
