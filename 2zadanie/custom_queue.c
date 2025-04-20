#include "custom_queue.h"
#include <stdlib.h>

node_t* g_queue_head = NULL;
node_t* g_queue_tail = NULL;

// pushes a client socket to the end of the queue
void enqueue(int *client_socket) {
	node_t* new_node = (node_t*)malloc(sizeof(node_t));
	new_node->m_client_socket = client_socket;
	new_node->m_next = NULL;

	if (g_queue_tail == NULL) {
		g_queue_head = new_node;
	} else {
		g_queue_tail->m_next = new_node;
	}
	g_queue_tail = new_node;
}

// returns the client socket from the head of the queue or NULL if the queue is empty
int* dequeue() {
	if (g_queue_head == NULL) {
		return NULL;
	}

	node_t* temp = g_queue_head;
	int* client_socket = g_queue_head->m_client_socket;
	g_queue_head = g_queue_head->m_next;

	if (g_queue_head == NULL) {
		g_queue_tail = NULL;
	}

	free(temp);
	return client_socket;
}