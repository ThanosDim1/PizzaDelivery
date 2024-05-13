#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "PizzaDelivery.h"
#include <time.h>

int Cookers = N_COOK;
int Oven = N_OVEN;
int Callers = N_TEL;
int Deliverer = N_DELIVERER;
int Tele = N_TEL;

int priority = 1;
int cookPriority = 1;
int ovenPriority = 1;
int packerPriority = 1;
int delivererPriority = 1;

double maxOrderCompletionTime = 0;
double maxCoolingTime = 0;
double orderCompletionTimeSum = 0;
double coolingTimeSum = 0;

typedef struct order {
    int MargaritaPizza;
    int PepperoniPizza;
    int SpecialPizza;
    int TotalPizzas;
}order;


// Initializes mutex, prints message if initialization failed and exits with code -1. Used in main() only.
void initializeMutex(pthread_mutex_t *mutex){
    if (pthread_mutex_init(mutex, NULL) != 0){
        printf("ERROR: pthread_mutex_init() failed in main()\n");
        exit(-1);
    }
}

// Initializes conds, prints message if initialization failed and exits with code -1. Used in main() only.
void initializeCondition(pthread_cond_t *cond){
    if (pthread_cond_init(cond, NULL) != 0){
        printf("ERROR: pthread_cond_init() failed in main()\n");
        exit(-1);
    }
}

// Acquires lock, if acquisition fails then id of thread is printed and program exits.
void acquireLock(pthread_mutex_t *mutex, int oid, void* t){
    if (pthread_mutex_lock(mutex) != 0){
        printf("ERROR: pthread_mutex_lock() failed in thread %d\n", oid);
        exit(t);
    }
}

// Releases lock, if release fails then id of thread is printed and program exits.
void releaseLock(pthread_mutex_t *mutex, int oid, void* t){
    if (pthread_mutex_unlock(mutex) != 0){
        printf("ERROR: pthread_mutex_unlock() failed in thread %d\n", oid);
        exit(t);
    }
}

// Destroys lock, if destruction fails it prints a message and exits with code -1. Used in main() only.
void destroyLock(pthread_mutex_t *mutex){
    if (pthread_mutex_destroy(mutex) != 0){
        printf("ERROR: pthread_mutex_destroy() failed in main()\n");
    } 
}

// Destroys cond, if destruction fails it prints a message and exits with code -1. Used in main() only.
void destroyCond(pthread_cond_t *cond){
    if (pthread_cond_destroy(cond) != 0){
        printf("ERROR: pthread_cond_destroy() failed in main()\n");
    } 
}

int CumulativeProb(unsigned int* seed){
    
    float PP[3] = {0.35, 0.25, 0.4};
    // Generate a random number between 0 and 1
    double uni = (double)rand_r(seed) / RAND_MAX;

    // Cumulative probability
    double cumulativeProbability = 0.0;

    // Iterate over each pizza and its probability
    for (int i = 0; i < 3; i++) {
        cumulativeProbability += PP[i];
        if (uni < cumulativeProbability) {
            return i;
        }
    }

    return 0;
}

// Returns 1 with a probability of p and 0 with a probability of 1-p.
int PaymentFail(unsigned int* seed){
    // Generate random probability between 0 and 1
    double uni = (double) rand_r(seed) / RAND_MAX;

    // Check if the generated probability indicates payment failure
    if (uni < P_FAIL) 
        return 1;
    else 
        return 0;
}




void *simulateServiceFunc(void *t){

    struct timespec timeStarted, timeFinishedBaking, timeFinishedPacking, timeDelivered; 
    int orderPriority, wait;
    int* id = (int *) t; 
    order newOrder;

    // Seed for thread to use rand_r().
    unsigned int newseed = seed + *id;

    // Get starting time.
    clock_gettime(CLOCK_REALTIME, &timeStarted);

    /* PART 1*/

    // Get total number of pizzas to order.
    newOrder.TotalPizzas = (rand_r(&newseed) % (N_ORDERHIGH - N_ORDERLOW + 1)) + N_ORDERLOW;
    newOrder.MargaritaPizza=0;
    newOrder.PepperoniPizza=0;
    newOrder.SpecialPizza=0;
    // Get number of special pizzas.
    int i;
    for (int i = 0; i< newOrder.TotalPizzas; i++){
        switch (CumulativeProb(newseed))
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
    
    /* END OF PART 1*/


    /* PART 2*/

    // Simulate time to attempt transaction.
    wait = (rand_r(&newseed) % (T_PAYMENTHIGH - T_PAYMENTLOW + 1)) + T_PAYMENTLOW;
    sleep(wait);

    // Acquire lock to update transaction counts and count of pizzas variables.
    acquireLock(&PaymentLock, *id, t);
    
    // Acquire print lock to notify about transaction success/failure.
    acquireLock(&OutputLock, *id, t);

    // With probability P_FAIL transaction fails.
    if (PaymentFail(&newseed)){
        
        printf("Order %d: Payment failed.\n", *id);

        FailedOrders++;
        releaseLock(&OutputLock, *id,t);
        releaseLock(&PaymentLock, *id,t);

        pthread_exit(NULL);

    }
    else{
     printf("Order %d: Payment successful.\n", *id);
        AcceptedOrders++;

        MargaritaPizzaCount += newOrder.MargaritaPizza;
        SpecialPizzaCount += newOrder.SpecialPizza;
        PeperoniPizzaCount += newOrder.PepperoniPizza;
        
        TotalRevenue += newOrder.MargaritaPizza * C_M + newOrder.PepperoniPizza * C_P + newOrder.SpecialPizza * C_S;

        releaseLock(OutputLock, *id,t);
        releaseLock(PaymentLock, *id,t);
    
        orderPriority = priority++; // assign order priority id to order, happens here because failed orders should not receive one.
    }
        
    /* END OF PART 2*/


    /* START OF PART 3*/
    
    // Enforce FCFS policy.
    acquireLock(&cookPriorityLock, *id, t);
    while(cookPriority != orderPriority){
        pthread_cond_wait(&cookPriorityCond, &cookPriorityLock);
    }
    
    while (Cookers == 0){
        pthread_cond_wait(AvailableCookCond,CookLock);
    }

    // Now get a cook.
    acquireLock(&CookLock, *id, t);
    Cookers -= 1;

    releaseLock(&CookLock, *id, t);
    cookPriority += 1;
    releaseLock(&cookPriorityLock, *id, t);
    pthread_cond_broadcast(&cookPriorityCond); // Notify other threads that they can "compete" for a cook. (Obv. thread with higher priority)

    // Simulate pizza preparation time.
    sleep(T_PREP * newOrder.TotalPizzas);

    /* END OF PART 3*/


    /* START OF PART 4 */
    
    // Enforce FCFS policy.
    acquireLock(&ovenPriorityLock, *id, t);
    while(orderPriority != ovenPriority){
        pthread_cond_wait(&ovenPriorityCond, &ovenPriorityLock);
    }

    // Get enough ovens. (one for each pizza ordered)
    acquireLock(&OvenLock, *id, t);

    // May re-wait multiple times.
    while( Oven < newOrder.TotalPizzas){
        pthread_cond_wait(&AvailableOvenCond, &OvenLock);
    }
    
    Oven -= newOrder.TotalPizzas;
    releaseLock(&OvenLock, *id, t);

    ovenPriority += 1;
    
    releaseLock(&ovenPriorityLock, *id, t);
    pthread_cond_broadcast(&ovenPriorityCond); // Notify other threads that they can "compete" for ovens. (Obv. thread with higher priority)
    
    // After ovens have been acquired, release cook.
    acquireLock(&CookLock, *id, t);
    Cookers += 1;
    releaseLock(&CookLock, *id, t);
    pthread_cond_signal(&AvailableCookCond); // At most one thread is waiting for cook at each point in time. (FCFS Policy)

    // Simulate baking time.
    sleep(T_BAKE);

    // Get time when baking finished.
    clock_gettime(CLOCK_REALTIME, &timeFinishedBaking);

    /* END OF PART 4 */


    /* START OF PART 5 */

    // Enforce FCFS policy.
   
    // Simulate packing time.
    sleep(T_PACK * newOrder.TotalPizzas);
    
    // Get time finished packing.
    clock_gettime(CLOCK_REALTIME, &timeFinishedPacking);

    // Release ovens now that packing is complete.
    acquireLock(&OvenLock, *id, t);
    Oven += newOrder.TotalPizzas;
    releaseLock(&OvenLock, *id, t);
    pthread_cond_signal(&AvailableOvenCond); // At most one thread is waiting for ovens at each point in time (FCFS Policy).

    // Print message stating total time for order with <oid> to get ready. (time from customer order up to time packing was finished)
    double orderPreparationTimeMinutes = ((timeFinishedPacking.tv_sec - timeStarted.tv_sec) + (double)(timeFinishedPacking.tv_nsec - timeStarted.tv_nsec) / 1e9) / 60.0;
    acquireLock(&OutputLock, *id, t);
    printf("Order with number %d was prepared in %f minutes.\n", *id, orderPreparationTimeMinutes);
    releaseLock(&OutputLock, *id, t);

    /* END OF PART 5 */


    /* START OF PART 6 */

    // Enforce FCFS policy.
    acquireLock(&delivererPriorityLock, *id, t);
    while (orderPriority != delivererPriority){
        pthread_cond_wait(&delivererPriorityCond, &delivererPriorityLock);
    }

    // Acquire deliverer.
    acquireLock(&DelivererLock, *id, t);
    while (Deliverer == 0){
        pthread_cond_wait(&AvailableDelivererCond, &DelivererLock);
    }
    Deliverer -= 1;
    releaseLock(&DelivererLock, *id, t);

    // Once deliverer has been acquired.
    delivererPriority += 1;
    releaseLock(&delivererPriorityLock, *id, t);
    pthread_cond_broadcast(&delivererPriorityCond); // Notify other threads that they can "compete" for a deliverer. (Obv. thread with higher priority)

    // Simulate time for deliverer to reach customer.
    wait = (rand_r(&newseed) % (T_DELHIGH - T_DELLOW + 1)) + T_DELLOW;
    sleep(wait);
    
    // Get time package was delivered.
    clock_gettime(CLOCK_REALTIME, &timeDelivered);

    // Print message stating total time for order with <oid> to be delivered. (time from customer order up to delivery)
    double orderCompletionTime = ((timeDelivered.tv_sec - timeStarted.tv_sec) + (double)(timeDelivered.tv_nsec - timeStarted.tv_nsec) / 1e9) / 60.0;
    acquireLock(&OutputLock, *id, t);
    printf("Order with number %d was delivered in %f minutes.\n", *oid, orderCompletionTime);
    releaseLock(&OutputLock, *id, t);

    // Simulate time for deliverer to return.
    sleep(wait);

    // Release deliverer.
    acquireLock(&DelivererLock, *id, t);
    Deliverer += 1;
    releaseLock(&DelivererLock, *id, t);
    pthread_cond_signal(&AvailableDelivererCond); // At most one thread is waiting for deliverer at each point in time (FCFS Policy).

    /* END OF PART 6*/


    /* START OF PART 7 */

    // Calculate cooling time (from time that order finished baking, up to time that order was delivered)
    double coolingTime = ((timeDelivered.tv_sec - timeFinishedBaking.tv_sec) + (double)(timeDelivered.tv_nsec - timeFinishedBaking.tv_nsec) / 1e9) / 60.0;
    
    acquireLock(&StatisticsLock, *id, t);

    // So that we can find max cooling time and max order completion time.
    if (orderCompletionTime > maxOrderCompletionTime) maxOrderCompletionTime = orderCompletionTime;
    if (coolingTime > maxCoolingTime) maxCoolingTime = coolingTime;

    // So that we can calculate mean order completion time and mean cooling time.
    orderCompletionTimeSum += orderCompletionTime;
    coolingTimeSum += coolingTime;

    releaseLock(&StatisticsLock, *id, t);

    /* END OF PART 7*/


    pthread_exit(t);
}

int main(int argc, char* argv[]){

    int customers;

    // Initialize locks
    initializeMutex(&OutputLock);
    initializeMutex(&PaymentLock);
    initializeMutex(&cookPriorityLock);
    initializeMutex(&ovenPriorityLock);
    initializeMutex(&delivererPriorityLock);
    initializeMutex(&CookLock);
    initializeMutex(&OvenLock);
    initializeMutex(&DelivererLock);
    initializeMutex(&StatisticsLock);

    // Initialize conds
    initializeCondition(&cookPriorityCond);
    initializeCondition(&ovenPriorityCond);
    initializeCondition(&delivererPriorityCond);
    initializeCondition(&AvailableCookCond);
    initializeCondition(&AvailableOvenCond);
    initializeCondition(&AvailableDelivererCond);
    
    // Check if correct count of arguments is provided.
    if (argc != 3){
        printf("Error: Not enough arguments provided. (Number of Customers and initial seed is required.)\n");
        exit(-1);
    }

    customers = atoi(argv[1]);
    seed = atoi(argv[2]);

    if (customers <= 0){
        printf("Error: Number of customers should be positive.\n");
        exit(-1);
    }
    
    // Allocate memory for array of threads.
    pthread_t *threads = (pthread_t *) malloc(customers * sizeof(pthread_t));
    if (threads == NULL){
        printf("Out of memory!");
        exit(-1);
    }
    
    // In order to use rand_r in main().
    unsigned int newseed = seed;

    // Create threads.
    int i;
    int wait = 0;
    int oids[customers];

    for (i = 0; i < customers; i++){
        oids[i] = i + 1;
        
        if (pthread_create(&threads[i], NULL, &simulateServiceFunc, &oids[i]) != 0) {
            printf("ERROR: Thread creation failed in main()\n");
            exit(-1);
        }

        // Sleep for random time to simulate time for new customer to connect.
        wait = (rand_r(&seed) % (T_ORDERHIGH - T_ORDERLOW + 1)) + T_ORDERLOW;
        sleep(wait);
        
    }

    // Wait for threads to terminate.
    for (i = 0; i < customers; i++){
        if (pthread_join(threads[i], NULL) != 0){
            printf("ERROR: pthread_join() failed in main()\n");
        }
    }

    /* PRINT STATS */
    if (AcceptedOrders > 0){
        //printf("Total specials sold: %d\n", totalSpecialSold);
        //printf("Total plain sold %d\n", totalPlainSold);
        printf("Total income: %d\n", TotalRevenue);
        printf("Total transactions: %d\n", AcceptedOrders + FailedOrders);
        printf("Failed transaction count: %d\n", FailedOrders);
        printf("Mean order completion time: %f\n", orderCompletionTimeSum / AcceptedOrders);
        printf("Max order completion time: %f\n", maxOrderCompletionTime);
        printf("Mean cooling time: %f\n", coolingTimeSum / AcceptedOrders);
        printf("Max cooling time: %f\n", maxCoolingTime);
    }else{
        printf("There was no succesful transaction.\n");
    }
    

    // Destroy locks.
    destroyLock(&OutputLock);
    destroyLock(&PaymentLock);
    destroyLock(&cookPriorityLock);
    destroyLock(&ovenPriorityLock);
    destroyLock(&delivererPriorityLock);
    destroyLock(&CookLock);
    destroyLock(&OvenLock);
    destroyLock(&DelivererLock);
    destroyLock(&StatisticsLock);

    // Destroy conds.
    destroyCond(&cookPriorityCond);
    destroyCond(&ovenPriorityCond);
    destroyCond(&delivererPriorityCond);
    destroyCond(&AvailableCookCond);
    destroyCond(&AvailableOvenCond);
    destroyCond(&AvailableDelivererCond);

    // Release allocated memory.
    free(threads);

    return 1;
}