#include "pcbmanager.h"
#include "system.h"

PCBManager::PCBManager(int maxProcesses) {

    bitmap = new BitMap(maxProcesses);
    pcbs = new PCB*[maxProcesses];
    printf("PCB size = %d\n", sizeof(pcbs));
    for(int i = 0; i < maxProcesses; i++) {
        pcbs[i] = NULL;
    }

}


PCBManager::~PCBManager() {

    delete bitmap;

    delete pcbs;

}


PCB* PCBManager::AllocatePCB() {

    // Aquire pcbManagerLock
    //pcbManagerLock->Acquire();

    int pid = bitmap->Find();

    // Release pcbManagerLock
    //pcbManagerLock->Release();
    if(pid != -1){

    pcbs[pid] = new PCB(pid);

    return pcbs[pid];
}
}


int PCBManager::DeallocatePCB(PCB* pcb) {

    // Check is pcb is valid -- check pcbs for pcb->pid
    
    if( (pcb !=NULL) && (pcb->pid !=-1) && (pcbs[pcb->pid] !=NULL) ) {
        // Aquire pcbManagerLock

        bitmap->Clear(pcb->pid);

        // Release pcbManagerLock

        delete pcbs[pcb->pid];

        pcbs[pcb->pid] = NULL;
    }
    return 0;
}

PCB* PCBManager::GetPCB(int pid) {
    // checking if pid not -1
    if(pid !=-1)
        return pcbs[pid];
}