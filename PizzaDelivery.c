#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "PizzaDelivery.h"

pthread_mutex_t OutputLock, StatisticsLock, PaymentLock;

pthread_cond_t AvailableCallerCond, AvailableDelivererCond, AvailableOvenCond, AvailableCookCond;

int Cookers = N_COOK;
int Oven = N_OVEN;
int Callers = N_TEL;
int Deliverer = N_DELIVERER;

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

typedef struct order {
    int MargaritaPizza;
    int PepperoniPizza;
    int SpecialPizza;
    int TotalPizzas;
}order;

int CumulativeProb(void* args){
    
    unsigned int* seed = (unsigned int*) args;
    
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
}

int PaymentFail(void* args){
    unsigned int* seed = (unsigned int*) args;
    // uniE(Uniform[0,1])
    double uni = (double) rand_r(seed) / RAND_MAX;

    if (uni < P_FAIL) return 1;

    return 0;
}

void PizzaServices(int* id){
    
    order newOrder;

    unsigned int newseed = seed + id;
    unsigned int wait;

    newOrder.TotalPizzas = (rand_r(&seed) % (N_ORDERHIGH - N_ORDERLOW + 1)) + N_ORDERLOW;

    newOrder.MargaritaPizza=0;
    newOrder.PepperoniPizza=0;
    newOrder.SpecialPizza=0;

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
    
    wait = (rand_r(&newseed) % (T_PAYMENTHIGH - T_PAYMENTLOW + 1)) + T_PAYMENTLOW;
    sleep(wait);

    if (PaymentFail(&newseed)){
        printf("Order %d: Payment failed.\n", *id);
    }
    else{
        printf("Order %d: Payment successful.\n", *id);
    } 
}



int main(int argc, char *argv[])
{

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


}