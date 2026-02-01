// test_dot_this.c

/* Add support for the TrapC dot-this operator and also this-> and :: for C++ compatibility. */

void Bar()
{   puts("Bar");
};

struct Foo
{   int x;
    int y;
    void SetX(int rhs)
    {   x = rhs;  
    }
    void SetY(int y)
    {  .y = y; // this->y = y
    }
    int GetSumThis()
    {   return .x + this->y;
    }
    void Bar()
    {   puts("FooBar");
    }
    void BarGlobal();
    void BarScoper();
};

void Foo.BarGlobal()
{   ..Bar();
}

void Foo::BarScoper();
{   ::Bar();
}

int main()
{   Foo foo;
    foo.SetX(10);      // Foo.SetX(&foo, 10)
    foo.SetY(20);      // Foo.SetY(&foo, 20)
    int sum = foo.GetSumThis();  // Foo.GetSumThis(&foo)
    printf("Sum: %d\n", sum);
    foo.Bar();
    foo.BarGlobal();
    foo.BarScoper();
    return 0;
}