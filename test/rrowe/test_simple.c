// test_simple.c 

#include <stdio.h>

struct Foo 
{   void fooMethod()
    {   puts("main: Foo.fooMethod: hello");
    }
};

void fooFunction()
{   puts("main: fooFunction: global");
}

int main()
{   puts("main: Starting, calling global function");
    fooFunction();
    puts("main: Creating foo struct");
    struct Foo foo;
    puts("main: Calling foo.helloMethod()");
    foo.fooMethod();
    return 0;
}
