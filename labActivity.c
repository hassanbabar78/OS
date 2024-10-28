#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#define MAX 10  // Max size for the queue (shared resource)

// Queue structure (shared resource)
typedef struct {
    int data[MAX];
    int front;
    int rear;
} Queue;

// Mutex and Semaphore declarations
pthread_mutex_t readerLock;
pthread_mutex_t writerLock;
pthread_mutex_t queueLock;
sem_t tryRead;  // Semaphore for controlling read access when writers are writing

int read_count = 0;
int write_count = 0;

Queue queue;

// Function prototypes
void* reader(void* arg);
void* writer(void* arg);
void initQueue();
int isEmpty(Queue* q);
void enqueue(Queue* q, int value);
int dequeue(Queue* q);
int peek(Queue* q);

// Main function
int main() {
    // Initialize mutexes and semaphore
    pthread_mutex_init(&readerLock, NULL);
    pthread_mutex_init(&writerLock, NULL);
    pthread_mutex_init(&queueLock, NULL);
    sem_init(&tryRead, 0, 1);  // Binary semaphore

    initQueue();

    // Create reader and writer threads
    pthread_t r1, r2, w1, w2;
    
    pthread_create(&r1, NULL, reader, NULL);
    pthread_create(&r2, NULL, reader, NULL);
    pthread_create(&w1, NULL, writer, NULL);
    pthread_create(&w2, NULL, writer, NULL);

    // Wait for threads to finish
    pthread_join(r1, NULL);
    pthread_join(r2, NULL);
    pthread_join(w1, NULL);
    pthread_join(w2, NULL);

    // Destroy mutexes and semaphore
    pthread_mutex_destroy(&readerLock);
    pthread_mutex_destroy(&writerLock);
    pthread_mutex_destroy(&queueLock);
    sem_destroy(&tryRead);

    return 0;
}

// Queue initialization
void initQueue() {
    queue.front = 0;
    queue.rear = 0;
}

// Check if queue is empty
int isEmpty(Queue* q) {
    return q->front == q->rear;
}

// Enqueue data to the queue
void enqueue(Queue* q, int value) {
    if (q->rear < MAX) {
        q->data[q->rear++] = value;
    }
}

// Dequeue data from the queue
int dequeue(Queue* q) {
    if (!isEmpty(q)) {
        return q->data[q->front++];
    }
    return -1;  // Return -1 if the queue is empty
}

// Peek the front of the queue
int peek(Queue* q) {
    if (!isEmpty(q)) {
        return q->data[q->front];
    }
    return -1;  // Return -1 if the queue is empty
}

// Reader thread function
void* reader(void* arg) {
    pthread_mutex_lock(&readerLock);  // Lock the reader count section
    read_count++;
    if (read_count == 1) {  // First reader locks the writer
        sem_wait(&tryRead);  // Block writers
    }
    pthread_mutex_unlock(&readerLock);

    pthread_mutex_lock(&queueLock);  // Lock the queue

    if (peek(&queue) != -1) {  // If the queue is not empty
        int data = dequeue(&queue);  // Dequeue data
        printf("Reader read data: %d\n", data);
    }

    // Simulate reading the file
    printf("Reader reading the file...\n");

    pthread_mutex_unlock(&queueLock);  // Unlock the queue

    pthread_mutex_lock(&readerLock);
    read_count--;
    if (read_count == 0) {  // Last reader unlocks the writer
        sem_post(&tryRead);  // Allow writers
    }
    pthread_mutex_unlock(&readerLock);

    return NULL;
}

// Writer thread function
void* writer(void* arg) {
    pthread_mutex_lock(&writerLock);  // Lock the writer section
    write_count++;
    if (write_count == 1) {  // First writer locks the readers
        sem_wait(&tryRead);  // Block readers
    }
    pthread_mutex_unlock(&writerLock);

    pthread_mutex_lock(&queueLock);  // Lock the queue

    if (peek(&queue) == -1) {  // If the queue is empty
        int data = rand() % 100;  // Generate random data
        enqueue(&queue, data);  // Enqueue data
        printf("Writer wrote data: %d\n", data);
    }

    // Simulate writing the file
    printf("Writer writing to the file...\n");

    pthread_mutex_unlock(&queueLock);  // Unlock the queue

    pthread_mutex_lock(&writerLock);
    write_count--;
    if (write_count == 0) {  // Last writer unlocks the readers
        sem_post(&tryRead);  // Allow readers
    }
    pthread_mutex_unlock(&writerLock);

    return NULL;
}
