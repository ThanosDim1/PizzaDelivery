int seed;                 // Seed for random number generation
float AverageWaitTime;    // Average wait time for orders
int MaxWaitTime;          // Maximum wait time for orders
float AverageCoolingTime; // Average cooling time for orders
float AverageCompletionTime;
int MaxColdTime;             // Maximum cooling time for orders
int MargaritaPizzaCount = 0; // Count of Margarita pizzas
int PeperoniPizzaCount = 0;  // Count of Pepperoni pizzas
int SpecialPizzaCount = 0;   // Count of Special pizzas
int TotalRevenue = 0;        // Total revenue earned
int FailedOrders = 0;        // Count of failed orders
int AcceptedOrders = 0;      // Count of accepted orders

// Variables to track maximum order completion time and cooling time,
// as well as their cumulative sums
double maxOrderCompletionTime = 0;
double maxCoolingTime = 0;
double orderCompletionTimeSum = 0;
double coolingTimeSum = 0;

// Mutexes and condition variables for synchronization
pthread_mutex_t OutputLock, StatisticsLock, PaymentLock, CallerLock, CookLock, OvenLock, DelivererLock;
pthread_cond_t AvailableDelivererCond, AvailableOvenCond, AvailableCookCond, AvailableCallerCond;

// Probabilities for pizza types and payment failure
#define P_M 0.35
#define P_P 0.25
#define P_S 0.4
#define P_FAIL 0.05

// Costs for different types of pizzas
#define C_M 10
#define C_P 11
#define C_S 12

// Time parameters
#define T_PREP 1        // Preparation time per pizza
#define T_BAKE 10       // Baking time for all pizzas
#define T_PACK 1        // Packing time per pizza
#define T_DELLOW 5      // Lower bound of delivery time
#define T_DELHIGH 15    // Upper bound of delivery time
#define T_PAYMENTLOW 1  // Lower bound of payment time
#define T_PAYMENTHIGH 3 // Upper bound of payment time
#define T_ORDERLOW 1    // Lower bound of time between orders
#define T_ORDERHIGH 5   // Upper bound of time between orders

// Number of resources and constraints
#define N_CALLERS 2    // Number of telephone lines
#define N_COOK 2       // Number of cooks
#define N_OVEN 10      // Number of ovens
#define N_DELIVERER 10 // Number of deliverers
#define N_ORDERLOW 1   // Lower bound of number of pizzas per order
#define N_ORDERHIGH 5  // Upper bound of number of pizzas per order