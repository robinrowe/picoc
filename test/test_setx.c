/* Test 1: Basic struct without member functions */
#include <stdio.h>

struct Foo 
{ 
    int x;
};

void SetX(struct Foo *f, int newX)
{
    printf("SetX called with %d\n", newX);
    f->x = newX;
}

int main()
{
    struct Foo foo;
    printf("Starting test\n");
    SetX(&foo, 42);
    printf("After SetX, foo.x = %d\n", foo.x);
    return 0;
}
