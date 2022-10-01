#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "p3170225-p3170107-pizza1.h"

//gcc -Wall -pthread p3170225-p3170107-pizza1.c p3170225-p3170107-pizza1.h

//Declaring Mutex and Cond Varriables
pthread_mutex_t cookLock;//mutex for cooks
pthread_cond_t cookCond;//cond for cooks
pthread_mutex_t ovenLock;//mutex for ovens 
pthread_cond_t ovenCond;//cond for ovens
pthread_mutex_t screenLock;//mutex for screen
pthread_mutex_t timeLock;//mutex for keeping time

// time stuff
double sumTime; // total order preperation time
double maxTime=0; //max order preperation time yet

int cooks= NCOOK;
int ovens=NOVEN;

void *order(void *x){
    int * ptrOD= (int*) x;
    int od= *ptrOD; //contains info from odArray
	double elapsed=0; // for time elapsed
   
//order accepted
	pthread_mutex_lock(&screenLock);
    printf("New Order with number %d, number of pizzas:%d.\n",od/10, od%10 );
    pthread_mutex_unlock(&screenLock);
    
//Cook availability
    pthread_mutex_lock(&cookLock);

 //start clock
	pthread_mutex_lock(&timeLock);
	struct timespec start;
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      	perror( "clock gettime" );
      exit( EXIT_FAILURE );
    }
    pthread_mutex_unlock(&timeLock);

    while(cooks==0 || cooks<0){
        pthread_cond_wait(&cookCond,&cookLock);
    }
    cooks--;
    sleep((od%10)*TPREP);// wait time for pizza prep
     pthread_mutex_unlock(&cookLock);

 //Oven availability 
	pthread_mutex_lock(&cookLock);
    pthread_mutex_lock(&ovenLock);
    while (ovens==0 || ovens <0){
        pthread_cond_wait(&ovenCond,&ovenLock);
    }
    ovens--;

    sleep(TBAKE);// baking time 

    ovens++;
    pthread_cond_signal(&ovenCond);
    pthread_mutex_unlock(&ovenLock);
  
  cooks++;
     pthread_cond_signal(&cookCond);
    pthread_mutex_unlock(&cookLock);
   
//stop clock 
pthread_mutex_lock(&timeLock);
struct timespec stop;
	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
     	 	perror( "clock gettime" );
		exit( EXIT_FAILURE );
    }
pthread_mutex_unlock(&timeLock);

	//print out time 
        elapsed =  ( stop.tv_sec - start.tv_sec ) ;
    	pthread_mutex_lock(&screenLock);
        printf("Order with id %d prepared in %lf minutes.\n",od/10, elapsed);
	pthread_mutex_unlock(&screenLock);
   
//updating global variables
    if(elapsed>maxTime){
       maxTime=elapsed;
    }
   sumTime += elapsed ;
   pthread_exit(NULL);
}

int main(int argc, char* *argv ){
    // check argument number 
    if(argc!=3){    
        printf("ERROR:the program should take 2 arguments, the number of Customers and Seed for random numbers!\n");
        exit(-1);
    }
   
    // Check if Customer Number is Positive 
    int nCust = atoi(argv[1]);
    if(nCust <0){
        printf("ERROR: The number of orders should be positive. Number given= %d.\n", nCust);
        exit(-1);
    } 

    //intitiallize seed for rand_r
    unsigned int seed= atoi (argv[1]);
	unsigned int* seedp= &seed;

    // Allocate memory for threads
    pthread_t *threads;
    threads=(pthread_t *) calloc(nCust,sizeof(pthread_t)); 
    if (threads==NULL){
        printf("NOT ENOUGH MEMORY");
        return -1;
    }

    //Initialize mutexes and conditions
    pthread_mutex_init(&cookLock, NULL);
    pthread_cond_init(&cookCond, NULL);
    pthread_mutex_init(&ovenLock, NULL);
    pthread_cond_init(&ovenCond, NULL);
    pthread_mutex_init(&screenLock, NULL);
    pthread_mutex_init(&timeLock, NULL);
    
    //Creating threads
    int rc; 
//odArray contains order data. Last digit is pizza number. The rest is the id
    int* odArray; 
	odArray=(int  *) calloc(nCust,sizeof(int));
    for( int i=0; i< nCust;i++){
	 odArray[i]=(i+1)*10+(rand_r(seedp)%(NORDERHIGH-NORDERLOW)+ NORDERLOW);
       rc=pthread_create(&threads[i],NULL, order, &odArray[i]);
	    if (rc!=0){
		    printf("Error at creating thread %d : %d\n",i,rc);
	    }

        sleep(rand_r(seedp)%(TORDERHIGH-TORDERLOW)+TORDERLOW); 

    }
   
    //joining threads
    for (int i = 0; i < nCust; i++) {
	   rc= pthread_join(threads[i], NULL);
	    if (rc!=0){
	    	printf("Error at joining thread %d : %d\n",i,rc);
	    }	
    }
    
    // final exit
    double avg= sumTime/nCust;
    printf("Max order Preparation time: %lf minutes.\n", maxTime);
    printf("Average order preperation time: %lf minutes.\n",avg);
    
    //Destroying Mutexes and Conditions, Freeing allocated Memory
    pthread_mutex_destroy(&cookLock); 
    pthread_cond_destroy(&cookCond);
    pthread_mutex_destroy(&ovenLock);
    pthread_cond_destroy(&ovenCond);
    pthread_mutex_destroy(&screenLock);
    pthread_mutex_destroy(&timeLock);
    free(threads);
    free(odArray);
    return 1;
}