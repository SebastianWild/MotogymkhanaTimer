#include "Arduino.h"

#ifndef LinkedList_H
#define LinkedList_H

struct Node {
    int data;
    Node* next;
};

class SinglyLinkedList {
    public:
        SinglyLinkedList();
        void append(int value);
        int dropLast();
        int getSize();
    private:
        Node* head;
        Node* tail;
        int size = 0;
};

#endif