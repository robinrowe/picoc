struct Foo 
{ 
    int x;
    int y;
    
    void SetX(int newX)
    {
        .x = newX;  // dot accesses member of 'this'
    }
    
    void SetY(int y)
    {
        .y = y;
    }
    
    int GetSum()
    {
        return .x + y;
    }
};

int main()
{
    Foo foo;
    foo.SetX(10);      // Transformed to: Foo_SetX(&foo, 10)
    foo.SetY(20);      // Transformed to: Foo_SetY(&foo, 20)
    int sum = foo.GetSum();  // Transformed to: Foo_GetSum(&foo)
    printf("Sum: %d\n", sum);
    return 0;
}