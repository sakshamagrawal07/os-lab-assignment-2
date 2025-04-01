#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PROCESSES 100

struct Process {
    char name[5];
    int arrivalTime, burstTime, remainingTime;
    int ioInterval, ioDuration;
    int waitingTime, turnaroundTime, completionTime, responseTime;
    int insertedIOtime;  // To track when the process entered IO
    bool inIO, executed;
};

struct Process processes[MAX_PROCESSES];
int processCount = 0;

// Function to read process data from file
void readData(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    while (fscanf(file, "%[^;];%d;%d;%d;%d\n",
                  processes[processCount].name,
                  &processes[processCount].arrivalTime,
                  &processes[processCount].burstTime,
                  &processes[processCount].ioInterval,
                  &processes[processCount].ioDuration) != EOF) {
        processes[processCount].remainingTime = processes[processCount].burstTime;
        processes[processCount].waitingTime = 0;
        processes[processCount].turnaroundTime = 0;
        processes[processCount].completionTime = 0;
        processes[processCount].responseTime = -1;
        processes[processCount].inIO = false;
        processes[processCount].executed = false;
        processes[processCount].insertedIOtime = -1;
        processCount++;
    }

    fclose(file);
}

// Function to print the scheduling results
void printProcesses() {
    printf("\nProcess Execution Results:\n");
    printf("------------------------------------------------------------\n");
    printf("PID  Arrival  Burst  Completion  Turnaround  Waiting  Response\n");
    float AWT,ATAT,ART;

    for (int i = 0; i < processCount; i++) {
        printf("%-4s %-8d %-6d %-11d %-11d %-7d %-7d\n",
               processes[i].name,
               processes[i].arrivalTime,
               processes[i].burstTime,
               processes[i].completionTime,
               processes[i].turnaroundTime,
               processes[i].waitingTime,
               processes[i].responseTime
    );
    AWT+=processes[i].waitingTime;
    ATAT+=processes[i].turnaroundTime;
    ART+=processes[i].responseTime;
    }

    printf("\nAverage Waiting Time : %f\n",(float)(AWT/(float)processCount));
    printf("Average TurnAround Time : %f\n",(float)(ATAT/(float)processCount));
    printf("Average Response Time : %f\n",(float)(ART/(float)processCount));

    printf("------------------------------------------------------------\n");
}

// Shortest Job First (SJF) Non-Preemptive Scheduling 
void sjf() {
    printf("\nExecuting SJF (Non-Preemptive) ...\n");

    int completed = 0, time = 0;

    while (completed < processCount) {
        int minIdx = -1;

        // Check if any process has completed its I/O and can return to CPU
        for (int i = 0; i < processCount; i++) {
            if (processes[i].inIO && (time - processes[i].insertedIOtime) >= processes[i].ioInterval) {
                processes[i].inIO = false;
                processes[i].insertedIOtime = -1;
            }
        }

        // Find the shortest available job (not in I/O and arrived)
        for (int i = 0; i < processCount; i++) {
            if (!processes[i].executed && !processes[i].inIO && processes[i].arrivalTime <= time) {
                if (minIdx == -1 || processes[i].remainingTime < processes[minIdx].remainingTime) {
                    minIdx = i;
                }
            }
        }

        // If no process is available, increment time
        if (minIdx == -1) {
            time++;
            continue;
        }

        // Set response time if it's the first execution of the process
        if (processes[minIdx].responseTime == -1) {
            processes[minIdx].responseTime = time - processes[minIdx].arrivalTime;
        }

        // Execute the process in chunks until it finishes or requires I/O
        int executedTime = (processes[minIdx].remainingTime < processes[minIdx].ioDuration) ? 
                           processes[minIdx].remainingTime : processes[minIdx].ioDuration;

        time+=executedTime;
        processes[minIdx].remainingTime-=executedTime;

        // If process is completed
        if (processes[minIdx].remainingTime == 0) {
            processes[minIdx].executed = true;
            processes[minIdx].waitingTime=time-processes[minIdx].arrivalTime-processes[minIdx].burstTime;
            processes[minIdx].completionTime = time;
            processes[minIdx].turnaroundTime = processes[minIdx].completionTime - processes[minIdx].arrivalTime;
            completed++;
        } 
        // If process needs I/O
        else {
            processes[minIdx].inIO = true;
            processes[minIdx].insertedIOtime = time;
        }
    }

    printProcesses();
}

int main() {
    readData("processes.txt");
    sjf();
    return 0;
}
