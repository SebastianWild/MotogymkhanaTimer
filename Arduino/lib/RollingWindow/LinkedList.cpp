#include "Arduino.h"
#include "LinkedList.h"

SinglyLinkedList::SinglyLinkedList() {
    head = NULL;
    tail = NULL;
}

void SinglyLinkedList::append(int value) {
    Node* newNode = new Node;
    newNode -> data = value;
    newNode -> next = NULL;
    if (head == NULL) {
        head = newNode;
        tail = newNode;
    } else {
        head -> next = newNode;
        head = newNode;
    }
    size++;
}

int SinglyLinkedList::dropLast() {
    if (tail == NULL)
        return 0;

    // probably missing edge case here when list just has one element in it...

    Node* temp = tail;
    int value = temp -> data;
    
    tail = temp -> next;

    size--;

    delete temp;
    return value;
} 

int SinglyLinkedList::getSize() {
    return size;
}