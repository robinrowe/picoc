// test_simple.c 

#include <stdio.h>

struct Foo 
{   void helloMethod()
    {   puts("hello");
    }
};

void helloFunction()
{   puts("global");
}

int main()
{   puts("main: Starting, calling global function");
    helloFunction();
    puts("main: Creating foo struct");
    struct Foo foo;
    puts("main: Calling foo.helloMethod()");
    foo.helloMethod();
    return 0;
}
