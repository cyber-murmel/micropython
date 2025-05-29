#include "py/builtin.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"

#include "os/os_sched.h"
#include "os/os_task.h"
#include "FreeRTOS/FreeRTOS.h"

#include "uart.h"
#include "modmachine.h"
#include "mphalport.h"

void *mp_main_task_handle;

// Allocate memory for the MicroPython GC heap.
static char heap[MP_HEAP_SIZE];

static void micropython_task(void *p_param) {
    uart_init();

    uart_tx_strn("Hello\r\n", 7);

    // Initialise the MicroPython runtime.
    mp_stack_ctrl_init();
    gc_init(heap, heap + sizeof(heap));
    mp_init();

    // initialise peripherals
    machine_pins_init();

    // Start a normal REPL; will exit when ctrl-D is entered on a blank line.
    pyexec_friendly_repl();

    // Deinitialise the runtime.
    gc_sweep_all();
    mp_deinit();
}

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    os_task_create(\
        &mp_main_task_handle, \
        MICROPY_TASK_NAME, \
        micropython_task, \
        0, \
        MICROPY_TASK_STACK_DEPTH, \
        MICROPY_TASK_PRIORITY);
    os_sched_start();

    return 1;
}

// Handle uncaught exceptions (should never be reached in a correct C implementation).
void nlr_jump_fail(void *val) {
    for (;;) {
    }
}

// Do a garbage collection cycle.
void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

// There is no filesystem so stat'ing returns nothing.
mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

// There is no filesystem so opening a file raises an exception.
mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}
