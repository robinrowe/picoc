#include <stdio.h>

struct Foo 
{   void hello()
    {   puts("hello");
    }
};

int main()
{   struct Foo foo;
    foo.hello();
    return 0;
}