#include "Arduino.h"
#include "LinkedList.h"

#ifndef RollingWindow_H
#define RollingWindow_H

class RollingWindow {
    private:
        int maxSize;
        int total = 0;
        SinglyLinkedList* storage;
    public:
        int append(int value);
        int average();
        RollingWindow(int size);
};

#endif