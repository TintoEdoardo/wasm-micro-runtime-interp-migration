/*
* Created by Edoardo Tinto @ Unipd
*/

void
sleep(int intervals) {
    int iteration_cycles = 20000;
    int counter = 0;
    int limit = intervals * iteration_cycles;
    while (counter < limit) {
        counter +=1 ;
    }
}

__attribute__((export_name("start_counting"))) void
start_counting()
{
    /* Initialization.  */
    int counter = 0;
    int milliseconds = 1000;

    /* Main loop.  */
    while (counter < 5) {
        counter += 1;
        sleep(milliseconds);
    }
}
