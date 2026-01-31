// test_mangle.c 

#include <stdio.h>

struct Foo 
{   void fooMethod()
    {   puts("main: Foo.fooMethod: hello");
    }
};

int main()
{   puts("main: Creating foo struct");
    struct Foo foo;
    puts("main: Calling Foo.fooMethod(&foo)");
//    foo.fooMethod();
    return 0;
}
