#include "copyright.h"
#include "system.h"
#include "synch.h" 
#include "elevator.h"

int nextPersonID = 1;
Lock *personIDLock = new Lock("PersonIDLock");
Lock *haillock = new Lock("hailLock");
int waitingFloor;
int requiredFloor;
bool isHail = false;
ELEVATOR *e;

Condition *isHailed = new Condition("isHailed");

void ELEVATOR::start(int numFloors) {

    while(1) {
    

    eL->Acquire();

    isHailed->Wait(eL);

    while(personsWaiting[waitingFloor-1]>0 && currentFloor<=numFloors){
    if(currentFloor<waitingFloor){
    while(currentFloor<waitingFloor)
    currentFloor++;
}
    else if(currentFloor>waitingFloor){
    while(currentFloor>waitingFloor)
    currentFloor--;
}

    printf("Elevator arrives at Floor %d\n", currentFloor);
    entering[currentFloor]->Signal(eL);
    isHailed->Wait(eL);

}      
 
    while(occupancy>0 && currentFloor<=numFloors){
    if(currentFloor<requiredFloor){
    while(currentFloor<requiredFloor)
    currentFloor++;
}
    else if(currentFloor>requiredFloor){
    currentFloor--;
}
    printf("Elevator reached to Floor %d\n", currentFloor);
    leaving[currentFloor]->Signal(eL);
    isHailed->Wait(eL);

}
    eL->Release();
}

}
   
void ElevatorThread(int numFloors) {
 
    printf("Elevator with %d floors was created!\n", numFloors);

    e = new ELEVATOR(numFloors);

    e->start(numFloors); 

    
}

ELEVATOR::ELEVATOR(int numFloors) {
    currentFloor = 1;
    maxOccupancy = 5;
    entering = new Condition*[numFloors];
    //Intialize entering
    for (int i = 0; i< numFloors; i++) {
        entering[i] = new Condition("Entering " + i);
    }
    personsWaiting = new int[numFloors];
    eL = new Lock("ElevatorLock"); 

    //Initialize leaving
    leaving = new Condition*[numFloors];
    for(int i = 0; i< numFloors; i++) {
            leaving[i] = new Condition("leaving " + i);
       }
}

void Elevator(int numFloors) {
 
    //create Elevator Thread
    Thread *t = new Thread("Elevator");
    t->Fork(ElevatorThread, numFloors);

}

void ELEVATOR::hailElevator(Person *p) {

    eL->Acquire();

    personsWaiting[p->atFloor-1]++;

    waitingFloor=p->atFloor;
    isHailed->Signal(eL);
    entering[waitingFloor]->Wait(eL);

    //get into elevator
    printf("Person %d got into the elevator at floor %d.\n", p->id, currentFloor);
    //decrement persons waiting atFloor
    personsWaiting[p->atFloor-1]--;
    //increment persons inside elevator
    occupancy++;

    requiredFloor = p->toFloor;
    isHailed->Signal(eL);


    isHail = true;
    leaving[requiredFloor]->Wait(eL);
    //get out of elevator
    printf("Person %d got out of the elevator at floor %d.\n", p->id, currentFloor);
    //decrement persons inside elevator
    occupancy--;

    isHailed->Signal(eL);

    eL->Release();

}
 
void PersonThread(int person) {

    Person *p = (Person *)person;

    printf("Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);
      
    e->hailElevator(p);

} 

int getNextPersonID() {
    int personID = nextPersonID;
    personIDLock->Acquire();
    nextPersonID = nextPersonID + 1;
    personIDLock->Release();
    return personID; 
}

void ArrivingGoingFromTo(int atFloor, int toFloor) {

    
    //Creates Person struct
    Person *p = new Person;
    p->id = getNextPersonID();
    p->atFloor = atFloor;
    p->toFloor = toFloor;

    //Creates Person Thread
    char* abc = const_cast<char*>("Person" + p->id);
    Thread *t = new Thread(abc);
    t->Fork(PersonThread, (int)p);

}