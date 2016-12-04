#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>
#include <stdatomic.h>

// IMPORTANT VARIABLES
#define NUM_THREADS 3
#define BUFFER_SIZE 4

// Define the semaphore object
typedef struct sem_t {
	int count; // Number of threads that are currently blocked
	int pending_posts; // Number of pending posts
	pthread_mutex_t mutex; // Mutex for the semaphore
} sem_t;

// Initialize a semaphore to some default values
int sem_init(sem_t *sem, int ignore, int init) {
	sem->count = init;
	sem->pending_posts = 0;
	pthread_mutex_init(&(sem->mutex), NULL); 
}

// Increments (unlocks) the semaphore pointed to by sem
int sem_post(sem_t *sem) {
	pthread_mutex_lock( &(sem->mutex) );
	sem->count++;
	if (sem->count <= 0) {
		sem->pending_posts++;
	}
	pthread_mutex_unlock( &(sem->mutex) );
}

// Blocks a thread until sem->count is positive
// or sem->pending_posts is greater than zero
void block(sem_t *sem) {
	while (1) {
		pthread_mutex_lock( &(sem->mutex) );
		if (sem->count >= 0 || sem->pending_posts > 0) {
			sem->pending_posts--;
			pthread_mutex_unlock( &(sem->mutex) );
			return;
		}
		pthread_mutex_unlock( &(sem->mutex) );
		// Sleep and check later if count >=0
		usleep(100);
	}
}

// Decrements (locks) the semaphore pointed to by sem
int sem_wait(sem_t *sem) {
	pthread_mutex_lock( &(sem->mutex) );
	sem->count--;
	if (sem->count < 0) {
		// We must release lock, or no one can ever call sem_post() for us.
		pthread_mutex_unlock( &(sem->mutex) );
		block(sem);
		pthread_mutex_lock( &(sem->mutex) );
	}
	pthread_mutex_unlock( &(sem->mutex) );
}

// Type definition for all the required parts of a Buffer
typedef struct Buffer {
	int buf[BUFFER_SIZE]; // Array serving as the buffer
	unsigned int first_occupied_slot; // Index of occupied empty slot in array
	unsigned int first_empty_slot; // Index of first empty slot in array
	sem_t sem_producer;  // Number of empty slots available
	sem_t sem_consumer;  // Number of items in the buffer
	pthread_mutex_t mut_buf; // Mutex for buffer
} Buffer;

// Declare the two buffers that will be randomly chosen by consumer/producer
Buffer buffer1;
Buffer buffer2;

// Add int to the buffer
void add(Buffer *buf, int val) {
	// Wait for empty slots
	sem_wait(&buf->sem_producer);
	// Lock the mutex for modifying buf
	pthread_mutex_lock(&buf->mut_buf);
	
	// Store given val in the buffer
	atomic_store(&buf->buf[buf->first_empty_slot], val);
	// Increment the first empty slot
	unsigned int ii = atomic_fetch_add(&buf->first_empty_slot, 1);
	
	// Roll back to beginning of buffer
	if (ii >= BUFFER_SIZE) {
		atomic_store(&buf->first_empty_slot, 0);
	}
	
	pthread_mutex_unlock(&buf->mut_buf);
	// Tell the consumer there's a new work item
	sem_post(&buf->sem_consumer);
}

// Remove and return int from the buffer
int rem(Buffer *buf) {
	// Wait for empty slots
	sem_wait(&buf->sem_consumer);
	// Lock the mutex for modifying buf
	pthread_mutex_lock(&buf->mut_buf);
	
	// Fetch and store the buffer value to be returned
	int val = atomic_load(&buf->buf[buf->first_occupied_slot]);
	// Increment the first occupied slot
	unsigned int ii = atomic_fetch_add(&buf->first_occupied_slot, 1);
	
	// Roll back to beginning of buffer
	if (ii >= BUFFER_SIZE) {
		atomic_store(&buf->first_occupied_slot, 0);
	}
	
	pthread_mutex_unlock(&buf->mut_buf);
	// Tell the producer there's a new work item
	sem_post(&buf->sem_producer);
	return val;
}

// Producer thread
void *producer(void *arg) {
	int work_item = 1;
	while (1) {
		// Sleep for a random time
		usleep( 5000 * (rand() % 5 ));
		
		// Randomly generate a boolean to pick which buffer to use
		if ((rand() % 2) == 0) {
			add(&buffer1, work_item++);
		} else {
			add(&buffer2, work_item++);
		}
	}
}

// Consumer thread
void *consumer(void *arg) {
	while (1) {
		int work_item;
		// Sleep for a random time
		usleep( 5000 * (rand() % 5 ));
		
		// Randomly generate a boolean to pick which buffer to use
		if ((rand() % 2) == 0) {
			work_item = rem(&buffer1);  
		} else {
			work_item = rem(&buffer2);  
		}

		printf("%d ", work_item);
		fflush(stdout);
	}
}

// Initialize the values of a Buffer object
void buffer_init(Buffer *buf) {
	buf->first_occupied_slot = 0;
	buf->first_empty_slot = 0;
	sem_init(&buf->sem_producer, 0, BUFFER_SIZE);
	sem_init(&buf->sem_consumer, 0, 0);
	pthread_mutex_init(&buf->mut_buf, NULL);
}

int main() {
	// Init the two buffers
	buffer_init(&buffer1);
	buffer_init(&buffer2);
	
	// Create an array of producer and consumer threads
	pthread_t prod_threads[NUM_THREADS];
	pthread_t con_threads[NUM_THREADS];
	for (int ii = 0; ii < NUM_THREADS; ++ii) {
		pthread_create(&(prod_threads[ii]), 0, producer, 0);
		pthread_create(&(con_threads[ii]), 0, consumer, 0);
	}
	
	// Don't let the primary thread exit 
	while (1) {
		sleep(10);
	}
}
