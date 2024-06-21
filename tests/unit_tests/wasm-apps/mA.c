__attribute__((export_name("A1"))) int
A1()
{
    int i = 0;
    while(i < 100000) {
        i += 1;
    }

    return i;
}

__attribute__((export_name("A2"))) int
A2()
{
    return 4;
}
