#include "syscall.h"

void infinity(){
	
	while(1 == 1) 
	{
		Yield();
	}
}

int main()
{
	int ret;
	int id = Fork(infinity);
	Yield();
	ret = Kill(id);
	Exit(ret);
        return ret;
}