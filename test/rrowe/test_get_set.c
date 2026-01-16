/* Simple debug test */
#include <stdio.h>

struct Foo 
{ 
    int x;
    
    void SetX(int newX)
    {
        printf("SetX called with %d\n", newX);
        x = newX;
    }
    
    int GetX()
    {
        printf("GetX called, x = %d\n", x);
        return x;
    }
};

int main()
{
    struct Foo foo;
    printf("Before SetX\n");
    foo.SetX(42);
    printf("After SetX\n");
    int result = foo.GetX();
    printf("Result: %d\n", result);
    return 0;
}
