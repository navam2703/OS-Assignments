// FCFS Code
// TO BE FIXED: Critical section being skipped [Not performing input of numbers] and C1 not terminating.

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
#include <sys/time.h>
#include <sys/mman.h>

/* 
 pthread_wait_cond will automatically release a lock before it and wait on a conditional variable
 until signalled. 
 We use this to communicate between threads.
*/

pthread_cond_t T1=PTHREAD_COND_INITIALIZER,T2=PTHREAD_COND_INITIALIZER,T3=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

// Global variables for monitoring activity.
int play1=0,play2=0,play3=0;

// Shared memory function
void* share_memory(size_t size) {
    int protection = PROT_READ | PROT_WRITE; // Gives read and write permissions.
    int visibility = MAP_SHARED | MAP_ANONYMOUS; // Visible to these processes but not third party processes.


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

char* D1;  // To communicate the death of all processes.
char* D2;
char* D3;

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
	unsigned long long sum=0;
    
    int n3 = VOID_TO_INT(arg);
    
    while(fgets(str,10,fp)!=NULL && n3--){

        // C3 is asleep until monitor tells to wake up.
		pthread_mutex_lock(&mutex);
        while(!play3){pthread_cond_wait(&T3,&mutex);}
        
        // Critical section
        printf("[C3]: Adding..\n");
        sum += atoi(str);
        pthread_mutex_unlock(&mutex);
    }
    
    return (void *)sum;
}

void* C3_monitor_function(void *arg){
     
    while(strcmp(D3,"Die")!=0){ 
        // If scheduler says to stop.
        if(strcmp(MC3_memory,"Stop")==0){
            //printf("Locking...\n");

            // To pause
            pthread_mutex_lock(&mutex);
            play3 = 0;
            pthread_mutex_unlock(&mutex);
        }
      
        // If scheduler says to start.
        if(strcmp(MC3_memory,"Start")==0){

            // To play
            pthread_mutex_lock(&mutex);
            play3 = 1;
            pthread_cond_signal(&T3);
            pthread_mutex_unlock(&mutex);
        }
      
    }

}

void* C2_execution_function(void *arg)
{   
    // Opening n2 file.
    FILE* fp1;
    fp1 = fopen("n2.txt","r");
    char str[8];

    int n2 = VOID_TO_INT(arg);


    
	while(fgets(str,10,fp1)!=NULL && n2-- ){
        pthread_mutex_lock(&mutex);
        while(!play2){pthread_cond_wait(&T2,&mutex);}
            
        //Critical section
        int num = atoi(str);
        printf("[C2]: %d\n" , num);
        pthread_mutex_unlock(&mutex);
    }

}

void* C2_monitor_function(void *arg){

    
    while(strcmp(D2,"Die")!=0){ 

        if(MC2_memory=="Die"){
            //printf("[C1 MONITOR]: Im out\n");
            pthread_mutex_unlock(&mutex);
            break;
        }
        // If scheduler says to stop.
        if((strcmp(MC2_memory,"Stop")==0)){
            //printf("Locking C2...\n");
            pthread_mutex_lock(&mutex);
            play2 = 0;
            pthread_mutex_unlock(&mutex);
        }
        
     
        // If scheduler says to start.
        if(strcmp(MC2_memory,"Start")==0){
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
        // Unless monitor tells C1 to start, it will be asleep.
        pthread_mutex_lock(&mutex);
        while(!play1){pthread_cond_wait(&T1,&mutex);}
        
        //Critical section

        // Optional input method.
        //int x;
        //scanf("%d",x);
        
        printf("[C1]: Adding..\n");
        arg+=arr[i];  
        pthread_mutex_unlock(&mutex);
    }
    
    return (void *)arg;
}

void* C1_monitor_function(){
    
    // If C1 is over, monitor thread should terminate. 
    while(strcmp(D1,"Die")!=0){    

        //printf("MC1 memory: %s\n",MC1_memory);
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
            //Starting/resuming task thread.
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

    float time_quantum;

    // Pipes.
    int p1[2],p2[2],p3[2];
    char buf[40];

    
    
   // Shared memories MC1,MC2,MC3 are main-to-process communication
   MC1_memory = share_memory(128);
   strcpy(MC1_memory,"Stop");

   MC2_memory = share_memory(128);
   strcpy(MC2_memory,"Stop");

   MC3_memory = share_memory(128);
   strcpy(MC3_memory,"Stop");

   // Shared memory to kill the threads.
   D1 = share_memory(128);
   strcpy(D1,"Live");

   D2 = share_memory(128);
   strcpy(D2,"Live");

   D3 = share_memory(128);
   strcpy(D3,"Live");
   

   // Part of sys/time.h
   // Measures seconds elapsed since Jan 1970.
   time_t C1_Arrival_Time,C2_Arrival_Time,C3_Arrival_Time;
   
   // Inputting time quantum.
   printf("Enter the time quantum: ");
   scanf("%f",&time_quantum);
   
   // Workload input.
   int n1,n2,n3;
   printf("\nEnter n1: ");
   scanf("%d",&n1);

   printf("\nEnter n2: ");
   scanf("%d",&n2);

   printf("\nEnter n3: ");
   scanf("%d",&n3);
   
    
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
    time(&C1_Arrival_Time);
	pid = fork();
 
	// If fork() returns zero then it
	// means it is a child process.
	if (pid == 0) {
        //C1

		pthread_t C1_monitor_thread;
		pthread_t C1_execution_thread;

        void* status; // To store return value from execution thread.

        //Concurrent execution of both threads
        pthread_create(&C1_execution_thread , NULL, C1_execution_function,INT_TO_VOID(n1));
		pthread_create(&C1_monitor_thread , NULL, C1_monitor_function,NULL);
    	

		//pthread_join waits for the threads passed as argument to finish(terminate).
    	pthread_join(C1_execution_thread , &status); //The value returned by the execution function will be stored in status.
    
        long sum = (long)status; // Type casting status to long and storing in sum.

        close(p1[0]);
        write(p1[1],&sum,sizeof(sum)); // Writing sum to pipe.
        close(p1[1]);

        strcpy(D1,"Die"); // Intimating to M and monitor that task thread is dead.
       

    }

        else {
        
        // Noting arrival time of C2.
        time(&C2_Arrival_Time);
		pid1 = fork();
		if (pid1 == 0) {
            //C2

			pthread_t C2_execution_thread;
            pthread_t C2_monitor_thread;

			//Concurrent execution of both threads
            pthread_create(&C2_execution_thread , NULL, C2_execution_function,INT_TO_VOID(n2));
			pthread_create(&C2_monitor_thread , NULL, C2_monitor_function,NULL);
    		
            //pthread_join waits for the threads passed as argument to finish(terminate).
    		pthread_join(C2_execution_thread , NULL);
    		
            close(p2[0]);
            write(p2[1],"Done Printing",14);
            close(p2[1]);

            strcpy(D2,"Die"); //Intimating to M and monitor that C2 is dead.
           
		}
		else {
            // Noting arrival time of C3.
            time(&C3_Arrival_Time); 
            
            
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
    		

                close(p3[0]);
                write(p3[1],&sum2,sizeof(sum2));
                close(p3[1]);

                strcpy(D3,"Die"); // Intimating to M and the monitor that C3 is dead.
            }
            else{

                // SCHEDULER FOR ROUND ROBIN

                // To store outputs from pipes from C1 and C3.
                unsigned long long int c1_sum,c3_sum;

                // Initialising wait times.
                long double C1_Wait_Time=(time(NULL)-C1_Arrival_Time)*1000;
                long double C2_Wait_Time=(time(NULL)-C2_Arrival_Time)*1000;
                long double C3_Wait_Time=(time(NULL)-C3_Arrival_Time)*1000;
                
                // Initialising finish times.
                time_t C1_Finish_Time=C1_Arrival_Time,C2_Finish_Time=C2_Arrival_Time,C3_Finish_Time=C3_Arrival_Time;
                
                // Arrays to store start times of each process.
                int s1,s2,s3;
                long int c1s[1000];
                long int c2s[1000];
                long int c3s[1000];

             
                

                while(1){
                    
                    if(strcmp(D1,"Die")!=0){
                        
                        // Noting start time of this interval.
                        time_t start1;
                        time(&start1);

                        c1s[s1++] = (start1 - C1_Arrival_Time); //Storing the start time.
                        C1_Wait_Time += ((start1 - C1_Finish_Time)*1000); // Adding this intervals wait time.

                        strcpy(MC1_memory,"Start");

                        sleep(time_quantum);

                        strcpy(MC1_memory,"Stop");
                        
                        // Noting finish time of this interval
                        time(&C1_Finish_Time);
                    }
                    
                
                     if(strcmp(D2,"Die")!=0){
                        time_t start2;
                        time(&start2);

                        c2s[s2++] = (start2 - C2_Arrival_Time);
                        C2_Wait_Time += ((start2 - C2_Finish_Time)*1000);

                        strcpy(MC2_memory,"Start");

                        sleep(time_quantum);

                        strcpy(MC2_memory,"Stop");

                        time(&C2_Finish_Time);
                    }
                  

                     if(strcmp(D3,"Die")!=0){
                        time_t start3;
                        time(&start3);

                        c3s[s3++] = (start3 - C3_Arrival_Time);
                        C3_Wait_Time += ((start3 - C3_Finish_Time)*1000);

                        strcpy(MC3_memory,"Start");

                        sleep(time_quantum);

                        strcpy(MC3_memory,"Stop");
                        
                        time(&C3_Finish_Time);
                    }
                   

                    if((strcmp(D1,"Die")==0) && (strcmp(D2,"Die")==0) && (strcmp(D3,"Die")==0))break;
                }


                        // Getting message via pipe from C1.
                        read(p1[0],&c1_sum,sizeof(c1_sum));
                        close(p1[0]);
                        printf("C1 output: %lld\n",c1_sum);

                        // Getting message via pipe from C2.
                        read(p2[0],buf,14);
                        close(p2[0]);
                        printf("C2 output: %s\n",buf);

                        // Getting message via pipe from C3.
                        read(p3[0],&c3_sum,sizeof(c3_sum));
                        close(p3[0]); 
                        printf("C3 output: %lld\n\n",c3_sum);
                       

                       // Gant chart data output.
                        printf("\n------------------------------------\n");
                        printf("Output: \n");
                        printf("C1 started at times:\n");
                        for(int i=0;i<s1;i++){
                            printf("t= %ld s\n",c1s[i]);
                        }
                        printf("\nC1 Waiting Time: %Lf s\n",C1_Wait_Time/1000);
                        printf("C1 Turnaround Time: %ld s\n\n\n",(C1_Finish_Time - C1_Arrival_Time));

                        printf("C2 started at times:\n");
                        for(int i=0;i<s2;i++){
                            printf("t= %ld s\n",c2s[i]);
                        }


                        printf("\nC2 Waiting Time: %Lf s\n",C2_Wait_Time/1000);
                        printf("C2 Turnaround Time: %ld s\n\n\n",(C2_Finish_Time - C2_Arrival_Time));

                         printf("C3 started at times:\n");
                        for(int i=0;i<s3;i++){
                            printf("t= %ld s\n",c3s[i]);
                        }

                        printf("\nC3 Waiting Time: %Lf s\n",C3_Wait_Time/1000);
                        printf("C3 Turnaround Time: %ld s\n\n\n",(C3_Finish_Time - C3_Arrival_Time));
            }
             
        }
    }
}