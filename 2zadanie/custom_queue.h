#ifndef CUSTOM_QUEUE_H
#define CUSTOM_QUEUE_H

	typedef struct node {
		struct node* m_next;
		int *m_client_socket;
	} node_t;

	// function prototypes for custom queue
	void enqueue(int *client_socket);
	int* dequeue();

#endif 