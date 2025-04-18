#include "custom_queue.h"
#include <stdlib.h>

node_t* queue_head = NULL;
node_t* queue_tail = NULL;

void enqueue(int *client_socket) {
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    new_node->client_socket = client_socket;
    new_node->next = NULL;

    if (queue_tail == NULL) {
        queue_head = new_node;
    } else {
        queue_tail->next = new_node;
    }
    queue_tail = new_node;
}


int* dequeue() {
    if (queue_head == NULL) {
        return NULL;
    }

    node_t* temp = queue_head;
    int* client_socket = queue_head->client_socket;
    queue_head = queue_head->next;

    if (queue_head == NULL) {
        queue_tail = NULL;
    }

    free(temp);
    return client_socket;
}