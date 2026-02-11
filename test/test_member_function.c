// test_member_function.c

#include <stdio.h>

void gbar(int* x)
{   printf("gbar hello: %x\n",*x);
}

struct Foo 
{   void hello()
    {   puts("Foo.hello");
    }
};

int main()
{   int x = 3;
    gbar(&x);
    struct Foo foo;
    foo.hello();// works if comment out this line
    return 0;
}