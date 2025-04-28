#ifndef PCB_H
#define PCB_H

#include "list.h"
#include "pcbmanager.h"
#include "filemanager.h"
#include "useropenfile.h"

#include "thread.h"
#include "bitmap.h"
#include "synch.h"

class Thread;
class PCBManager;
class UserOpenFile;
class Condition;
extern PCBManager* pcbManager;

class PCB {

    public:
        PCB(Thread* t, int id, Thread* par);
        PCB(int id);
        ~PCB();
        int pid;
        PCB* parent;
        Thread* thread;
        int exitStatus;
        Lock* pcbLock;

        int GetPID() {return pid;};
        void AddChild(PCB* pcb);
        int RemoveChild(PCB* pcb);
        bool HasExited();
        void DeleteExitedChildrenSetParentNull();
        int GetUID();

        List* files;
        BitMap* uids;
        Thread* thrd;
        Thread* pare;
        Condition* pcbCond;
    private:
        List* children;

};

#endif // PCB_H