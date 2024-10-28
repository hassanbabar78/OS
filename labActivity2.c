#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX 10           // Max size for the queue
#define TOTAL_ENTRIES 10 // Total number of entries to process
#define READER_COUNT 3   // Number of reader threads
#define WRITER_COUNT 2   // Number of writer threads

typedef struct {
    char data[MAX];
    int front;
    int rear;
    int count;
} Queue;

sem_t queueLock;  // Semaphore for queue access
sem_t tryRead;    // Semaphore to control read access

Queue queue;

// Function prototypes
void* reader(void* arg);
void* writer(void* arg);
void initQueue();
int isEmpty(Queue* q);
int isFull(Queue* q);
void enqueue(Queue* q, char value);
char dequeue(Queue* q);

int main() {
    sem_init(&queueLock, 0, 1);
    sem_init(&tryRead, 0, 1);

    initQueue();

    // Pre-fill the queue with fixed 'R' and 'W' values
    char initialValues[TOTAL_ENTRIES] = {'R', 'W', 'R', 'R', 'W', 'W', 'R', 'W', 'R', 'W'};
    for (int i = 0; i < TOTAL_ENTRIES; i++) {
        enqueue(&queue, initialValues[i]);
    }

    pthread_t readers[READER_COUNT];
    pthread_t writers[WRITER_COUNT];

    // Create reader and writer threads
    for (int i = 0; i < READER_COUNT; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
    }

    for (int i = 0; i < WRITER_COUNT; i++) {
        pthread_create(&writers[i], NULL, writer, NULL);
    }

    // Wait for threads to finish
    for (int i = 0; i < READER_COUNT; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < WRITER_COUNT; i++) {
        pthread_join(writers[i], NULL);
    }

    // Destroy semaphores
    sem_destroy(&queueLock);
    sem_destroy(&tryRead);

    return 0;
}

// Initialize the queue
void initQueue() {
    queue.front = 0;
    queue.rear = 0;
    queue.count = 0;
}

// Check if queue is empty
int isEmpty(Queue* q) {
    return q->count == 0;
}

// Check if queue is full
int isFull(Queue* q) {
    return q->count == MAX;
}

// Enqueue data to the queue
void enqueue(Queue* q, char value) {
    if (!isFull(q)) {
        q->data[q->rear] = value;
        q->rear = (q->rear + 1) % MAX;
        q->count++;
    }
}

// Dequeue data from the queue
char dequeue(Queue* q) {
    if (!isEmpty(q)) {
        char value = q->data[q->front];
        q->front = (q->front + 1) % MAX;
        q->count--;
        return value;
    }
    return '\0';  // Return null character if the queue is empty
}

// Reader thread function
void* reader(void* arg) {
    while (1) {
        sem_wait(&queueLock);  // Wait to access the queue
        if (!isEmpty(&queue)) {  // If the queue is not empty
            char data = dequeue(&queue);  // Dequeue data
            if (data == 'R') {
                printf("Reader: Read done from file\n");
            }
        } else {
            printf("Reader: Queue is empty, waiting...\n");
        }
        sem_post(&queueLock);  // Release queue access
        sleep(1);  // Simulate reading time
        break;  // Terminate the thread after one operation for demonstration
    }
    return NULL;
}

// Writer thread function
void* writer(void* arg) {
    while (1) {
        sem_wait(&queueLock);  // Wait to access the queue
        if (!isFull(&queue)) {  // If the queue is not full
            char value = 'W';  
            enqueue(&queue, value);  // Enqueue 'W' to simulate writing
            printf("Writer: Writing done\n");
        } else {
            printf("Writer: Queue is full, waiting...\n");
        }
        sem_post(&queueLock);  // Release queue access
        sleep(2);  // Simulate writing time
        break;  // Terminate the thread after one operation for demonstration
    }
    return NULL;
}
