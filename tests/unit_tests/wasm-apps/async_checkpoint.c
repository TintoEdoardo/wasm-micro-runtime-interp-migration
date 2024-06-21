/*
* Created by Edoardo Tinto @ Unipd
*/

__attribute__((export_name("counter"))) int
counter()
{
    /* Initialization.  */
    int counter = 0;

    /* Main loop.  */
    while (counter < 100000000) {
        counter += 1;
    }

    return counter;
}
