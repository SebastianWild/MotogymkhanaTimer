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
        void clear();
        int average();
        int size();
        RollingWindow(int size);
};

#endif