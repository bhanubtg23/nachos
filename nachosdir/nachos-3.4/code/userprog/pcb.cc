#include "pcb.h"


PCB::PCB(int id) {

    pid = id;
    parent = NULL;
    children = new List();
    thread = NULL;
    exitStatus = -9999;
    pcbLock = new Lock("pcbLock");
    files = new List();
}

PCB::PCB(Thread* t, int id, Thread* par)
{
    pid = id;
    pare = par;
    thrd = t;
    children = new List();
    files = new List();
    pcbLock = new Lock("pcbLock");
    pcbCond = new Condition("pcbCond");
    uids = new BitMap(MAX_FILES);
    uids->Mark(0); //stdin and stdout
    uids->Mark(1);
//     files->SortedInsert(new UserOpenFile("stdin", 0));
//     files->SortedInsert(new UserOpenFile("stdout", 1));
}



PCB::~PCB() {

    delete children;
    delete pcbLock;
    delete uids;
    delete pcbCond;
}



void PCB::AddChild(PCB* pcb) {

    children->Append(pcb);


}


int PCB::RemoveChild(PCB* pcb){

    return children->RemoveItem(pcb);

}


bool PCB::HasExited() {
    return exitStatus == -9999 ? false : true;
}


void decspn(int arg) {
    PCB* pcb = (PCB*)arg;
    if (pcb->HasExited()) pcbManager->DeallocatePCB(pcb);
    else pcb->parent = NULL;
}


void PCB::DeleteExitedChildrenSetParentNull() {
    children->Mapcar(decspn);
}

int
PCB::GetUID()
{
    int uid = -1;

    pcbLock->Acquire();
    uid = uids->Find();
    pcbLock->Release();
    
    return uid;
}