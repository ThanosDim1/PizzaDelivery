#include "PizzaDelivery.h"

int Cookers = N_COOK;
int Oven = N_OVEN;
int Callers = N_TEL;
int Deliverer = N_DELIVERER;
int Tele = N_TEL;

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
void acquireLock(pthread_mutex_t *mutex, int *id){
    if (pthread_mutex_lock(mutex) != 0){
        printf("ERROR: pthread_mutex_lock() failed in thread %d\n", *id);
        exit(-1);
    }
}

// Releases lock, if release fails then id of thread is printed and program exits.
void releaseLock(pthread_mutex_t *mutex, int *id){
    if (pthread_mutex_unlock(mutex) != 0){
        printf("ERROR: pthread_mutex_unlock() failed in thread %d\n", *id);
        exit(-1);
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

int PaymentFail(unsigned int* seed){
    // Generate random probability between 0 and 1
    double uni = (double) rand_r(seed) / RAND_MAX;

    // Check if the generated probability indicates payment failure
    if (uni < P_FAIL) 
        return 1;
    else 
        return 0;
}


void PizzaServices(int* id){
    
    order newOrder;

    unsigned int newseed = seed + *id;
    unsigned int wait;

    newOrder.TotalPizzas = (rand_r(newseed) % (N_ORDERHIGH - N_ORDERLOW + 1)) + N_ORDERLOW;

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

    acquireLock(&OutputLock, *id);

    acquireLock(&PaymentLock, *id);
    
    wait = (rand_r(newseed) % (T_PAYMENTHIGH - T_PAYMENTLOW + 1)) + T_PAYMENTLOW;
    sleep(wait);

    if (PaymentFail(&newseed)){
        
        printf("Order %d: Payment failed.\n", *id);

        FailedOrders++;
        releaseLock(&OutputLock, *id);
        releaseLock(&PaymentLock, *id);

        pthread_exit(NULL);

    }
    else{

        printf("Order %d: Payment successful.\n", *id);
        AcceptedOrders++;

        MargaritaPizzaCount += newOrder.MargaritaPizza;
        SpecialPizzaCount += newOrder.SpecialPizza;
        PeperoniPizzaCount += newOrder.PepperoniPizza;
        
        TotalRevenue += newOrder.MargaritaPizza * C_M + newOrder.PepperoniPizza * C_P + newOrder.SpecialPizza * C_S;

        releaseLock(OutputLock, *id);
        releaseLock(PaymentLock, *id);
    } 

    while (Cookers == 0){
        pthread_cond_wait(AvailableCookCond,CookLock);
    }

    acquireLock(CookLock, *id);

    Cookers --;

    sleep(T_PREP * newOrder.TotalPizzas);

    Cookers ++;

    releaseLock(CookLock, *id);
    pthread_cond_singal(AvailableCookCond);

    acquireLock(OvenLock, *id);

    while (Oven < newOrder.TotalPizzas){
        pthread_cond_wait(AvailableOvenCond,OvenLock);
    }

    Oven -= newOrder.TotalPizzas;

    sleep(T_BAKE);

    Oven += newOrder.TotalPizzas;

    releaseLock(OvenLock, *id);
    pthread_cond_signal(AvailableOvenCond);

    acquireLock(DelivererLock, *id);

    while  (Deliverer == 0){
        pthread_cond_wait(AvailableDelivererCond,DelivererLock);
    }

    Deliverer --;

    wait = (rand_r(newseed) % (T_DELLHIGH - T_DELLOW + 1)) + T_DELLHIGH;

    sleep(T_PACK * newOrder.TotalPizzas + 2 * wait);

    Deliverer ++;

    releaseLock(DelivererLock, *id);
    pthread_cond_signal(AvailableDelivererCond);
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

    //EDO THA XEIRIZOMASTE TOUS TILEFONITES

    
}