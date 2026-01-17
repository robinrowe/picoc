// test_simple.c 

#include <stdio.h>

struct Foo 
{   void hello()
    {   puts("hello");
    }
};

int main()
{   puts("Starting main");
    struct Foo foo;
    puts("Created foo variable");
    foo.hello();
    puts("Called foo.hello()");
    return 0;
}
