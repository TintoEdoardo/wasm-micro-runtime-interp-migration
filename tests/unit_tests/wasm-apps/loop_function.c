/*
* Created by Edoardo Tinto @ Unipd
*/
__attribute__((import_name("runtime_request_checkpoint"))) extern void
runtime_request_checkpoint();

__attribute__((export_name("loop_function"))) int
loop_function()
{
    /* Initialization.  */
    int i = 0;

    for(; i < 5; i++) {
        if(i == 2)
            runtime_request_checkpoint();
    }

    return i;
}
