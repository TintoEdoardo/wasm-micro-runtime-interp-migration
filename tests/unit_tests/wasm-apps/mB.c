__attribute__((import_module("mA")))
__attribute__((import_name("A1"))) extern int
A1();

__attribute__((import_module("mA")))
__attribute__((import_name("A2"))) extern int
A2();

__attribute__((export_name("B1"))) int
B1()
{
    A2();

    int i = 0;
    while(i < 1000) {
        A1();
        i += 1;
    }
    return 76;
}
