// test_simple.c 

#include <stdio.h>

struct Foo 
{   void hello()
    {   puts("hello");
    }
};

void Global()
{   puts("global");
}

int main()
{   puts("Starting main");
    Global();
    struct Foo foo;
    puts("Created foo variable");
    foo.hello();
    puts("Called foo.hello()");
    return 0;
}
