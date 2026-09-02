/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mpconfig.h"
#if MICROPY_PY_ZEPHYR

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/debug/thread_analyzer.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/sys_heap.h>

#include "modzephyr.h"
#include "py/runtime.h"
#include "py/emitglue.h"
#include "py/objtype.h"
#include "py/objfun.h"

static mp_obj_t mod_is_preempt_thread(void) {
    return mp_obj_new_bool(k_is_preempt_thread());
}
static MP_DEFINE_CONST_FUN_OBJ_0(mod_is_preempt_thread_obj, mod_is_preempt_thread);

static mp_obj_t mod_current_tid(void) {
    return MP_OBJ_NEW_SMALL_INT(k_current_get());
}
static MP_DEFINE_CONST_FUN_OBJ_0(mod_current_tid_obj, mod_current_tid);

#ifdef CONFIG_THREAD_ANALYZER
static mp_obj_t mod_thread_analyze(mp_obj_t cpu_in) {
    #if KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(4, 0, 0)
    unsigned int cpu = mp_obj_get_int(cpu_in);
    thread_analyzer_print(cpu);
    #else
    thread_analyzer_print();
    #endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mod_thread_analyze_obj, mod_thread_analyze);

static volatile mp_obj_t thread_info_list;
static const size_t thread_info_len = 3
#ifdef CONFIG_THREAD_RUNTIME_STATS
+1
#ifdef CONFIG_SCHED_THREAD_USAGE
+1
#endif
#endif
#ifdef CONFIG_THREAD_ANALYZER_STACK_SAFETY
+1
#endif
#ifdef CONFIG_THREAD_ANALYZER_PRIV_STACK_USAGE
+2
#endif
;

static const size_t usage_len = 2
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
+3
#endif
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
+1
#endif
;

static void mod_thread_analyzer_run_cb(struct thread_analyzer_info *info) {
    mp_obj_t thread_info = mp_obj_new_dict(thread_info_len);
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_name), mp_obj_new_str(info->name, strlen(info->name)));
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_stack_size), mp_obj_new_int_from_uint(info->stack_size));
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_stack_used), mp_obj_new_int_from_uint(info->stack_used));
#ifdef CONFIG_THREAD_RUNTIME_STATS
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_utilization), mp_obj_new_int_from_uint(info->utilization));
#ifdef CONFIG_SCHED_THREAD_USAGE
    mp_obj_t usage = mp_obj_new_dict(usage_len);
    mp_obj_dict_store(usage, MP_OBJ_NEW_QSTR(MP_QSTR_execution_cycles), mp_obj_new_int_from_uint(info->usage.execution_cycles));
    mp_obj_dict_store(usage, MP_OBJ_NEW_QSTR(MP_QSTR_total_cycles), mp_obj_new_int_from_uint(info->usage.total_cycles));
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
    mp_obj_dict_store(usage, MP_OBJ_NEW_QSTR(MP_QSTR_current_cycles), mp_obj_new_int_from_uint(info->usage.current_cycles));
    mp_obj_dict_store(usage, MP_OBJ_NEW_QSTR(MP_QSTR_peak_cycles), mp_obj_new_int_from_uint(info->usage.peak_cycles));
    mp_obj_dict_store(usage, MP_OBJ_NEW_QSTR(MP_QSTR_average_cycles), mp_obj_new_int_from_uint(info->usage.average_cycles));
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
    mp_obj_dict_store(usage, MP_OBJ_NEW_QSTR(MP_QSTR_idle_cycles), mp_obj_new_int_from_uint(info->usage.idle_cycles));
#endif /* CONFIG_SCHED_THREAD_USAGE_ALL */
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_usage), usage);
#endif /* CONFIG_SCHED_THREAD_USAGE */
#endif /* CONFIG_THREAD_RUNTIME_STATS */
#ifdef CONFIG_THREAD_ANALYZER_STACK_SAFETY
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_stack_safety), mp_obj_new_int_from_uint(info->stack_safety));
#endif /* CONFIG_THREAD_ANALYZER_STACK_SAFETY */
#ifdef CONFIG_THREAD_ANALYZER_PRIV_STACK_USAGE
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_priv_stack_size), mp_obj_new_int_from_uint(info->priv_stack_size));
    mp_obj_dict_store(thread_info, MP_OBJ_NEW_QSTR(MP_QSTR_priv_stack_used), mp_obj_new_int_from_uint(info->priv_stack_used));
#endif /* CONFIG_THREAD_ANALYZER_PRIV_STACK_USAGE */
    mp_obj_list_append(thread_info_list, thread_info);
};

static mp_obj_t mod_thread_analyzer_run(mp_obj_t cpu_in) {
    thread_info_list = mp_obj_new_list(0, NULL);
    #if KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(4, 0, 0)
    unsigned int cpu = mp_obj_get_int(cpu_in);
    thread_analyzer_run(mod_thread_analyzer_run_cb, cpu);
    #else
    thread_analyzer_run(mod_thread_analyzer_run_cb);
    #endif
    return thread_info_list;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mod_thread_analyzer_run_obj, mod_thread_analyzer_run);
#endif // CONFIG_THREAD_ANALYZER

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
static mp_obj_t mod_sys_memory_stats(void) {
	struct sys_heap **ha;
	int n = sys_heap_array_get(&ha);
    // mp_obj_t sys_memory_stats_list = mp_obj_new_list(n, NULL);
    mp_obj_t memory_stats = mp_obj_new_dict(n);

    for (int i = 0; i < n; i++) {
        struct sys_memory_stats stats;
        sys_heap_runtime_stats_get(ha[i], &stats);

        mp_obj_t dict = mp_obj_new_dict(3);
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_allocated_bytes), mp_obj_new_int_from_uint(stats.allocated_bytes));
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_free_bytes), mp_obj_new_int_from_uint(stats.free_bytes));
        mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_max_allocated_bytes), mp_obj_new_int_from_uint(stats.max_allocated_bytes));

        mp_obj_dict_store(memory_stats, mp_obj_new_int_from_uint((size_t) (ha[i])), dict);
        // mp_obj_list_store(sys_memory_stats_list, MP_OBJ_NEW_SMALL_INT(i), dict);
	}

    return memory_stats;
}
static MP_DEFINE_CONST_FUN_OBJ_0(mod_sys_memory_stats_obj, mod_sys_memory_stats);
#endif // CONFIG_SYS_HEAP_RUNTIME_STATS

#ifdef CONFIG_SHELL_BACKEND_SERIAL
static mp_obj_t mod_shell_exec(mp_obj_t cmd_in) {
    const char *cmd = mp_obj_str_get_str(cmd_in);
    shell_execute_cmd(shell_backend_uart_get_ptr(), cmd);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mod_shell_exec_obj, mod_shell_exec);
#endif // CONFIG_SHELL_BACKEND_SERIAL

static const mp_rom_map_elem_t mp_module_time_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_zephyr) },
    { MP_ROM_QSTR(MP_QSTR_is_preempt_thread), MP_ROM_PTR(&mod_is_preempt_thread_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_tid), MP_ROM_PTR(&mod_current_tid_obj) },
    #ifdef CONFIG_THREAD_ANALYZER
    { MP_ROM_QSTR(MP_QSTR_thread_analyze), MP_ROM_PTR(&mod_thread_analyze_obj) },
    { MP_ROM_QSTR(MP_QSTR_thread_analyzer_run), MP_ROM_PTR(&mod_thread_analyzer_run_obj) },
    #endif
    #ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
    { MP_ROM_QSTR(MP_QSTR_sys_memory_stats), MP_ROM_PTR(&mod_sys_memory_stats_obj) },
    #endif
    #ifdef CONFIG_SHELL_BACKEND_SERIAL
    { MP_ROM_QSTR(MP_QSTR_shell_exec), MP_ROM_PTR(&mod_shell_exec_obj) },
    #endif
    #ifdef CONFIG_DISK_ACCESS
    { MP_ROM_QSTR(MP_QSTR_DiskAccess), MP_ROM_PTR(&zephyr_disk_access_type) },
    #endif
    #ifdef CONFIG_FLASH_MAP
    { MP_ROM_QSTR(MP_QSTR_FlashArea), MP_ROM_PTR(&zephyr_flash_area_type) },
    #endif
    #ifdef CONFIG_FILE_SYSTEM
    { MP_ROM_QSTR(MP_QSTR_FileSystem), MP_ROM_PTR(&zephyr_filesystem_type) },
    #endif
};

static MP_DEFINE_CONST_DICT(mp_module_time_globals, mp_module_time_globals_table);

const mp_obj_module_t mp_module_zephyr = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_time_globals,
};

MP_REGISTER_MODULE(MP_QSTR_zephyr, mp_module_zephyr);

#endif // MICROPY_PY_ZEPHYR
