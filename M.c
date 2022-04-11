//Amogh Ramani Bharadwaj 2019A7PS0086H
//Gaurav Sinha 2019A7PS0131H
//Vraj Ketan Gandhi 2019A7PS0158H
//Debdeep Naha 2019A3PS1289H
//Ananya Kumar Das 2019A7PS0001H
//Swastik Barpanda 2019A3PS0482H
//Anish Kumar Kacham 2019A7PS0091H
//Navam Pravin Shrivastav 2019A7PS0092H


#include<stdio.h>
#include<string.h>
#include <unistd.h>



int main(){
    char s[5];
    printf("Enter choice of scheduling algorithm : [fcfs/rr] ");
    scanf("%s",s);
    
    // Running the required executable as per user input.
    if(strcmp(s,"fcfs")==0) execl("./fcfs","FCFS",NULL);
    else if(strcmp(s,"rr")==0) execl("./rr","RR",NULL);
    else printf("Invalid option.\n");
    return 0;
    
}