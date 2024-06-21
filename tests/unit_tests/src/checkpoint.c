/*
* Created by Edoardo Tinto @ Unipd
*/

#include "CUnit/Basic.h"
#include <platform_common.h>
#include "wasm_export.h"
#include <string.h>
#include "wasm_runtime.h"
#include "bh_read_file.h"

#include "wasm_exec_env.h"
#include "wasm_interp.h"

/* * * * * TEST SUITE 1 * * * * */

int init_suite_1(void)
{
    (void)0;
    return 0;
}

int clean_suite_1(void)
{
    (void)0;
    return 0;
}

/* Shared entities.  */
static uint8 *wasm_buffer;
static WASMExecEnv *exec_env;
static WASMExecEnvCheckpoint *exec_env_checkpoint;
static WASMModule *module;
static struct WASMModuleInstance *module_inst;
static struct WASMInterpFrame *frame;
static struct WASMInterpFrameCheckpoint *frame_checkpoint;

/* 10M.  */
static char sandbox_memory_space[10 * 1024 * 1024] = { 0 };

/* 6K */
const uint32 stack_size = 6 * 1024;
const uint32 heap_size = 6 * 1024;

/* Parameters and return values.  */
char *args[1] = { 0 };

/* Exported native functions.  */
void
runtime_request_checkpoint();

static char *module_search_path = ".";
static bool
module_callback_reader(package_type_t module_type, const char *module_name,
                       uint8 **p_buffer, uint32 *p_size)
{
    char *file_format = NULL;
    file_format = ".wasm";
    const char *format = "%s/%s%s";
    int sz = strlen(module_search_path) + strlen("/") + strlen(module_name)
            + strlen(file_format) + 1;
    char *wasm_file_name = wasm_runtime_malloc(sz);
    if (!wasm_file_name) {
        return false;
    }
    snprintf(wasm_file_name, sz, format, module_search_path, module_name,
             file_format);
    *p_buffer = (uint8_t *)bh_read_file_to_buffer(wasm_file_name, p_size);

    wasm_runtime_free(wasm_file_name);
    return *p_buffer != NULL;
}

static void
module_callback_destroyer(uint8 *buffer, uint32 size)
{
    if (!buffer) {
        return;
    }

    wasm_runtime_free(buffer);
    buffer = NULL;
}

static void
initialize_static_variables(const char *module_name)
{
    if(module_name != NULL) {

        RuntimeInitArgs init_args = { 0 };
        char error_buf[128] = { 0 };

        uint8 *file_buf = NULL;
        uint32 file_buf_size = 0;
        wasm_module_t _module = NULL;
        wasm_module_inst_t _module_inst = NULL;

        static NativeSymbol native_symbols[] = {
                {
                        "runtime_request_checkpoint", // the name of WASM function name
                        runtime_request_checkpoint,   // the native function pointer
                        "",  // the function prototype signature, avoid to use i32
                        NULL        // attachment is NULL
                }
        };

        /* All malloc() only from the given buffer.  */
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = sandbox_memory_space;
        init_args.mem_alloc_option.pool.heap_size = sizeof(sandbox_memory_space);

        /* Native symbols need below registration phase.  */
        init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
        init_args.native_module_name = "env";
        init_args.native_symbols = native_symbols;

        /* Initialize runtime environment.  */
        if (!wasm_runtime_full_init(&init_args)) {
            CU_FAIL("Init runtime environment failed.\n");
        }

        /* Set module reader and destroyer.  */
        wasm_runtime_set_module_reader(module_callback_reader, module_callback_destroyer);

        /* Load WASM byte buffer from WASM bin file.  */
        if (!(file_buf =
                      (uint8 *)bh_read_file_to_buffer(module_name, &file_buf_size))) {
            CU_FAIL((char *)error_buf)
        }
        /* For proper deletion.  */
        wasm_buffer = file_buf;

        if (!(_module = wasm_runtime_load(file_buf,
                                          file_buf_size,
                                          error_buf,
                                          sizeof(error_buf)))) {
            CU_FAIL((char *)error_buf);
        }

        /* Instantiate the module.  */
        if (!(_module_inst = wasm_runtime_instantiate(
                _module,
                stack_size,
                heap_size,
                error_buf,
                sizeof(error_buf)))) {
            CU_FAIL(error_buf);
        }

        /* Assign a value to the remaining variables.  */
        exec_env = wasm_runtime_get_exec_env_singleton(_module_inst);
        exec_env_checkpoint = exec_env->exec_env_checkpoint;
        module   = (WASMModule *)_module;
        module_inst = (struct WASMModuleInstance *) _module_inst;
        frame = exec_env->cur_frame;

    } else {
        exec_env          = NULL;
        exec_env_checkpoint = NULL;
        module            = NULL;
        module_inst       = NULL;
        frame             = NULL;
        frame_checkpoint  = NULL;
    }

}

void
clean_static_variables()
{
    if (exec_env)
        wasm_runtime_destroy_exec_env(exec_env);
    if (module_inst) {
        wasm_runtime_deinstantiate((wasm_module_inst_t)module_inst);
    }
    if (module)
        wasm_runtime_unload((wasm_module_t)module);
    if (wasm_buffer)
        BH_FREE(wasm_buffer);
    wasm_runtime_destroy();
}

/* Tests section.  */
void
test_wasm_runtime_request_checkpoint(void)
{
    /* Initialization.  */
    initialize_static_variables("empty_module.wasm");

    /* Test 1.  */
    CU_ASSERT_FALSE(exec_env->requested_migration);
    wasm_runtime_request_checkpoint(exec_env);
    CU_ASSERT_TRUE(exec_env->requested_migration);

    /* Finalization.  */
    clean_static_variables();
}

static void *
issue_migration_request(void *vargp)
{
    sleep(1);
    wasm_runtime_request_checkpoint(exec_env);
    return NULL;
}

void
runtime_request_checkpoint()
{
    printf("### runtime_request_checkpoint ### \n");
    issue_migration_request((void *)0);
}

void
test_checkpoint_of_simple_function(void)
{
    /* Initialization.  */
    initialize_static_variables("simple_function.wasm");
    WASMExecEnvCheckpoint *tmp_exec_env_checkp;

    /* The function itself will issue the migration request.  */

    /* Test 2.  */
    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            NULL,
            "simple_function", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, CHECKPOINTED);

    /* Save a reference to the checkpoint.  */
    tmp_exec_env_checkp = malloc(stack_size);
    wasm_copy_checkpoint(tmp_exec_env_checkp, exec_env->exec_env_checkpoint, stack_size);

    /* Copy the data section in linear memory.  */
    WASMMemoryInstance *memory_inst = wasm_get_default_memory(module_inst);
    uint64 memory_size  = memory_inst->memory_data_size;
    uint8 *data_memory = memory_inst->memory_data;
    uint8 *tmp_memory_data = malloc(memory_size);
    memcpy(tmp_memory_data, data_memory, memory_size);

    /* Free the WAMR runtime.  */
    clean_static_variables();

    /* Restart an execution from a checkpoint.  */
    initialize_static_variables("simple_function.wasm");
    exec_env_checkpoint = tmp_exec_env_checkp;
    CU_ASSERT_PTR_NOT_NULL(exec_env_checkpoint);

    /* Add a reference to tmp_memory_data.  */
    exec_env_checkpoint->memory_data = tmp_memory_data;
    exec_env_checkpoint->memory_data_size = memory_size;

    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            exec_env_checkpoint,
            "simple_function", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, OPERATIONAL);

    /* Finalization.  */
    clean_static_variables();
}

void
test_checkpoint_of_loop_function(void)
{
    /* Initialization.  */
    initialize_static_variables("loop_function.wasm");
    WASMExecEnvCheckpoint *tmp_exec_env_checkp;

    /* The function itself will issue the migration request.  */

    /* Test 2.  */
    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            NULL,
            "loop_function", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, CHECKPOINTED);

    /* Save a reference to the checkpoint.  */
    tmp_exec_env_checkp = malloc(stack_size);
    wasm_copy_checkpoint(tmp_exec_env_checkp, exec_env->exec_env_checkpoint, stack_size);

    /* Copy the data section in linear memory.  */
    WASMMemoryInstance *memory_inst = wasm_get_default_memory(module_inst);
    uint64 memory_size  = memory_inst->memory_data_size;
    uint8 *data_memory = memory_inst->memory_data;
    uint8 *tmp_memory_data = malloc(memory_size);
    memcpy(tmp_memory_data, data_memory, memory_size);

    /* Free the WAMR runtime.  */
    clean_static_variables();

    /* Restart an execution from a checkpoint.  */
    initialize_static_variables("loop_function.wasm");
    exec_env_checkpoint = tmp_exec_env_checkp;
    CU_ASSERT_PTR_NOT_NULL(exec_env_checkpoint);

    /* Add a reference to tmp_memory_data.  */
    exec_env_checkpoint->memory_data = tmp_memory_data;
    exec_env_checkpoint->memory_data_size = memory_size;

    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            exec_env_checkpoint,
            "loop_function", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, OPERATIONAL);

    /* Finalization.  */
    clean_static_variables();
}

void
test_checkpoint_of_async_function(void)
{
    /* Initialization.  */
    initialize_static_variables("async_checkpoint.wasm");
    WASMExecEnvCheckpoint *tmp_exec_env_checkp;

    /* Simulate an incoming migration request.  */
    pthread_t thread_id;
    pthread_create(&thread_id, NULL,
                   issue_migration_request,
                   (void *)0);

    /* Test 2.  */
    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            NULL,
            "counter", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, CHECKPOINTED);

    /* Save a reference to the checkpoint.  */
    tmp_exec_env_checkp = malloc(stack_size);
    wasm_copy_checkpoint(tmp_exec_env_checkp, exec_env->exec_env_checkpoint, stack_size);

    /* Copy the data section in linear memory.  */
    WASMMemoryInstance *memory_inst = wasm_get_default_memory(module_inst);
    uint64 memory_size  = memory_inst->memory_data_size;
    uint8 *data_memory = memory_inst->memory_data;
    uint8 *tmp_memory_data = malloc(memory_size);
    memcpy(tmp_memory_data, data_memory, memory_size);

    /* Free the WAMR runtime.  */
    clean_static_variables();

    /* Restart an execution from a checkpoint.  */
    initialize_static_variables("async_checkpoint.wasm");
    exec_env_checkpoint = tmp_exec_env_checkp;
    CU_ASSERT_PTR_NOT_NULL(exec_env_checkpoint);

    /* Add a reference to tmp_memory_data.  */
    exec_env_checkpoint->memory_data = tmp_memory_data;
    exec_env_checkpoint->memory_data_size = memory_size;

    printf("RESUMING \n");
    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            exec_env_checkpoint,
            "counter", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, OPERATIONAL);

    /* Finalization.  */
    clean_static_variables();
}

void
test_checkpoint_of_multi_modules_function(void)
{
    /* Initialization.  */
    initialize_static_variables("mB.wasm");
    WASMExecEnvCheckpoint *tmp_exec_env_checkp;

    /* Simulate an incoming migration request.  */
    pthread_t thread_id;
    pthread_create(&thread_id, NULL,
                   issue_migration_request,
                   (void *)0);

    /* Test 2.  */
    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            NULL,
            "B1", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, CHECKPOINTED);

    /* Save a reference to the checkpoint.  */
    tmp_exec_env_checkp = malloc(stack_size);
    wasm_copy_checkpoint(tmp_exec_env_checkp, exec_env->exec_env_checkpoint, stack_size);

    /* Copy the data section in linear memory.  */
    WASMMemoryInstance *memory_inst = wasm_get_default_memory(module_inst);
    uint64 memory_size  = memory_inst->memory_data_size;
    uint8 *data_memory = memory_inst->memory_data;
    uint8 *tmp_memory_data = malloc(memory_size);
    memcpy(tmp_memory_data, data_memory, memory_size);

    /* Free the WAMR runtime.  */
    clean_static_variables();

    /* Restart an execution from a checkpoint.  */
    initialize_static_variables("mB.wasm");
    exec_env_checkpoint = tmp_exec_env_checkp;
    CU_ASSERT_PTR_NOT_NULL(exec_env_checkpoint);

    /* Add a reference to tmp_memory_data.  */
    exec_env_checkpoint->memory_data = tmp_memory_data;
    exec_env_checkpoint->memory_data_size = memory_size;

    printf("RESUMING \n");
    wasm_application_execute_func(
            (wasm_module_inst_t )module_inst,
            exec_env_checkpoint,
            "B1", 0, args);
    CU_ASSERT_EQUAL(exec_env->state, OPERATIONAL);

    /* Finalization.  */
    clean_static_variables();
}