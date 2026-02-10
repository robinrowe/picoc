// test_ctor.c

#include <stdio.h>

struct Foo 
{	int bar;
    Foo(int bar)
	{	.bar = bar;
	}
	~Foo()
	{	printf("Foo.bar = %d\n",bar);
	}
};

int main() 
{   struct Foo foo;
    foo.Foo(3);
	foo.~Foo();
	return 0;
}