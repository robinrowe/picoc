// test_scoper.c

#include <stdio.h>

static int bar;

struct Foo 
{	int bar;
};

int main() 
{   struct Foo foo;
    bar = 1;
	foo.bar = 2; 
	printf("bar = %i, foo.bar = %i\n",bar,foo.bar);
	::bar = 3;
    return 0;
}