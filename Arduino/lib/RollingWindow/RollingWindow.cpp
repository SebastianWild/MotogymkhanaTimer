#include "Arduino.h"
#include "RollingWindow.h"
#include "LinkedList.h"

RollingWindow::RollingWindow(int size) {
    maxSize = size;
    storage = new SinglyLinkedList;
}

int RollingWindow::append(int value) {
    total += value;
    storage -> append(value);
    
    if (storage -> getSize() > maxSize)
        total -= storage -> dropLast();

    return value;
}

int RollingWindow::average() {
    // yes, we truncate when dividing but whatever for our use case :D
    return total / storage -> getSize();
}