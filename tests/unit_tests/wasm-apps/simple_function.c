/*
* Created by Edoardo Tinto @ Unipd
*/
__attribute__((import_name("runtime_request_checkpoint"))) extern void
runtime_request_checkpoint();

__attribute__((export_name("simple_function"))) int
simple_function()
{
    /* Initialization.  */
    int A      = 76;
    int result;

    runtime_request_checkpoint();

    result = A;
    return result;
}
