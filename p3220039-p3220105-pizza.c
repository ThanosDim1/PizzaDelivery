#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "p3220039-p3220105-pizza.h"
#include <time.h>

// Global variables for the number of cookers, oven, callers, and deliverers
int Cookers = N_COOK;
int Oven = N_OVEN;
int Callers = N_CALLERS;
int Deliverer = N_DELIVERER;

// Structure to represent an order
typedef struct order
{
    int MargaritaPizza;
    int PepperoniPizza;
    int SpecialPizza;
    int TotalPizzas;
} order;

// Function to initialize a mutex
void initializeMutex(pthread_mutex_t *mutex)
{
    if (pthread_mutex_init(mutex, NULL) != 0)
    {
        printf("ERROR: pthread_mutex_init() failed in main()\n");
        exit(-1);
    }
}

// Function to initialize a condition variable
void initializeCondition(pthread_cond_t *cond)
{
    if (pthread_cond_init(cond, NULL) != 0)
    {
        printf("ERROR: pthread_cond_init() failed in main()\n");
        exit(-1);
    }
}

// Function to acquire a lock
void acquireLock(pthread_mutex_t *mutex, int id, int *t)
{
    if (pthread_mutex_lock(mutex) != 0)
    {
        printf("ERROR: pthread_mutex_lock() failed in thread %d\n", id);
        exit(*t);
    }
}

// Function to release a lock
void releaseLock(pthread_mutex_t *mutex, int id, int *t)
{
    if (pthread_mutex_unlock(mutex) != 0)
    {
        printf("ERROR: pthread_mutex_unlock() failed in thread %d\n", id);
        exit(*t);
    }
}

// Function to destroy a mutex
void destroyLock(pthread_mutex_t *mutex)
{
    if (pthread_mutex_destroy(mutex) != 0)
    {
        printf("ERROR: pthread_mutex_destroy() failed in main()\n");
    }
}

// Function to destroy a condition variable
void destroyCond(pthread_cond_t *cond)
{
    if (pthread_cond_destroy(cond) != 0)
    {
        printf("ERROR: pthread_cond_destroy() failed in main()\n");
    }
}

// Function to generate a random number based on cumulative probabilities
int CumulativeProb(unsigned int *seed)
{
    float PP[3] = {0.35, 0.25, 0.4};              // Cumulative probabilities for different pizza types
    double uni = (double)rand_r(seed) / RAND_MAX; // Generate a random number between 0 and 1
    double cumulativeProbability = 0.0;
    for (int i = 0; i < 3; i++)
    {
        cumulativeProbability += PP[i];
        if (uni < cumulativeProbability)
        {
            return i; // Return the index corresponding to the selected pizza type
        }
    }
    return 0;
}

// Function to determine if a payment fails based on a probability
int PaymentFail(unsigned int *seed)
{
    double uni = (double)rand_r(seed) / RAND_MAX; // Generate a random number between 0 and 1
    if (uni < P_FAIL)                             // Check if the random number is less than the failure probability
        return 1;                                 // Payment fails
    else
        return 0; // Payment succeeds
}

void *services(void *t)
{

    // Declare variables for time tracking, waiting time, thread id, and order details
    struct timespec timeStarted, timeFinishedBaking, timeFinishedPacking, timeDelivered;
    int wait;
    int *id = (int *)t;
    order newOrder;

    // Generate a new seed for random number generation based on thread id
    unsigned int newseed = seed + *id;

    // Get the current time when the service starts
    clock_gettime(CLOCK_REALTIME, &timeStarted);

    acquireLock(&CallerLock, *id, t);

    // Wait while there are no available callers
    while (Callers == 0)
    {
        pthread_cond_wait(&AvailableCallerCond, &CallerLock);
    }
    Callers -= 1; // Reduce the count of available callers
    releaseLock(&CallerLock, *id, t);

    // Generate the number of pizzas for the order within a range
    newOrder.TotalPizzas = (rand_r(&newseed) % (N_ORDERHIGH - N_ORDERLOW + 1)) + N_ORDERLOW;

    // Initialize counts for different types of pizzas in the order
    newOrder.MargaritaPizza = 0;
    newOrder.PepperoniPizza = 0;
    newOrder.SpecialPizza = 0;

    // Generate each pizza type randomly for the order
    for (int i = 0; i < newOrder.TotalPizzas; i++)
    {
        switch (CumulativeProb(&newseed))
        {
        case 0:
            newOrder.MargaritaPizza++;
            break;
        case 1:
            newOrder.PepperoniPizza++;
            break;
        case 2:
            newOrder.SpecialPizza++;
            break;
        default:
            break;
        }
    }

    // Generate a random waiting time for payment
    wait = (rand_r(&newseed) % (T_PAYMENTHIGH - T_PAYMENTLOW + 1)) + T_PAYMENTLOW;
    sleep(wait);

    // Acquire locks for payment and output operations
    acquireLock(&PaymentLock, *id, t);
    acquireLock(&OutputLock, *id, t);

    // Check if payment fails
    if (PaymentFail(&newseed))
    {
        printf("Order %d: Payment failed. Order rejected.\n", *id);
        FailedOrders++;
        releaseLock(&OutputLock, *id, t);
        releaseLock(&PaymentLock, *id, t);

        // Release the caller after failure of payment
        acquireLock(&CallerLock, *id, t);
        Callers += 1;
        releaseLock(&CallerLock, *id, t);
        pthread_cond_broadcast(&AvailableCallerCond); // Signal availability of callers to other threads

        pthread_exit(NULL); // Exit thread if payment fails
    }
    else
    {
        // Payment successful
        printf("Order %d: Payment successful. Order accepted.\n ------------------------------------------\n", *id);
        AcceptedOrders++;

        // Update counts and revenue based on the order
        MargaritaPizzaCount += newOrder.MargaritaPizza;
        SpecialPizzaCount += newOrder.SpecialPizza;
        PeperoniPizzaCount += newOrder.PepperoniPizza;
        TotalRevenue += newOrder.MargaritaPizza * C_M + newOrder.PepperoniPizza * C_P + newOrder.SpecialPizza * C_S;

        // Release locks after updating
        releaseLock(&OutputLock, *id, t);
        releaseLock(&PaymentLock, *id, t);
    }

    // Release the caller after payment
    acquireLock(&CallerLock, *id, t);
    Callers += 1;
    releaseLock(&CallerLock, *id, t);
    pthread_cond_broadcast(&AvailableCallerCond); // Signal availability of callers to other threads

    acquireLock(&CookLock, *id, t);

    // Wait while there are no available cookers
    while (Cookers == 0)
    {
        pthread_cond_wait(&AvailableCookCond, &CookLock);
    }
    Cookers -= 1; // Reduce the count of available cookers
    releaseLock(&CookLock, *id, t);

    // Sleep for preparation time of all pizzas in the order
    sleep(T_PREP * newOrder.TotalPizzas);

    // Acquire lock for the oven
    acquireLock(&OvenLock, *id, t);

    // Wait while there are not enough ovens available
    while (Oven < newOrder.TotalPizzas)
    {
        pthread_cond_wait(&AvailableOvenCond, &OvenLock);
    }
    Oven -= newOrder.TotalPizzas; // Reduce the count of available ovens
    releaseLock(&OvenLock, *id, t);

    // Increase the count of available cookers after preparation
    acquireLock(&CookLock, *id, t);
    Cookers += 1;
    releaseLock(&CookLock, *id, t);
    pthread_cond_broadcast(&AvailableCookCond); // Signal availability of cookers to other threads

    // Sleep for baking time
    sleep(T_BAKE);

    clock_gettime(CLOCK_REALTIME, &timeFinishedBaking); // Record time after baking

    // Sleep for packing time of all pizzas in the order
    sleep(T_PACK * newOrder.TotalPizzas);

    clock_gettime(CLOCK_REALTIME, &timeFinishedPacking); // Record time after packing

    // Calculate order preparation time and print
    double orderPreparationTimeMinutes = ((timeFinishedPacking.tv_sec - timeStarted.tv_sec) + (double)(timeFinishedPacking.tv_nsec - timeStarted.tv_nsec) / 1e9) / 60.0 * 100;
    int lastTwoDigits = (int)(orderPreparationTimeMinutes * 100) % 100;
    if (lastTwoDigits >= 60)
    {
        orderPreparationTimeMinutes += 0.4;
    }

    acquireLock(&OutputLock, *id, t);
    printf("Order with number %d was prepared in %.2f minutes.\n ------------------------------------------\n", *id, orderPreparationTimeMinutes);
    releaseLock(&OutputLock, *id, t);

    // Acquire lock for the deliverer
    acquireLock(&DelivererLock, *id, t);

    // Wait while there are no available deliverers
    while (Deliverer == 0)
    {
        pthread_cond_wait(&AvailableDelivererCond, &DelivererLock);
        printf("Order %d: Waiting for deliverer.\n", *id);
    }

    // Increase the count of available ovens after delivery
    acquireLock(&OvenLock, *id, t);
    Oven += newOrder.TotalPizzas;
    releaseLock(&OvenLock, *id, t);
    pthread_cond_broadcast(&AvailableOvenCond); // Signal availability of ovens to other threads

    // Reduce the count of available deliverers
    Deliverer -= 1;
    releaseLock(&DelivererLock, *id, t);

    // Sleep for delivery time
    wait = (rand_r(&newseed) % (T_DELHIGH - T_DELLOW + 1)) + T_DELLOW;
    sleep(wait);
    clock_gettime(CLOCK_REALTIME, &timeDelivered); // Record time after delivery
    double orderCompletionTime = ((timeDelivered.tv_sec - timeStarted.tv_sec) + (double)(timeDelivered.tv_nsec - timeStarted.tv_nsec) / 1e9) / 60.0 * 100;
    lastTwoDigits = (int)(orderCompletionTime * 100) % 100;
    if (lastTwoDigits >= 60)
    {
        orderCompletionTime += 0.4;
    }

    // Print order delivery time
    acquireLock(&OutputLock, *id, t);
    printf("Order with number %d was delivered in %.2f minutes.\n ------------------------------------------\n", *id, orderCompletionTime);
    releaseLock(&OutputLock, *id, t);

    sleep(wait);

    // Increase the count of available deliverers
    acquireLock(&DelivererLock, *id, t);
    Deliverer += 1;
    releaseLock(&DelivererLock, *id, t);
    pthread_cond_broadcast(&AvailableDelivererCond); // Signal availability of deliverers to other threads

    // Calculate and update statistics
    double coolingTime = ((timeDelivered.tv_sec - timeFinishedBaking.tv_sec) + (double)(timeDelivered.tv_nsec - timeFinishedBaking.tv_nsec) / 1e9) / 60.0 * 100;
    acquireLock(&StatisticsLock, *id, t);
    if (orderCompletionTime > maxOrderCompletionTime)
        maxOrderCompletionTime = orderCompletionTime;
    if (coolingTime > maxCoolingTime)
        maxCoolingTime = coolingTime;
    orderCompletionTimeSum += orderCompletionTime;
    AverageCompletionTime = orderCompletionTimeSum / AcceptedOrders;
    lastTwoDigits = (int)(AverageCompletionTime * 100) % 100;
    if (lastTwoDigits >= 60)
    {
        AverageCompletionTime += 0.4;
    }
    coolingTimeSum += coolingTime;
    AverageCoolingTime = coolingTimeSum / AcceptedOrders;
    lastTwoDigits = (int)(AverageCoolingTime * 100) % 100;
    if (lastTwoDigits >= 60)
    {
        AverageCoolingTime += 0.4;
    }
    releaseLock(&StatisticsLock, *id, t);

    pthread_exit(t); // Exit the thread
}

int main(int argc, char *argv[])
{
    int customers;

    // Initialize mutexes and condition variables
    initializeMutex(&OutputLock);
    initializeMutex(&PaymentLock);
    initializeMutex(&CookLock);
    initializeMutex(&OvenLock);
    initializeMutex(&DelivererLock);
    initializeMutex(&StatisticsLock);
    initializeCondition(&AvailableCookCond);
    initializeCondition(&AvailableOvenCond);
    initializeCondition(&AvailableDelivererCond);

    // Check if correct number of command-line arguments provided
    if (argc != 3)
    {
        printf("Error: Not enough arguments provided. (Number of Customers and initial seed is required.)\n");
        exit(-1);
    }

    // Get number of customers and initial seed from command-line arguments
    customers = atoi(argv[1]);
    seed = atoi(argv[2]);

    // Validate number of customers
    if (customers <= 0)
    {
        printf("Error: Number of customers should be positive.\n");
        exit(-1);
    }

    // Allocate memory for thread IDs
    pthread_t *threads = (pthread_t *)malloc(customers * sizeof(pthread_t));
    if (threads == NULL)
    {
        printf("Out of memory!");
        exit(-1);
    }

    unsigned int newseed = seed;
    int i;
    int wait = 0;
    int *ids = (int *)malloc(customers * sizeof(int));
    if (ids == NULL)
    {
        printf("Out of memory!");
        exit(-1);
    }

    // Create threads for each customer
    for (i = 0; i < customers; i++)
    {
        ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, &services, &ids[i]) != 0)
        {
            printf("ERROR: Thread creation failed in main()\n");
            exit(-1);
        }

        // Generate random wait time between orders
        wait = (rand_r(&seed) % (T_ORDERHIGH - T_ORDERLOW + 1)) + T_ORDERLOW;
        sleep(wait);
    }

    // Wait for all threads to finish
    for (i = 0; i < customers; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            printf("ERROR: pthread_join() failed in main()\n");
        }
    }

    // Print statistics if there were accepted orders
    if (AcceptedOrders > 0)
    {
        printf("Statistics:\n");
        printf("------------------------------------------------\n");
        printf("| %-30s | %10d |\n", "Total revenue", TotalRevenue);
        printf("| %-30s | %10d |\n", "Margarita pizza count", MargaritaPizzaCount);
        printf("| %-30s | %10d |\n", "Pepperoni pizza count", PeperoniPizzaCount);
        printf("| %-30s | %10d |\n", "Special pizza count", SpecialPizzaCount);
        printf("| %-30s | %10d |\n", "Accepted orders count", AcceptedOrders);
        printf("| %-30s | %10d |\n", "Failed orders count", FailedOrders);
        printf("| %-30s | %10.2f |\n", "Average order completion time", AverageCompletionTime);
        printf("| %-30s | %10.2f |\n", "Max order completion time", maxOrderCompletionTime);
        printf("| %-30s | %10.2f |\n", "Average cooling time", AverageCoolingTime);
        printf("| %-30s | %10.2f |\n", "Max cooling time", maxCoolingTime);
        printf("------------------------------------------------\n");
    }
    else
    {
        printf("There was no successful transaction.\n");
    }

    // Destroy mutexes and condition variables
    destroyLock(&OutputLock);
    destroyLock(&PaymentLock);
    destroyLock(&CookLock);
    destroyLock(&OvenLock);
    destroyLock(&DelivererLock);
    destroyLock(&StatisticsLock);
    destroyCond(&AvailableCookCond);
    destroyCond(&AvailableOvenCond);
    destroyCond(&AvailableDelivererCond);

    // Free allocated memory
    free(threads);
    free(ids);

    return 1;
}