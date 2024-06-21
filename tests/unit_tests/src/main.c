/*
* Written by Edoardo Tinto @ Unipd
*/

#include "checkpoint.c"
#include "CUnit/Basic.h"

int main()
{
    CU_pSuite  pSuite = NULL;

    /* Initialize the CUnit test registry.  */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Add a suite to the registry.  */
    pSuite = CU_add_suite("Checkpoint_Suite_1", init_suite_1, clean_suite_1);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite.  */
    if (
        (NULL == CU_add_test(pSuite, "Test wasm_runtime_request_checkpoint", test_wasm_runtime_request_checkpoint)) ||
        (NULL == CU_add_test(pSuite, "Test checkpoint of a simple function", test_checkpoint_of_simple_function)) ||
        (NULL == CU_add_test(pSuite, "Test checkpoint of a loop function", test_checkpoint_of_loop_function)) ||
        (NULL == CU_add_test(pSuite, "Test checkpoint of an async function", test_checkpoint_of_async_function)) ||
        (NULL == CU_add_test(pSuite, "Test checkpoint of a multi-modules function", test_checkpoint_of_multi_modules_function))
        )
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
