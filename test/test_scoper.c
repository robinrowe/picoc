// test_scoper.c

#include <stdio.h>

static int bar;

struct Foo 
{	int bar;
	int Bar()
	{	this->bar = 5; // TokenIdentifier "this" followed by TokenArrow then TokenIdentifier
		return bar;
	}
};

int main() 
{   struct Foo foo;
    bar = 1;
	foo.bar = 2; // TokenDot followed by TokenIdentifier
    ..bar = 3;   // TokenDoubleDot followed by TokenIdentifier
    ::bar = 4;   // TokenScopeRes followed by TokenIdentifier 
	printf("bar = %i, foo.bar = %i, foo.Bar() = %i\n",bar,foo.bar,foo.Bar());
    return 0;
}