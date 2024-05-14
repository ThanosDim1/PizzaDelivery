int seed;
float AverageWaitTime;
int MaxWaitTime;
float AverageColdTime;
int MaxColdTime;
int MargaritaPizzaCount = 0;
int PeperoniPizzaCount = 0;
int SpecialPizzaCount = 0;
int TotalRevenue = 0;
int FailedOrders = 0;
int AcceptedOrders = 0;

pthread_mutex_t OutputLock, StatisticsLock, PaymentLock, TeleLock, CookLock, OvenLock, DelivererLock;
pthread_cond_t AvailableTeleCond, AvailableDelivererCond, AvailableOvenCond, AvailableCookCond, AvailableTeleCond;
pthread_mutex_t TelePriorityLock, cookPriorityLock, ovenPriorityLock, delivererPriorityLock;
pthread_cond_t TelePriorityCond, cookPriorityCond, ovenPriorityCond, delivererPriorityCond;

// Propabilities
#define P_M 0.35
#define P_P 0.25
#define P_S 0.4
#define P_FAIL 0.05

// Costs
#define C_M 10
#define C_P 11
#define C_S 12

// Time
#define T_PREP 1
#define T_BAKE 10
#define T_PACK 1
#define T_DELLOW 5
#define T_DELHIGH 15
#define T_PAYMENTLOW 1
#define T_PAYMENTHIGH 3
#define T_ORDERLOW 1
#define T_ORDERHIGH 5

// Number
#define N_TEL 2
#define N_COOK 2
#define N_OVEN 10
#define N_DELIVERER 10
#define N_ORDERLOW 1
#define N_ORDERHIGH 5
