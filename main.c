#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Note for me: gcc your_code.c -o your_executable -pthread


// Bounded Queue struct
typedef struct BoundedQ {
    char** buffer;
    int max_size;
    int front;
    int next_pos;
    int count;
    int f;
    pthread_mutex_t mutex;
    pthread_cond_t full;
    pthread_cond_t empty;
} BoundedQ;

BoundedQ* createBoundedQ(int max_size) {
    BoundedQ* queue = (BoundedQ*)malloc(sizeof(BoundedQ));
    if (queue == NULL) {
        // Allocation failed
        printf("malloc failed");
    }
    queue->buffer = (char**)malloc(max_size * sizeof(char*));
    if (queue->buffer == NULL) {
        // Allocation failed
        printf("malloc failed");
    }
    queue->max_size = max_size;
    queue->front = 0;
    queue->next_pos = -1;
    queue->count = 0;
    queue->f = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->full, NULL);
    pthread_cond_init(&queue->empty, NULL);
    return queue;
}

void destroyBoundedQ(BoundedQ* queue) {
    free(queue->buffer);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->full);
    pthread_cond_destroy(&queue->empty);
    free(queue);
}

void boundedQ_insert(BoundedQ* queue, const char* value) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->count >= queue->max_size) {
        pthread_cond_wait(&queue->full, &queue->mutex);
    }
    queue->next_pos = (queue->next_pos + 1) % queue->max_size;
    queue->buffer[queue->next_pos] = strdup(value);
    queue->count++;
    pthread_cond_signal(&queue->empty);
    pthread_mutex_unlock(&queue->mutex);
}

char* boundedQ_remove(BoundedQ* queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->count <= 0 && queue->f == 1) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    while (queue->count <= 0) {
        pthread_cond_wait(&queue->empty, &queue->mutex);
    }
    char* s = queue->buffer[queue->front];
    queue->front = (queue->front + 1) % queue->max_size;
    queue->count--;
    pthread_cond_signal(&queue->full);
    pthread_mutex_unlock(&queue->mutex);
    if (strcmp(s, "DONE") == 0) {
        queue->f = 1;
    }
    return s;
}

typedef struct Node {
    char* data;
    struct Node* next;
} Node;

typedef struct UnboundedQ {
    Node* front;
    Node* next_pos;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
} UnboundedQ;

UnboundedQ* createUnboundedQ() {
    UnboundedQ* queue = (UnboundedQ*)malloc(sizeof(UnboundedQ));
    if (queue == NULL) {
        // Allocation failed
        printf("malloc failed");
    }
    queue->front = NULL;
    queue->next_pos = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->empty, NULL);
    return queue;
}

void destroyUnboundedQ(UnboundedQ* queue) {
    if (queue == NULL) return;
    Node* current = queue->front;
    while (current != NULL) {
        Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->empty);
    free(queue);
}

void unboundedQ_insert(UnboundedQ* queue, const char* value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        // Allocation failed
        printf("malloc failed");
    }
    newNode->data = strdup(value);
    newNode->next = NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->next_pos == NULL) {
        // Queue is empty
        queue->front = newNode;
        queue->next_pos = newNode;
    } else {
        queue->next_pos->next = newNode;
        queue->next_pos = newNode;
    }

    pthread_cond_signal(&queue->empty);
    pthread_mutex_unlock(&queue->mutex);
}

char* unboundedQ_remove(UnboundedQ* queue) {
    pthread_mutex_lock(&queue->mutex);

    while (queue->front == NULL) {
        pthread_cond_wait(&queue->empty, &queue->mutex);
    }

    Node* removedNode = queue->front;
    char* removedString = removedNode->data;

    queue->front = queue->front->next;
    if (queue->front == NULL) {
        queue->next_pos = NULL; // Queue is now empty
    }

    free(removedNode);
    pthread_mutex_unlock(&queue->mutex);

    return removedString;
}



// Producer
typedef struct {
    int id;
    int numberOfProducts;
    BoundedQ* queue;
} ProducerArgs;

void* producer(void* arguments) {
    char final[50];

    // Get function args
    ProducerArgs* args = (ProducerArgs*)arguments;
    int id = args->id;
    int numberOfProducts = args->numberOfProducts;
    BoundedQ* queue = args->queue;

    // Init articles number to 0
    int SPORTS = 0, NEWS = 0, WEATHER = 0;

    for (int i = 0; i < numberOfProducts; ++i) {
        // Pick a random type of product
        int type = rand() % 3;
        // Create a product
        char product[50];
        switch (type) {
            case 0:
                sprintf(product, "SPORTS %d", SPORTS);
                SPORTS++;
                break;
            case 1:
                sprintf(product, "NEWS %d", NEWS);
                NEWS++;
                break;
            case 2:
                sprintf(product, "WEATHER %d", WEATHER);
                WEATHER++;
                break;
            default:
                break;
        }
        sprintf(final, "producer %d %s", id, product);
        // Insert the new string to the current queue
        boundedQ_insert(queue, final);
    }
    boundedQ_insert(queue, "DONE");
    free(args);
    pthread_exit(NULL);
}

// Dispatcher
typedef struct {
    BoundedQ** producers;
    UnboundedQ** co_editors;
} DispatcherArgs;


void* dispatcher(void* arguments) {
    DispatcherArgs* args = (DispatcherArgs*)arguments;
    BoundedQ** producers_queues = args->producers;
    UnboundedQ** co_editors_queues = args->co_editors;

    int current_queue = 0;
    int ended_queues = 0;
    while (ended_queues < 3) {
        // Remove one string from each producer queue each time
        char* removed_string = boundedQ_remove(producers_queues[current_queue]);
        if (removed_string == NULL) {
            free(removed_string);
            current_queue = (current_queue + 1) % 3;
            continue;
        }
        if (strcmp(removed_string, "DONE") == 0) {
            current_queue = (current_queue + 1) % 3;
            ended_queues++;
            free(removed_string);
            continue;
        }
        if (strstr(removed_string, "SPORTS") != NULL) {
            unboundedQ_insert(co_editors_queues[0], removed_string);

        }
        if (strstr(removed_string, "NEWS") != NULL) {
            unboundedQ_insert(co_editors_queues[1], removed_string);
        }
        if (strstr(removed_string, "WEATHER") != NULL) {
            unboundedQ_insert(co_editors_queues[2], removed_string);
        }
        current_queue = (current_queue + 1) % 3;
        free(removed_string);
    }
    // When the Dispatcher receives a "DONE" message from all Producers, it sends a "DONE" message through each of its queues.
    for (int i = 0; i < 3; ++i) {
        unboundedQ_insert(co_editors_queues[i], "DONE");
    }
    free(args);

    pthread_exit(NULL);
}

typedef struct {
    int id;
    UnboundedQ * co_editors_queue;
    BoundedQ* screen_queue;
} CoEditorArgs;

void* co_editor(void* arg) {
    CoEditorArgs* args = (CoEditorArgs*)arg;
    UnboundedQ * co_editors_queue = args->co_editors_queue;
    BoundedQ* screen_queue = args->screen_queue;

    while (1) {
        char* s = unboundedQ_remove(co_editors_queue);
        if (s == NULL) {
            free(s);
            continue;
        }
        if (strcmp(s, "DONE") == 0) {
            free(s);
            break;
        }
        usleep(100 * 1000);
        boundedQ_insert(screen_queue, s);
        free(s);
    }
    boundedQ_insert(screen_queue, "DONE");
    free(args);
    pthread_exit(NULL);
}

int main(int argc, char const* argv[]) {
    if (argc < 2) {
        printf("Too few arguments.\n");
        return 1;
    }

    pthread_t* producers_threads;
    int num_producers = 0;
    int co_editors_queues_size;

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Failed to open configuration file.\n");
        return 1;
    }

    char line[100];
    int id, numberOfProducts, queue_size;


    // Move on the file and count the number of producers (for allocation)
    while (fgets(line, sizeof(line), file)) {

        if (sscanf(line, "%d", &id) == 1) {
            if (fgets(line, sizeof(line), file) && sscanf(line, "%d", &numberOfProducts) == 1) {
                if (fgets(line, sizeof(line), file) && sscanf(line, "%d", &queue_size) == 1) {
                    num_producers++;
                }
            }
            else {
                co_editors_queues_size = id;
            }
        }
    }

    // Reset the file pointer to the beginning of the file
    rewind(file);

    producers_threads = (pthread_t*)malloc(num_producers * sizeof(pthread_t));
    BoundedQ** producers_queues = (BoundedQ**)malloc(num_producers * sizeof(BoundedQ*));
    UnboundedQ ** co_editors_queues = (UnboundedQ**)malloc(3 * sizeof(UnboundedQ*));
    if ( producers_threads == NULL || producers_queues == NULL || co_editors_queues == NULL) {
        // Allocation failed
        printf("malloc failed");
    }


    // Get line from configuration file, put the strings into parameters
    int index = 0;
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%d", &id) == 1) {
            if (fgets(line, sizeof(line), file) && sscanf(line, "%d", &numberOfProducts) == 1) {
                if (fgets(line, sizeof(line), file) && sscanf(line, "%d", &queue_size) == 1) {
                    producers_queues[index] = (BoundedQ*)malloc(sizeof(BoundedQ));
                    if ( producers_queues[index] == NULL) {
                        // Allocation failed
                        printf("malloc failed");
                    }
                    producers_queues[index] = createBoundedQ(queue_size);

                    // Create producer args object then exec producer()
                    ProducerArgs* producer_args = (ProducerArgs*)malloc(sizeof(ProducerArgs));
                    if ( producer_args == NULL) {
                        // Allocation failed
                        printf("malloc failed");
                    }
                    producer_args->id = id;
                    producer_args->numberOfProducts = numberOfProducts;
                    producer_args->queue = producers_queues[index];
                    pthread_create(&(producers_threads[index]), NULL, producer, (void*)producer_args);
                    index++;
                }
            }
        }
    }
    fclose(file);
    // Create 3 unbounded co-editors queues
    for (int i = 0; i < 3; ++i) {
        co_editors_queues[i] = (UnboundedQ*)malloc(sizeof(UnboundedQ));
        if ( co_editors_queues[i] == NULL) {
            // Allocation failed
            printf("malloc failed");
        }
        co_editors_queues[i] = createUnboundedQ();
    }
    // Create dispatcher args object then exec dispatcher()
    DispatcherArgs * dispatcher_args = (DispatcherArgs*)malloc(sizeof(DispatcherArgs));
    if ( dispatcher_args == NULL) {
        // Allocation failed
        printf("malloc failed");
    }
    dispatcher_args->producers = producers_queues;
    dispatcher_args->co_editors = co_editors_queues;
    pthread_t dispatcher_thread;
    pthread_create(&dispatcher_thread, NULL, dispatcher, (void*)dispatcher_args);



    BoundedQ* screen_queue = createBoundedQ(co_editors_queues_size);
    // Create co_editor args object then exec co_editor()
    CoEditorArgs* co_editor_args[3];
    pthread_t co_editor_threads[3];
    for (int i = 0; i < 3; i++) {
        co_editor_args[i] = (CoEditorArgs*)malloc(sizeof(CoEditorArgs));
        if ( co_editor_args[i] == NULL) {
            // Allocation failed
            printf("malloc failed");
        }
        co_editor_args[i]->id = i;
        co_editor_args[i]->co_editors_queue = co_editors_queues[i];
        co_editor_args[i]->screen_queue = screen_queue;
        pthread_create(&(co_editor_threads[i]), NULL, co_editor, (void*)(co_editor_args[i]));
    }

    int ended = 0;
    while (ended != 3) {
        char* s = boundedQ_remove(screen_queue);

        if (s == NULL) {
            free(s);
            continue;
        }

        if (strcmp(s, "DONE") == 0) {
            free(s);
            ended++;
            continue;
        }
        printf("%s\n", s);
        free(s);
    }

    // Clean
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers_threads[i], NULL);
        destroyBoundedQ(producers_queues[i]);
    }
    free(producers_queues);
    free(producers_threads);

    for (int i = 0; i < 3; i++) {
        pthread_join(co_editor_threads[i], NULL);
        destroyUnboundedQ(co_editors_queues[i]);

    }
    free(co_editors_queues);
    pthread_join(dispatcher_thread, NULL);
    destroyBoundedQ(screen_queue);
    printf("DONE\n");


    return 0;

}

