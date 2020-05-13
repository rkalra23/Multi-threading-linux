#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
// check it later //
#include <semaphore.h>
#include <sys/time.h>
/////////////////////

//TODO Define global data structures to be used
int buf_weight[],buf_ready[], front_weight=0,front_ready=0, rear_weight=0,rear_ready=0,MAX_BUFF_ready,MAX_BUFF_weight;
sem_t empty_weight,empty_ready, full_ready,full_weight,mutex,mutex_weight;


int size_r=0,size_w=0;
int patient_treated=0, patient_left=0;
//////////////// timer /////////////
 void waitFor(unsigned int secs) {
    unsigned int retTime = time(0) + secs;   // Get finishing time.
    while (time(0) < retTime);               // Loop until it arrives.
}


/*
* This function is used to represent treating your patients
*/
void treat_patient(int patient) {
	 ///////////// timer related //////////
	//TODO define treat_patient for your use including any parameters needed
	int random,min=5,max=8;
	random=(rand()%(max-min + 1) + min);
	//patient_treated=patient_treated+1;
	waitFor(8);
	printf(" ******** patient %d treated ******** \n",patient);
}


/**
 * This thread is responsible for getting patients from the waiting room 
 * to treat and sleep when there are no patients.
 */
void *doctor_thread(void *arg) {
	int ID_doc = *((int *)arg);
        int finished=1;
        int ret;
        static int nextConsumed = 0;
        static int nextConsumed_weight =0;
        do 
        {
	 printf("Doctor %d\n",ID_doc);
         sem_wait(&full_ready);
         sem_wait(&mutex);
         nextConsumed = buf_ready[front_ready];
         /* Check to make sure we did not read from an empty slot */
        if (nextConsumed == -1) {
            fprintf(stderr, "Synch Error: Consumer %d Just Read from empty slot %d\n", ID_doc, front_ready);
            exit(1);
        }
        /* We must be OK */
        printf("Doctor %d  attending patient  %d from slot %d\n", ID_doc, nextConsumed, front_ready);
        buf_ready[front_ready] = -1;
	size_r=size_r-1;
        front_ready = (front_ready + 1) % MAX_BUFF_ready;
        printf("incremented front_ready!\n");
	sem_post(&mutex);
        treat_patient(nextConsumed);
	sem_post(&empty_ready);
        printf("Now moved to weighting queue\n");
	printf("Size_w %d is and size_r is %d\n",size_w,size_r);
	while(size_w!=0){
	sem_wait(&mutex_weight);
        nextConsumed_weight = buf_weight[front_weight];
	printf("Element in weight queue is %d \n",nextConsumed_weight);
        if(nextConsumed_weight==-1)
        {
                printf("weight queue is empty \n");
                sem_post(&mutex_weight);
                //break;
        }
        else
        {
        /* we are good */
        printf("Doctor %d  treated patient %d from weight queue slot %d\n", ID_doc, nextConsumed_weight, front_weight);
        buf_weight[front_weight] = -1;
	size_w=size_w-1;
        front_weight = (front_weight + 1) % MAX_BUFF_ready;

        printf("incremented front_weight!\n");
	sem_post(&mutex_weight);
        treat_patient(nextConsumed_weight);
        }
	}
        }while (size_r);
	printf("doctor thread %d exiting hhhhhhhhhhhhhhhhhhhhh\n",ID_doc);
	 pthread_exit(1);
}

/**
 * This thread is responsible for acting as a patient, waking up doctors, waiting for doctors 
 * and be treated.
 */
void *patient_thread(void *arg) 
{
	int ID = *((int *)arg);
        int ret,min=1,max=5;
	int random;
        static int nextPatient = 0;
        printf("patient thread %d\n",ID);
                for(int i=0;i<ID;i++)
                {
                ret=sem_trywait(&empty_ready);
                printf("for patient %d sem return value check %d \n",i,ret);
                if(ret==0)
                {
                        // Check to see if Overwriting unread slot 
                        sem_wait(&mutex);
                        if (buf_ready[rear_ready] != -1) {
                                fprintf(stderr, "Synchronization Error patient thread :Patient %d Just overwrote %d from Slot %d\n",i,buf_ready[rear_ready], rear_ready);
                                exit(1);
                        }
                        // we are good till this point// 
                        buf_ready[rear_ready] = nextPatient;
                        printf("patient thread. Put patient %d in slot %d of ready_queue\n",nextPatient,rear_ready);
                        rear_ready = (rear_ready + 1) % MAX_BUFF_ready;
			size_r=size_r+1;
                        printf("incremented rear_ready!\n");

                        (void) sem_post(&mutex);
                        (void) sem_post(&full_ready);

                }
                else
                {
		  printf("Coming to weight queue in patient thread \n");
                  sem_wait(&mutex_weight);
		  if(size_w==MAX_BUFF_weight)
			{
				// patient_left= patient_left+1;
				printf("weighting arear is full patient %d has to leave\n",nextPatient);
			}
		  else{/*
                  while (buf_weight[rear_weight] != -1) {
                                //fprintf(stderr, "Synchronization Error: Patient %d Just overwrote %d from Slot %d\n", i,buf_weight[rear_weight], rear_weight);
				printf("weighting queue has not been read size_w %d and MAX_BUFF_weight %d  buffer element %d and rear value %d \n",size_w,MAX_BUFF_weight,buf_weight[rear_weight], rear_weight);
				 rear_weight = (rear_weight + 1) % MAX_BUFF_ready;

                        }
			*/
                        // Looks like we are OK//
                        buf_weight[rear_weight] = nextPatient;
                        printf("thread  %d. Put patient %d in slot %d of weighting_queue\n",i,nextPatient,rear_weight);
			size_w=size_w+1;
                        rear_weight = (rear_weight + 1) % MAX_BUFF_ready;
		   printf("incremented rear_weight!\n");
		  }

                sem_post(&mutex_weight);
                }
		printf("Patient end %d \n",i);
                nextPatient++;
		random=(rand()%(max-min + 1) + min);
		waitFor(random);
                }
		printf(" patient thread %d exiting pppppppppppppp \n ",ID);
	pthread_exit(1);
	//return NULL;

}

int main(int argc, char **argv) {

	//TODO: Define set-up required
	long int Patient,Doctor,WaitingSize;
	//TODO: store commandline options to be used

	Doctor=atoi(argv[3]);
        Patient=atoi(argv[2]);
        WaitingSize=atoi(argv[1]);
	int ID_Doc[Doctor];
        pthread_t TID_Doc[Doctor];
        pthread_t TID_Pat;
	/* lock is a binary semaphore used to make a critical same as mutex 
	 * To make sure only one thread accessing at a time*/
	sem_init(&mutex, 0, 1);
	sem_init(&mutex_weight,0,1);
	/* full is used to check queue is empty or not */
	sem_init(&full_ready, 0,0 );
	sem_init(&full_weight, 0,0);
	/* empty is used to check queue is full or not */
	sem_init(&empty_ready, 0,Doctor);
	sem_init(&empty_weight, 0,WaitingSize);
	MAX_BUFF_ready =  Doctor;
	MAX_BUFF_weight=WaitingSize;
	printf("argumenst are %d",argc);
	if(argc != 4){
		printf("Usage: DoctorsOffice <waitingSize> <patients> <doctors>\n");
		exit(0);
	}
	for(int i=0;i<argc;i++){
		printf("%s \n",argv[i]);
	}
	printf("\n %d %d %d\n",Doctor,Patient,WaitingSize);
	//TODO: Start Doctor Threads
	for (int i = 0; i < Doctor; i++) {
		ID_Doc[i] = i;
	}
	for (int i = 0; i < Doctor; i++) {
		buf_ready[i] = -1;
	}
	for (int i = 0; i < WaitingSize; i++) {
		buf_weight[i] = -1;
	}

	pthread_create(&TID_Pat, NULL,patient_thread,(void *)&Patient);
	for(int k=0;k<Doctor;k++)
	{	
		pthread_create(&TID_Doc[k], NULL,doctor_thread,(void *)&ID_Doc[k]);
	}
	sleep(2);
	//TODO: Clean up
	(void) sem_unlink("/empty_ready");
	(void) sem_unlink("/empty_weight");
    	(void) sem_unlink("/full_ready");
	(void) sem_unlink("/full_weight");
    	(void) sem_unlink("/mutex");
	(void) sem_unlink("/mutex_weight");
	pthread_exit(1);	
}
