// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "memoryManager.h"

#include <stdio.h>
#include <string.h>
#include "pcbmanager.h"
#include "pcb.h"
#include "filesys.h"
#include "filemanager.h"
#include "addrspace.h"

int UserRead(int virtAddr, char * buffer, int size)
{
	int pos = 0,copied =0, left = 0, copy_size;
	int phyAddr;
	ExceptionType exception;

	while ( size > 0 ) 
	{
			do {
	    			exception = machine->Translate(virtAddr, &phyAddr, 1,FALSE);
				if (exception != NoException) {
					machine->RaiseException(exception, virtAddr);
				}
			}while(exception != NoException);
	    left = PageSize - (phyAddr) % PageSize;
	    copy_size = min( left, size);
	    bcopy ( buffer + copied,&machine->mainMemory[phyAddr], copy_size);
	    size -= copy_size;
	    copied += copy_size;
	    virtAddr += copy_size;
	}	
	return copied;
}

int UserWrite(int virtAddr, char * buffer, int size)
{
	int pos = 0,copied =0, left = 0, copy_size;
	int phyAddr;
	ExceptionType exception;

	while ( size > 0 ) 
	{
			do {
	    			exception = machine->Translate(virtAddr, &phyAddr, 1,FALSE);
				if (exception != NoException) {
					machine->RaiseException(exception, virtAddr);
				}
			}while(exception != NoException);
	    left = PageSize - (phyAddr) % PageSize;
	    copy_size = min( left, size);
	    bcopy (&machine->mainMemory[phyAddr], buffer + copied, copy_size);
	    size -= copy_size;
	    copied += copy_size;
	    virtAddr += copy_size;
	}	
	return copied;
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
void incrementPC() {
    int oldPCReg = machine->ReadRegister(PCReg);

    machine->WriteRegister(PrevPCReg, oldPCReg);
    machine->WriteRegister(PCReg, oldPCReg + 4);
    machine->WriteRegister(NextPCReg, oldPCReg + 8);
}

void childFunction(int pid) {


    // 1. Restore the state of registers
    currentThread->RestoreUserState();
 


    // 2. Restore the page table for child
     machine->pageTable = currentThread->space->GetPageTable();
    machine->pageTableSize = currentThread->space->GetNumPages();
    

     machine->Run();

}

void
readPath(char *path, int cmd)
{
	int pos = 0,copied =0;
	int phyAddr;

	do {
		machine->Translate(cmd, &phyAddr, 1,FALSE);
		bcopy(&machine->mainMemory[phyAddr], path + copied, 1);
		copied++;
		cmd++;
	}while(path[pos++] != 0);
	path[pos]=0;
}

int doFork(int functionAddr) {
    // 1. Check if sufficient memory exists to create new process
    printf("System Call: (%d) invoked (Fork) system call\n", currentThread->space->pcb->pid);


    if( currentThread->space->GetNumPages() > mem->GetFreePageCount()){
    //printf(" Child Process - required memory not available\n");

    return -1;
}
    // if check fails, return -1

    // 2. SaveUserState for the parent thread


    int pid=currentThread->space->pcb->pid;
    currentThread->SaveUserState();

    // 3. Create a new address space for child by copying parent address space
    AddrSpace*   childAddrSpace= new AddrSpace(currentThread->space);

    // 4. Create a new thread for the child and set its addrSpace
   Thread*  childThread = new Thread("childThread");
    childThread->space = childAddrSpace;
    // 5. Create a PCB for the child and connect it all up
    PCB*   pcb= pcbManager->AllocatePCB();
    printf("Fork - process id - %d \n",pcb->pid);
     pcb->thread = childThread;
    pcb->parent = currentThread->space->pcb;
    // set parent for child pcb
    // add child for parent pcb
    pcb->parent->AddChild(pcb);
    // 6. Set up machine registers for child and save it to child thread

 
    machine->WriteRegister(PCReg , functionAddr);
    machine->WriteRegister(PrevPCReg , functionAddr-4);
     machine->WriteRegister(NextPCReg , functionAddr+4);

     childThread->SaveUserState();
     childThread->space->pcb=pcb;
    currentThread->space->pcb=pcb->parent;
    printf("Process %d Fork: starting address %d with %d pages memory .\n",currentThread->space->pcb->pid,  PCReg, currentThread->space->GetNumPages());
    currentThread->RestoreUserState();

    // 7. Call thread->fork on Child
    childThread->Fork(childFunction, functionAddr);  

    // 8. Restore register state of parent user-level process
    machine->pageTable = currentThread->space->GetPageTable();
    machine->pageTableSize = currentThread->space->GetNumPages();

     return pcb->pid;

}
void doYield() {

    printf("System Call: (%d) invoked (Yield) system call \n",currentThread->space->pcb->pid);
    currentThread->Yield();

}
void doExit(int exitcode) {
  PCB* pcb = currentThread->space->pcb; 
 


    printf("System Call: (%d) invoked (Exit) system call\n", currentThread->space->pcb->pid);
  

    pcb->exitStatus=exitcode;
    printf ("Process (%d) exits with (%d) system call\n", currentThread->space->pcb->pid, exitcode);

    // Manage PCB memory As a parent process
 
    // Delete exited children and set parent null for non-exited ones
    pcb->DeleteExitedChildrenSetParentNull();


    // Manage PCB memory As a child process
    if(pcb->parent == NULL) {
    pcbManager->DeallocatePCB(pcb);}


    pcb->parent=NULL;

    currentThread->space->pcb=pcb;

    pcbManager->DeallocatePCB(pcb);
    // Delete address space only after use is completed
    delete currentThread->space;

    // Finish current thread only after all the cleanup is done
    // because currentThread marks itself to be destroyed (by a different thread)
    // and then puts itself to sleep -- thus anything after this statement will not be executed!
    currentThread->Finish();
}

int doExec(char* filename) {
    // Use progtest.cc:StartProcess() as a guide
    //printf("Syscall Call: (%d) invoked Exec system call \n",currentThread->space->pcb->pid);
    // 1. Open the file and check validity
    //printf("" file name is %s \n",filename);
     OpenFile *executable = fileSystem->Open(filename);
     AddrSpace *space;

    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return -1;
     }

    PCB* pcb = pcbManager->AllocatePCB();
  
    // Initialize parent
     PCB* parentpcb = currentThread->space->pcb->parent;


    printf("current  parent pcb is %d \n", currentThread->space->pcb->parent->pid);
    pcbManager->DeallocatePCB(currentThread->space->pcb);
    printf("deallocated pcb \n");
    delete currentThread->space;
    space = new AddrSpace(executable);
    
    // 3. Check if Addrspace creation was successful
        if(space->valid == false) {
        printf("Could not create AddrSpace\n");
            return -1;
        }
    
    pcb->parent=parentpcb ;
    pcb->thread=currentThread;  
    space->pcb=pcb;
    
    // 6. Delete current address space
    

    // 7. SEt the addrspace for currentThread
     
    printf("Syscall Call: [%d] invoked Exec \n",currentThread->space->pcb->pid);

    // 8.     delete executable;  // close file

    // 9. Initialize registers for new addrspace
      space->InitRegisters();		// set the initial register values

    // 10. Initialize the page table
    space->RestoreState();		// load page table register
    currentThread->space = space;
    delete executable;
    // 11. Run the machine now that all is set up
     machine->Run();			// jump to the user progam
    ASSERT(FALSE); // Execution nevere reaches here

    return 0;
}


int doJoin(int pid) {
    // 1. Check if this is a valid pid and return -1 if not
     PCB* joinPCB = pcbManager->GetPCB(pid);
     if (joinPCB == NULL) return -1;

    // 2. Check if pid is a child of current process
    PCB* pcb = currentThread->space->pcb;
    printf("System Call: (%d) invoked (Join) system call \n",pcb->pid);

     if (pcb != joinPCB->parent) return -1;

    // 3. Yield until joinPCB has not exited
    //scheduler->ReadyToRun(joinPCB->thread);

     while(!joinPCB->HasExited()) {
        currentThread->Yield();
     }
    // 4. Store status and delete joinPCB
     int status = joinPCB->exitStatus;
     delete joinPCB;

     return status;

}


int doKill (int pid) {

    // 1. Check if the pid is valid and if not, return -1
     PCB* joinPCB = pcbManager->GetPCB(pid);
    if (joinPCB == NULL) return -1;

    // 2. IF pid is self, then just exit the process
    if (joinPCB == currentThread->space->pcb) {
            doExit(0);
             return 0;
     }else {
        joinPCB->exitStatus = pid;

        // Delete exited children and set parent null for non-exited ones
        joinPCB->DeleteExitedChildrenSetParentNull();

        // Manage PCB memory As a child process
        if(joinPCB->parent == NULL) pcbManager->DeallocatePCB(joinPCB);

            
        joinPCB->parent->RemoveChild(joinPCB);
        joinPCB->parent=NULL;

        pcbManager->DeallocatePCB(joinPCB);

        delete joinPCB->thread->space;
        printf("deleted killing thread space \n");  

    }
    // 3. Valid kill, pid exists and not self, do cleanup similar to Exit
    // However, change references from currentThread to the target thread
    // pcb->thread is the target thread
    // 4. Set thread to be destroyed.
    scheduler->RemoveThread(joinPCB->thread);
    Thread * parent=currentThread;
        // 5. return 0 for success!
    currentThread=parent;
    printf("after killing");
    return 0;
}



// perform MMU translation to access physical memory
char* readString(int virtualAddr) {
    int i = 0;
    char* str = new char[256];
    unsigned int physicalAddr = currentThread->space->Translate(virtualAddr);

    // Need to get one byte at a time since the string may straddle multiple pages that are not guaranteed to be contiguous in the physicalAddr space
    bcopy(&(machine->mainMemory[physicalAddr]),&str[i],1);
    while(str[i] != '\0' && i != 256-1)
    {
        virtualAddr++;
        i++;
        physicalAddr = currentThread->space->Translate(virtualAddr);
        bcopy(&(machine->mainMemory[physicalAddr]),&str[i],1);
    }
    if(i == 256-1 && str[i] != '\0')
    {
        str[i] = '\0';
    }

    return str;
}
void doCreate(char* fileName)
{
    printf("Syscall Call: [%d] invoked Create.\n", currentThread->space->pcb->pid);
    fileSystem->Create(fileName, 0);
}
void
ExceptionHandler(ExceptionType which)
{

    int type = machine->ReadRegister(2);
    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } 
    else if ((which == SyscallException) && (type == SC_Fork)){

    DEBUG('a', "FORK, initiated by user program.\n");
    int reg= machine->ReadRegister(4);

    int pid=doFork(machine->ReadRegister(4));
    machine->WriteRegister(2,pid);
    incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Join)){
    DEBUG('a', "join, initiated by user program.\n");

    int id = machine->ReadRegister(4);
            
                int res = doJoin(id);
                
                machine->WriteRegister(2, res);
                incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Exec)){
    DEBUG('a', "exec, initiated by user program.\n");
    int virtAddr = machine->ReadRegister(4);
            char* fileName = readString(virtAddr);
            int ret = doExec(fileName);
            machine->WriteRegister(2, ret);
            incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Yield)){
    DEBUG('a', "yield, initiated by user program.\n");
    doYield();
    incrementPC();

    }else if ((which == SyscallException) && (type == SC_Exit)){
    DEBUG('a', "exit, initiated by user program.\n");
    doExit(machine->ReadRegister(4));
    incrementPC();

    
    }else if ((which == SyscallException) && (type == SC_Kill)){
    printf("inside kill \n");
    DEBUG('a', "kill, initiated by user program.\n");
    int ret = doKill(machine->ReadRegister(4));
            machine->WriteRegister(2, ret);
            incrementPC();
    }else if ((which == SyscallException) && (type == SC_Create)){
    int virtAddr = machine->ReadRegister(4);
            char* fileName = readString(virtAddr);
            doCreate(fileName);
            incrementPC();
    }else if ((which == SyscallException) && (type == SC_Open)){
        //
        char path[32];
		int newFileID = -1;
        int threadId = currentThread->space->pcb->GetPID();
		printf("System Call: [%d] invoked Open\n", threadId);

                readPath(path,machine->ReadRegister(4)); //get executable path
		DEBUG('w', "Read File name \"%s\"\n", path);
		int sysfid = fileManager->GetSysOpenFile(path);

		if(sysfid < 0)
		{
		    DEBUG('w', "File \"%s\" not in File Table!\n", path);
		    OpenFile* ofile = fileSystem->Open(path);
		    if(ofile == NULL)
			printf("File \"%s\" not found!\n", path);
		    else
		    {
			int id = fileManager->GetFID();
			if(id < 0)
			    printf("Failed to Open file \"%s\". No more system files allowed!\n", path);
			else
			{
			    PCB* currPCB = currentThread->space->pcb;
			    DEBUG('w', "File \"%s\" now has sysfid %d\n", path, id);
			    SysOpenFile* sfile = new SysOpenFile(id, path, ofile);
			    sfile->count++;
			    fileManager->files[id] = sfile;

			    int uid = currPCB->GetUID();
			    if(uid < 0)
				printf("Failed to Open file \"%s\". No more user files allowed!\n", path);
			    else
			    {
				UserOpenFile* ufile = new UserOpenFile(path, id, uid);
				DEBUG('w', "File starting offset is %d\n", ufile->offset);
				DEBUG('w', "File \"%s\" now has uid %d\n", path, uid);
				currPCB->files->SortedInsert((void*)ufile, uid);
				newFileID = uid;
			    }
			}
		    }
		}
		else
		{
		    PCB* currPCB = currentThread->space->pcb;
		    int uid = currPCB->GetUID();
		    if(uid < 0)
			printf("Failed to Open file \"%s\". No more user files allowed!\n", path);
		    else
		    {
			DEBUG('w', "\"%s\" already exists with id %d.\n", path, sysfid);
			fileManager->files[sysfid]->count++;
			DEBUG('w', "System file \"%s\" now has %d process(es) using it.\n", path, fileManager->files[sysfid]->count);		    
			
			UserOpenFile* ufile = new UserOpenFile(path, sysfid, uid);
			DEBUG('w', "File starting offset is %d\n", ufile->offset);
			currPCB->files->SortedInsert((void*)ufile, uid);
			DEBUG('w', "File \"%s\" now has uid %d\n", path, uid);
			newFileID = uid;
		    }
		}

		machine->WriteRegister(2, newFileID);
		
        //
    }else if ((which == SyscallException) && (type == SC_Close)){
        printf("System Call: [%d] invoked Close\n", currentThread->space->pcb->GetPID());

		int uid = machine->ReadRegister(4);
		if(uid >= 2)
		{
		    PCB* currPCB = currentThread->space->pcb;
		    UserOpenFile* ufile = (UserOpenFile*)currPCB->files->Remove(uid);
		    if(ufile != NULL)
		    {
			DEBUG('w', "Found UserOpenFile to remove with uid = %d\n", uid);
			fileManager->files[ufile->index]->count--;
			if(fileManager->files[ufile->index]->count > 0)
			    DEBUG('w', "%d process(es) now watching file %d\n", fileManager->files[ufile->index]->count, ufile->index);
			else
			{
			    DEBUG('w', "No more references to file %d\n", uid);
			    fileManager->files[ufile->index] = NULL;
			    fileManager->ClearFID(ufile->index);
			}
		    }
		}
		
     }else if ((which == SyscallException) && (type == SC_Write)){
        printf("System Call: [%d] invoked Write\n", currentThread->space->pcb->GetPID());
		int buffer_pointer,size, file_id ;
		char *buffer;
		int size_writed;
		OpenFile* file;
		
		buffer_pointer = machine->ReadRegister(4);
		size = machine->ReadRegister(5);
		file_id = machine->ReadRegister(6);
		
		if (file_id == ConsoleOutput) 
		{
		    buffer = new char[size +1];
		    UserWrite(buffer_pointer, buffer, size);
		    buffer[size] ='\0';
		    printf("%s",buffer);
		}
		else if(file_id != ConsoleInput)
		{
		    buffer = new char[size];
		    size_writed = UserWrite(buffer_pointer, buffer, size);
		    
		    UserOpenFile * uof = (UserOpenFile *) currentThread->space->pcb->files->GetElement(file_id);
		    if(uof != NULL)
		    {
			file = fileManager->files[uof->index]->file;
			file->WriteAt(buffer, size,  uof->offset);
			uof->offset += size_writed;
		    }
		}
		
    }else if ((which == SyscallException) && (type == SC_Read)){
        printf("System Call: [%d] invoked Read\n", currentThread->space->pcb->GetPID());
		int size;
		int buffer;
		int id, i, read;
		OpenFile* f;

		buffer = machine->ReadRegister(4);
		size = machine->ReadRegister(5);
		id = machine->ReadRegister(6);
		read = -1;

		char result[size+1];
		if(id != ConsoleOutput)
		{
		    if(id == ConsoleInput)
		    {
			for(i = 0; i < size; i++)
			{
			    result[i] = getchar();
			    read = i;
			}
			result[size] = '\0';
		    }
		    else
		    {
			UserOpenFile * uof = (UserOpenFile *) currentThread->space->pcb->files->GetElement(id);
			if(uof !=NULL)
			{
			    DEBUG('w', "About to read %d bytes, starting at offset %d, from file %d\n", size, uof->offset, id);
			    f = fileManager->files[uof->index]->file;
			    fileManager->fidLock->Acquire();
			    read = f->ReadAt(result, size, uof->offset);
			    fileManager->fidLock->Release();
			    uof->offset += read;
			    result[size] = '\0';
			}
		    }
		    
		    int bR = UserRead(buffer, result, size);
		    DEBUG('w', "Bytes Read: %d\n", bR);
		}
		machine->WriteRegister(2, read);
		
		
    }
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
        }
}

