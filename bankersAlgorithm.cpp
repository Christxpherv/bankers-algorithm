#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

pthread_mutex_t mutex;

// Function prototypes
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
void *customer_thread(void *arg);
bool safety_algorithm(int customer_num, int request[]);
void print_request_result(int customer_num, int request[], bool granted);

int main(int argc, char *argv[]) {
    // Initialize available resources
    if (argc != NUMBER_OF_RESOURCES + 1) {
        fprintf(stderr, "Usage: %s <available resources>\n", argv[0]);
        exit(1);
    }
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
    }

    // Initialize maximum, allocation, and need arrays (you can choose your initialization method)
    // For demonstration, let's initialize maximum randomly
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }

    // Initialize mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        exit(1);
    }

    // Create customer threads
    pthread_t customers[NUMBER_OF_CUSTOMERS];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        pthread_create(&customers[i], NULL, customer_thread, (void *)(intptr_t)i);
    }

    // Join threads
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}

int request_resources(int customer_num, int request[]) {
    // Lock mutex
    pthread_mutex_lock(&mutex);

    // Check if request exceeds need
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    // Check if request exceeds available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    // Try allocating resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    // Check for safety
    if (!safety_algorithm(customer_num, request)) {
        // Rollback if not safe
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}

int release_resources(int customer_num, int release[]) {
    // Lock mutex
    pthread_mutex_lock(&mutex);

    // Release resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}

void *customer_thread(void *arg) {
    int customer_num = (intptr_t)arg;
    while (1) {
        // Simulate request
        int request[NUMBER_OF_RESOURCES];
        // Generate random request
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            request[i] = rand() % (need[customer_num][i] + 1);
        }

        if (request_resources(customer_num, request) == 0) {
            print_request_result(customer_num, request, true);

            // Simulate release
            usleep(rand() % 1000000); // Sleep for random time
            int release[NUMBER_OF_RESOURCES];
            // Generate random release
            for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
                release[i] = rand() % (allocation[customer_num][i] + 1);
            }

            release_resources(customer_num, release);
            printf("Customer %d released resources.\n", customer_num);
        } else {
            print_request_result(customer_num, request, false);
        }

        usleep(rand() % 1000000); // Sleep for random time
    }
}

bool safety_algorithm(int customer_num, int request[]) {
    int temp_available[NUMBER_OF_RESOURCES];
    int temp_allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
    int temp_need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

    // Copy current state
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        temp_available[i] = available[i] - request[i];
    }
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            temp_allocation[i][j] = allocation[i][j];
            temp_need[i][j] = need[i][j];
        }
    }

    bool finish[NUMBER_OF_CUSTOMERS] = {false};

    // Check if the system is in a safe state
    for (int count = 0; count < NUMBER_OF_CUSTOMERS; count++) {
        bool found = false;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                bool can_allocate = true;
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (temp_need[i][j] > temp_available[j]) {
                        can_allocate = false;
                        break;
                    }
                }
                if (can_allocate) {
                    found = true;
                    finish[i] = true;
                    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                        temp_available[j] += temp_allocation[i][j];
                    }
                    break;
                }
            }
        }
        if (!found) {
            return false; // Deadlock detected
        }
    }
    return true; // System in safe state
}

void print_request_result(int customer_num, int request[], bool granted) {
    pthread_mutex_lock(&mutex);
    
    printf("Customer %d requested resources: ", customer_num);
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        printf("%d ", request[i]);
    }
    if (granted) {
        printf("and the request has been granted.\n");
    } else {
        printf("but the request has been denied.\n");
    }

    pthread_mutex_unlock(&mutex);
}

