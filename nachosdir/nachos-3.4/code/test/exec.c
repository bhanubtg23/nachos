#include "syscall.h"

void usememory(){

	Exec("../test/memory");

}

int main()
{
	int i;
	for (i = 0; i < 5; i++) {

		Fork(usememory);
		Yield();
	}
	Exit(0);
}
