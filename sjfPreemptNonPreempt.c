#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PROCESSES 100

struct Process {
    char name[5];
    int arrivalTime, burstTime, remainingTime;
    int waitingTime, turnaroundTime, completionTime, responseTime;
    int executed;
};

struct Process processes[MAX_PROCESSES];
int processCount = 0;

void readData(char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    while (fscanf(file, "%[^;];%d;%d\n",
                  processes[processCount].name,
                  &processes[processCount].arrivalTime,
                  &processes[processCount].burstTime) != EOF) {
        processes[processCount].remainingTime = processes[processCount].burstTime;
        processes[processCount].waitingTime = 0;
        processes[processCount].turnaroundTime = 0;
        processes[processCount].completionTime = 0;
        processes[processCount].responseTime = -1; // -1 means not yet responded
        processes[processCount].executed = 0;
        processCount++;
    }

    fclose(file);
}

void printProcesses() {
    printf("\nProcess Execution Results:\n");
    printf("------------------------------------------------------------\n");
    printf("PID  Arrival  Burst  Completion  Turnaround  Waiting  Response\n");
    for (int i = 0; i < processCount; i++) {
        printf("%-4s %-8d %-6d %-11d %-11d %-7d %-7d\n",
               processes[i].name,
               processes[i].arrivalTime,
               processes[i].burstTime,
               processes[i].completionTime,
               processes[i].turnaroundTime,
               processes[i].waitingTime,
               processes[i].responseTime);
    }
    printf("------------------------------------------------------------\n");
}

void sjf() {
    printf("\nExecuting SJF (Non-Preemptive)...\n");

    int completed = 0, time = 0;
    while (completed < processCount) {
        int minIdx = -1;
        for (int i = 0; i < processCount; i++) {
            if (processes[i].arrivalTime <= time && !processes[i].executed) {
                if (minIdx == -1 || processes[i].burstTime < processes[minIdx].burstTime)
                    minIdx = i;
            }
        }

        if (minIdx == -1) {
            time++;
            continue;
        }

        if (processes[minIdx].responseTime == -1)
            processes[minIdx].responseTime = time - processes[minIdx].arrivalTime;

        time += processes[minIdx].burstTime;
        processes[minIdx].completionTime = time;
        processes[minIdx].turnaroundTime = processes[minIdx].completionTime - processes[minIdx].arrivalTime;
        processes[minIdx].waitingTime = processes[minIdx].turnaroundTime - processes[minIdx].burstTime;
        processes[minIdx].executed = 1;
        completed++;
    }
    printProcesses();
}

void srtf() {
    printf("\nExecuting SRTF (Preemptive)...\n");

    int completed = 0, time = 0;
    while (completed < processCount) {
        int minIdx = -1;
        for (int i = 0; i < processCount; i++) {
            if (processes[i].arrivalTime <= time && processes[i].remainingTime > 0) {
                if (minIdx == -1 || processes[i].remainingTime < processes[minIdx].remainingTime)
                    minIdx = i;
             }
        }

        if (minIdx == -1) {
            time++;
            continue;
        }

        if (processes[minIdx].responseTime == -1)
            processes[minIdx].responseTime = time - processes[minIdx].arrivalTime;

        processes[minIdx].remainingTime--;
        time++;

        if (processes[minIdx].remainingTime == 0) {
            processes[minIdx].completionTime = time;
            processes[minIdx].turnaroundTime = processes[minIdx].completionTime - processes[minIdx].arrivalTime;
            processes[minIdx].waitingTime = processes[minIdx].turnaroundTime - processes[minIdx].burstTime;
            completed++;
        }
    }
    printProcesses();
}
int main() {
    readData("processes.txt");

    sjf();
    srtf();

    return 0;
}
