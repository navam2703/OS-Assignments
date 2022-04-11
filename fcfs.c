// FCFS Code

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <sys/mman.h>


// Union and 2 functions to convert to and from (int) and (void*) data types.
union void_cast {
    void* ptr;
    int value;
};

int VOID_TO_INT(void* ptr) {
    union void_cast u;
    u.ptr = ptr;
    return u.value;
}

void* INT_TO_VOID(int value) {
    union void_cast u;
    u.value = value;
    return u.ptr;
}

/* 
 pthread_wait_cond will automatically release a lock before it and wait on a conditional variable
 until signalled. 
 We use this to communicate between threads.
*/
pthread_cond_t T1=PTHREAD_COND_INITIALIZER,T2=PTHREAD_COND_INITIALIZER,T3=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Global variables involved in monitor activity.
int play1=0,play2=0,play3=0;

// Shared memory function.
void* share_memory(size_t size) {
    int protection = PROT_READ | PROT_WRITE; // Giving read and write permissions.
    int visibility = MAP_SHARED | MAP_ANONYMOUS; // Visible to all processes but not to third party processes.

    // mmap() will map the process to a memory, if that memory location is 
    // already occupied, it will map it to the next one.
    return mmap(NULL, size, protection, visibility, -1, 0);
}

char* C1_memory; // Interthread communication (monitor thread of C1 to execution thread of C1).
char* C2_memory;
char* C3_memory;

char* MC1_memory; // This is for Main to the three processes.
char* MC2_memory;
char* MC3_memory;

//Function to convert string to integer, or sti for short. 
int sti(char s[]){
    int num = 0;
    int l = strlen(s);
   
    for(int i=0;i<l-1;i++){
        num+=(int)(s[i]-'0');
        num*=10;
    }
    
    num=num/10;
    return num;
}

// Task thread for C3.
void* C3_execution_function(void *arg){  
    
    // Opening n3 text file.
    FILE* fp;
    fp = fopen("n3.txt" , "r");
    char str[8];
	long long sum=0;

    int n3 = VOID_TO_INT(arg);

    while(fgets(str,10,fp)!=NULL && n3--){

        // C3 is asleep until monitor tells to wake up.
		pthread_mutex_lock(&mutex);
        while(!play3){printf("[C3]: Locked by monitor...\n");pthread_cond_wait(&T3,&mutex);}
        
        // Critical section
        printf("[C3]: Adding....\n");
        sum += atoi(str);
        pthread_mutex_unlock(&mutex);
    }
    
    //Intimating to monitor thread that the task is over.
    strcpy(MC3_memory,"Die");

    return (void *)sum;
}

// Monitor function for C3.
void* C3_monitor_function(void *arg){
     
    while(strcmp(MC3_memory,"Die")!=0){ 

        // If scheduler says to stop.
        if(strcmp(MC3_memory,"Stop")==0){
            // To pause task thread.
            pthread_mutex_lock(&mutex);
            play3 = 0;
            pthread_mutex_unlock(&mutex);
        }
      
        // If scheduler says to start.
        if(strcmp(MC3_memory,"Start")==0){

            // To start/resume task thread.
            pthread_mutex_lock(&mutex);
            play3 = 1;
            pthread_cond_signal(&T3);
            pthread_mutex_unlock(&mutex);
        }
    }
}

// Task thread for C2.
void* C2_execution_function(void *arg){   
    // Opening n2 file.
    FILE* fp1;
    fp1 = fopen("n2.txt","r");
    char str[8];

    int n2 = VOID_TO_INT(arg);
    
	while(fgets(str,10,fp1)!=NULL && n2--){
        
        // Locking/Putting to sleep.
        pthread_mutex_lock(&mutex);
        while(!play2){printf("[C2]: Locked by monitor...\n");pthread_cond_wait(&T2,&mutex);}
            
        //Critical section
        int num = atoi(str);
        printf("[C2]: %d\n" , num);
        pthread_mutex_unlock(&mutex);
    }

    // Intimating to monitor that C2 is over.
	strcpy(MC2_memory,"Die");
}


// Monitor function for C2.
void* C2_monitor_function(void *arg){

    while(strcmp(MC2_memory,"Die")!=0){ 

        if(MC2_memory=="Die"){
            pthread_mutex_unlock(&mutex);
            break;
        }

        // If scheduler says to stop.
        if((strcmp(MC2_memory,"Stop")==0)){

            //Stopping task thread
            pthread_mutex_lock(&mutex);
            play2 = 0;
            pthread_mutex_unlock(&mutex);
        }
        
     
        // If scheduler says to start.
        if(strcmp(MC2_memory,"Start")==0){
            // Starting/resuming task thread.
            pthread_mutex_lock(&mutex);
            play2 = 1;
            pthread_cond_signal(&T2);
            pthread_mutex_unlock(&mutex);
        }
      
    }
}


void* C1_execution_function(void* argument){

    long long arg = 0;
    
    int n1 = VOID_TO_INT(argument);
    int arr[n1];

    for(int i=0;i<n1;i++)arr[i]=(rand()%1000000);

    for(int i=0;i<n1;i++){ 
        
        // Locking/Putting to sleep.
        pthread_mutex_lock(&mutex);
        while(!play1){printf("[C1]: Locked by monitor...\n");pthread_cond_wait(&T1,&mutex);}
        
        //Critical section

        // Optional input method.
        //int x;
        //scanf("%d",x);

        printf("[C1]: Adding\n");
        arg+=arr[i];  
        pthread_mutex_unlock(&mutex);
    }
   
    // Intimating to monitor that execution is over.
    strcpy(MC1_memory,"Die");
    return (void *)arg;
}

void* C1_monitor_function(){
    
    while(strcmp(MC1_memory,"Die")!=0){    

        if(MC1_memory=="Die"){
            printf("[C1 MONITOR]: Im out\n");
            pthread_mutex_unlock(&mutex);
            break;
        }
       
        // If scheduler says to stop.
        if(strcmp(MC1_memory,"Stop")==0){

            // Stopping task thread.
            pthread_mutex_lock(&mutex);
            play1 = 0;
            pthread_mutex_unlock(&mutex);
        }
       
        // If scheduler says to start.
        if(strcmp(MC1_memory,"Start")==0){
            
            // Starting/resuming task thread.
            pthread_mutex_lock(&mutex);
            play1 = 1;
            pthread_cond_signal(&T1);
            pthread_mutex_unlock(&mutex);
        }
        
    }
}

int main()
{   //Process PIDs.
	int pid,pid1,pid2;

    // Pipes.
    int p1[2],p2[2],p3[2];
    char buf[40];

    // List of shared memories. 
    // C1,C2,C3 are for monitor-to-execution thread communication.

   // MC1,MC2,MC3 are main-to-process communication
   MC1_memory = share_memory(128);
   strcpy(MC1_memory,"Stop");

   MC2_memory = share_memory(128);
   strcpy(MC2_memory,"Stop");

   MC3_memory = share_memory(128);
   strcpy(MC3_memory,"Stop");
   
   // Taking workload input.
    int n1,n2,n3;
   printf("Enter n1: ");
   scanf("%d",&n1);

   printf("\nEnter n2: ");
   scanf("%d",&n2);

   printf("\nEnter n3: ");
   scanf("%d",&n3);

   // Part of time.h library. Stores CPU time.
   clock_t C1_Arrival,C2_Arrival,C3_Arrival;

   double C1_Arrival_Time,C2_Arrival_Time,C3_Arrival_Time;
    
    
    //Creating pipes.
    if (pipe(p1)==-1)
	{
		fprintf(stderr, "Pipe Failed" );
		return 1;
	}
	if (pipe(p2)==-1)
	{
		fprintf(stderr, "Pipe Failed" );
		return 1;
	}
	if (pipe(p3)==-1)
	{
		fprintf(stderr, "Pipe Failed" );
		return 1;
	}
 
    // Noting arrival time of C1.
    C1_Arrival = clock();
    C1_Arrival_Time = (C1_Arrival/CLOCKS_PER_SEC)*1000;
    
	pid = fork();
 
	// If fork() returns zero then it
	// means it is a child process.
	if (pid == 0) {

        // C1
		pthread_t C1_monitor_thread;
		pthread_t C1_execution_thread;

        void* status; // To store return value from execution thread.

        //Concurrent execution of both threads
        pthread_create(&C1_execution_thread , NULL, C1_execution_function,INT_TO_VOID(n1));
		pthread_create(&C1_monitor_thread , NULL, C1_monitor_function,NULL);
    	

		//pthread_join waits for the threads passed as argument to finish(terminate).
    	pthread_join(C1_execution_thread , &status); //The value returned by the execution function will be stored in status.
    	pthread_join(C1_monitor_thread, NULL); 

        long sum = (long)status; // Type casting status to long and storing in sum.

        close(p1[0]);
        write(p1[1],&sum,sizeof(sum)); // Writing sum to pipe.
        close(p1[1]);
       

    }

        else {
        // Noting arrival time of C2.
        C2_Arrival = clock();
        C2_Arrival_Time = (C2_Arrival/CLOCKS_PER_SEC)*1000;


		pid1 = fork();
		if (pid1 == 0) {
            //C2
           
			pthread_t C2_execution_thread;
            pthread_t C2_monitor_thread;

			//Concurrent execution of both threads
			pthread_create(&C2_monitor_thread , NULL, C2_monitor_function,NULL);
    		pthread_create(&C2_execution_thread , NULL, C2_execution_function,INT_TO_VOID(n2));

            //pthread_join waits for the threads passed as argument to finish(terminate).
    		pthread_join(C2_execution_thread , NULL);
    		
            close(p2[0]);
            write(p2[1],"Done Printing",14);
            close(p2[1]);
           
		}
		else {
            // Noting arrival time of C3.
            C3_Arrival = clock();
            C3_Arrival_Time = (C3_Arrival/CLOCKS_PER_SEC)*1000;
            
			pid2 = fork();
			if (pid2 == 0) {               
               //C3
				pthread_t C3_monitor_thread;
				pthread_t C3_execution_thread;
                
                void* status;

                //Concurrent execution of both threads
    			pthread_create(&C3_execution_thread , NULL, C3_execution_function,INT_TO_VOID(n3));
                pthread_create(&C3_monitor_thread , NULL, C3_monitor_function,NULL);

                pthread_join(C3_execution_thread , &status);
                long sum2 = (long)status;
    		    pthread_join(C3_monitor_thread, NULL);

                close(p3[0]);
                write(p3[1],&sum2,sizeof(sum2));
                close(p3[1]);
              
            }
            else{

                // SCHEDULER FOR FCFS.
                
                // To store pipe output from C1 and C3.
                long long c1_sum,c3_sum;

                clock_t C1_Start ,C1_Finish_Time;
                double C1_Turnaround_Time,C1_Wait_Time;

                C1_Start=clock(); // Current CPU time.

                double C1_Start_Time = (C1_Start/CLOCKS_PER_SEC)*1000;

                
                strcpy(MC1_memory,"Start"); // Tells to monitor to start the task thread of C1.
                while(strcmp(MC1_memory,"Die")); // Waits till C1 is done.

                // Getting message via pipe from C1.
                read(p1[0],&c1_sum,sizeof(c1_sum));
                close(p1[0]);
                

                C1_Finish_Time=clock();
                
                // Calculation
                C1_Wait_Time = ((C1_Start-C1_Arrival_Time)/CLOCKS_PER_SEC)*1000;
                C1_Turnaround_Time = ((C1_Finish_Time-C1_Arrival_Time)/CLOCKS_PER_SEC)*1000;

                clock_t C2_Start ,C2_Finish_Time;
                double C2_Turnaround_Time,C2_Wait_Time;

                C2_Start=clock();

                double C2_Start_Time = (C2_Start/CLOCKS_PER_SEC)*1000;

                strcpy(MC2_memory,"Start"); // Monitor of C2 starts the task thread of C2.
                while(strcmp(MC2_memory,"Die")); // Waits till C2 is done.
                
                // Getting message via pipe from C2.
                read(p2[0],buf,14);
                close(p2[0]);

                C2_Finish_Time=clock();

                // Calculation
                C2_Wait_Time = ((C2_Start-C2_Arrival_Time)/CLOCKS_PER_SEC)*1000;
                C2_Turnaround_Time = ((C2_Finish_Time-C2_Arrival_Time)/CLOCKS_PER_SEC)*1000;

                clock_t C3_Start ,C3_Finish_Time;
                double C3_Turnaround_Time,C3_Wait_Time;

                C3_Start=clock();

                double C3_Start_Time = (C3_Start/CLOCKS_PER_SEC)*1000;
 
                strcpy(MC3_memory,"Start");  // Monitor of C3 starts the task thread of C3.
                while(strcmp(MC3_memory,"Die")); // Waits till C3 is done.
                
                // Getting message via pipe from C3.
                read(p3[0],&c3_sum,sizeof(c3_sum));
                close(p3[0]); 
                

                C3_Finish_Time=clock();
                
                // Calculation
                C3_Wait_Time = ((C3_Start-C3_Arrival_Time)/CLOCKS_PER_SEC)*1000;
                C3_Turnaround_Time = ((C3_Finish_Time-C3_Arrival_Time)/CLOCKS_PER_SEC)*1000;

                // Pipe output and Gantt Chart data.
                printf("\n-------------------------------------------\nOutput:\n");

                printf("C1 output: %lld\n",c1_sum);
                printf("C2 output: %s\n",buf);
                printf("C3 output: %lld\n\n",c3_sum);

                printf("C1 arrived at t = %f ms\n",C1_Arrival_Time);
                printf("C1 starts at t = %f ms\n",C1_Wait_Time);
                printf("C1's Waiting Time: %f ms\n",C1_Wait_Time);
                printf("C1's Turnaround Time: %f ms\n\n",C1_Turnaround_Time);
               
               
                printf("C2 arrived at t = %f ms\n",C2_Arrival_Time);
                printf("C2 starts at t = %f ms\n",C2_Wait_Time);
                printf("C2's Waiting Time: %f ms\n",C2_Wait_Time);
                printf("C2's Turnaround Time: %f ms\n\n",C2_Turnaround_Time);

                printf("C3 arrived at t = %f ms\n",C3_Arrival_Time);
                printf("C3 starts at t = %f ms\n",C3_Wait_Time);
                printf("C3's Waiting Time: %f ms\n",C3_Wait_Time);
                printf("C3's Turnaround Time: %f ms\n\n",C3_Turnaround_Time);
                

            }
        }
    }
}