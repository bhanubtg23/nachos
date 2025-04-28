#include "syscall.h"

int global_cnt=0;

void sum(){
	int i;
	
	for (i=1;i<=100;i++) {
		global_cnt++;
	}
	Yield();
	
	Exit(global_cnt);
}

void fast_exit(){
	Exit(global_cnt);
}

int main()
{
	int i=0;
	SpaceId pid0,pid1,pid2;
	
	global_cnt++;
	pid0 = Fork(fast_exit);
	
	Yield();

	global_cnt++;
	pid1 = Fork(sum);

	Yield();
	
	global_cnt++;

	pid2 = Fork(fast_exit);

	Yield();
	
	Join(pid0);
	Join(pid1);
	Join(pid2);
	
	Exit(global_cnt);
}
