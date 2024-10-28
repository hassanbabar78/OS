#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX 10       
#define TOTAL_ENTRIES 3 

typedef struct {
    int data[MAX];
    int front;
    int rear;
    int count;  
} Queue;

sem_t readerLock;  
sem_t writerLock;  
sem_t queueLock;   
sem_t tryRead;     

int read_count = 0;
int write_count = 0;
int total_entries = 0; 

Queue queue;

void* reader(void* arg);
void* writer(void* arg);
void initQueue();
int isEmpty(Queue* q);
int isFull(Queue* q);
void enqueue(Queue* q, int value);
int dequeue(Queue* q);
int peek(Queue* q);

int main() {
    sem_init(&readerLock, 0, 1);
    sem_init(&writerLock, 0, 1);
    sem_init(&queueLock, 0, 1);
    sem_init(&tryRead, 0, 1);

    initQueue();

    pthread_t readers[3];  
    pthread_t writers[2];  
    
    for (int i = 0; i < 3; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
    }

    for (int i = 0; i < 2; i++) {
        pthread_create(&writers[i], NULL, writer, NULL);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(writers[i], NULL);
    }

    sem_destroy(&readerLock);
    sem_destroy(&writerLock);
    sem_destroy(&queueLock);
    sem_destroy(&tryRead);

    return 0;
}

void initQueue() {
    queue.front = 0;
    queue.rear = 0;
    queue.count = 0;  
}

int isEmpty(Queue* q) {
    return q->count == 0;
}

int isFull(Queue* q) {
    return q->count == MAX;
}

void enqueue(Queue* q, int value) {
    if (!isFull(q)) {
        q->data[q->rear] = value;
        q->rear = (q->rear + 1) % MAX; 
        q->count++;
        printf("Queue: Added %d to queue\n", value); 
        fflush(stdout); 
    }
}

int dequeue(Queue* q) {
    if (!isEmpty(q)) {
        int value = q->data[q->front];
        q->front = (q->front + 1) % MAX;  
        q->count--;
        printf("Queue: Removed %d from queue\n", value);  
        fflush(stdout);
        return value;
    }
    return -1; 
}

int peek(Queue* q) {
    if (!isEmpty(q)) {
        return q->data[q->front];
    }
    return -1; 
}

void* reader(void* arg) {
    while (1) {
        sem_wait(&readerLock); 
        read_count++;
        if (read_count == 1) {
            sem_wait(&tryRead);  
        }
        sem_post(&readerLock);  

        sem_wait(&queueLock);  

        if (!isEmpty(&queue)) {  
            int data = dequeue(&queue);  
            printf("Reader read data: %d\n", data);
            fflush(stdout);  
            printf("Reader reading the file...\n");
            fflush(stdout);  
        } else {
            printf("Reader is waiting, queue is empty...\n");
            fflush(stdout);  
        }

        sem_post(&queueLock);  

        sem_wait(&readerLock);  
        read_count--;
        if (read_count == 0) {  
            sem_post(&tryRead);  
        }
        sem_post(&readerLock);  

        if (total_entries >= TOTAL_ENTRIES) {
            break;
        }

        sleep(2);  
    }

    return NULL;
}

void* writer(void* arg) {
    while (1) {
        sem_wait(&writerLock);  
        write_count++;
        if (write_count == 1) {  
            sem_wait(&tryRead);  
        }
        sem_post(&writerLock);  

        sem_wait(&queueLock);  

        if (!isFull(&queue) && total_entries < TOTAL_ENTRIES) {  
            int data = rand() % 100;  
            enqueue(&queue, data);  
            total_entries++;  
            printf("Writer wrote data: %d\n", data);
            fflush(stdout);  
            printf("Writer writing to the file...\n");
            fflush(stdout);  
        } else {
            printf("Writer is waiting, queue is full or limit reached...\n");
            fflush(stdout);  
        }

        sem_post(&queueLock);  

        sem_wait(&writerLock);  
        write_count--;
        if (write_count == 0) {  
            sem_post(&tryRead);  
        }
        sem_post(&writerLock);  

        if (total_entries >= TOTAL_ENTRIES) {
            break;
        }

        sleep(3);  
    }

    return NULL;
}
