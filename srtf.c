#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROCESSES 100

struct Process
{
    char name[5];
    int arrivalTime, burstTime, remainingTime;
    int ioInterval, ioDuration;
    int waitingTime, turnaroundTime, completionTime, responseTime;
    int insertedIOtime;
    bool inIO, executed;
};

struct Process processes[MAX_PROCESSES];
int processCount = 0;

// Read process data from file
void readData(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        exit(1);
    }

    while (fscanf(file, "%[^;];%d;%d;%d;%d\n",
                  processes[processCount].name,
                  &processes[processCount].arrivalTime,
                  &processes[processCount].burstTime,
                  &processes[processCount].ioInterval,
                  &processes[processCount].ioDuration) != EOF)
    {
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

// Print process results
void printProcesses()
{
    float AWT, ATAT, ART;
    printf("\nProcess Execution Results:\n");
    printf("------------------------------------------------------------\n");
    printf("PID  Arrival  Burst  Completion  Turnaround  Waiting  Response\n");
    for (int i = 0; i < processCount; i++)
    {
        printf("%-4s %-8d %-6d %-11d %-11d %-7d %-7d\n",
               processes[i].name,
               processes[i].arrivalTime,
               processes[i].burstTime,
               processes[i].completionTime,
               processes[i].turnaroundTime,
               processes[i].waitingTime,
               processes[i].responseTime);
        AWT += processes[i].waitingTime;
        ATAT += processes[i].turnaroundTime;
        ART += processes[i].responseTime;
    }

    printf("\nAverage Waiting Time : %f\n", (float)(AWT / (float)processCount));
    printf("Average TurnAround Time : %f\n", (float)(ATAT / (float)processCount));
    printf("Average Response Time : %f\n", (float)(ART / (float)processCount));

    printf("------------------------------------------------------------\n");
}

// Shortest Remaining Time First (SRTF) Preemptive Scheduling with I/O Handling
void srtf()
{
    printf("\nExecuting SRTF (Preemptive) with I/O Handling...\n");

    int completed = 0, time = 0;
    int lastExecuted = -1; // Track last executed process for better preemption

    while (completed < processCount)
    {
        int minIdx = -1;

        // Check if any process has completed its I/O and can return to CPU
        for (int i = 0; i < processCount; i++)
        {
            if (processes[i].inIO && (time - processes[i].insertedIOtime) >= processes[i].ioDuration)
            {
                processes[i].inIO = false;
                processes[i].insertedIOtime = -1;
            }
        }

        // Find process with the shortest remaining time that is ready to execute
        for (int i = 0; i < processCount; i++)
        {
            if (!processes[i].executed && !processes[i].inIO && processes[i].arrivalTime <= time)
            {
                if (minIdx == -1 || processes[i].remainingTime < processes[minIdx].remainingTime)
                {
                    minIdx = i;
                }
            }
        }

        // If no process is available, increment time
        if (minIdx == -1)
        {
            time++;
            continue;
        }

        // If it's the first time the process is executing, set response time
        if (processes[minIdx].responseTime == -1)
        {
            processes[minIdx].responseTime = time - processes[minIdx].arrivalTime;
        }

        // Execute process for one time unit
        processes[minIdx].remainingTime--;
        time++;

        // Increment waiting time for other processes
        for (int i = 0; i < processCount; i++)
        {
            if (!processes[i].executed && !processes[i].inIO && processes[i].arrivalTime <= time && i != minIdx)
            {
                processes[i].waitingTime++;
            }
        }

        // If process completes
        if (processes[minIdx].remainingTime == 0)
        {
            processes[minIdx].executed = true;
            processes[minIdx].completionTime = time;
            processes[minIdx].turnaroundTime = processes[minIdx].completionTime - processes[minIdx].arrivalTime;
            completed++;
        }
        // If process needs I/O
        else if ((processes[minIdx].burstTime - processes[minIdx].remainingTime) % processes[minIdx].ioInterval == 0)
        {
            processes[minIdx].inIO = true;
            processes[minIdx].insertedIOtime = time;
        }
    }

    printProcesses();
}

int main()
{
    readData("processses.txt");
    srtf();
    return 0;
}
