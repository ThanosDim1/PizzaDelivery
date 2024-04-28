#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <pthread.h>

void *sample(){
    printf("Thread sample \n");
    sleep(1);
    printf("End Thread\n");
} 

int main(void)
{
    pthread_t tid_1, tid_2;
    pthread_create(&tid_1, NULL, &sample, NULL);
    pthread_create(&tid_2, NULL, &sample, NULL);
    pthread_join(tid_1,NULL);
    pthread_join(tid_2,NULL); 

}