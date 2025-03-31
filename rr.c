#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 10
#define MAX_LINE_LENGTH 100

typedef struct {
    char name[10];
    int arrival_time;
    int cpu_burst;
    int io_units;
    int cpu_units_before_io;
    int remaining_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
    int first_run;
    int in_ready_queue;
} Process;

typedef struct {
    Process* items[MAX_PROCESSES];
    int front, rear;
    int count;
} Queue;

void initQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

int isEmpty(Queue* q) {
    return q->count == 0;
}

void enqueue(Queue* q, Process* p) {
    if (q->count >= MAX_PROCESSES) return;
    
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->items[q->rear] = p;
    q->count++;
}

Process* dequeue(Queue* q) {
    if (isEmpty(q)) return NULL;
    
    Process* p = q->items[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->count--;
    return p;
}

int readProcessesFromFile(const char* filename, Process processes[]) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file: %s\n", filename);
        return 0;
    }

    int count = 0;
    char line[MAX_LINE_LENGTH];

    while (fgets(line, MAX_LINE_LENGTH, file) && count < MAX_PROCESSES) {
        // Parse process data from the line
        char name[10];
        int arrival_time, cpu_burst, io_units, cpu_units;
        sscanf(line, "%[^;];%d;%d;%d;%d", name, &arrival_time, &cpu_burst, &io_units, &cpu_units);
        
        strcpy(processes[count].name, name);
        processes[count].arrival_time = arrival_time;
        processes[count].cpu_burst = cpu_burst;
        processes[count].io_units = io_units;
        processes[count].cpu_units_before_io = cpu_units;
        processes[count].remaining_time = cpu_burst;
        processes[count].finish_time = 0;
        processes[count].waiting_time = 0;
        processes[count].turnaround_time = 0;
        processes[count].response_time = -1;  // -1 indicates not started yet
        processes[count].first_run = 1;       // 1 indicates first run is pending
        processes[count].in_ready_queue = 0;  // 0 indicates not in ready queue
        
        count++;
    }

    fclose(file);
    return count;
}

void simulateRoundRobin(Process processes[], int n, int quantum) {
    Queue ready_queue;
    initQueue(&ready_queue);
    
    int current_time = 0;
    int completed_processes = 0;
    Process* current_process = NULL;
    int time_slice = 0;
    
    printf("\nRound Robin Scheduling (Quantum = %d):\n", quantum);
    printf("Time | Process | Remaining\n");
    printf("---------------------------\n");
    
    while (completed_processes < n) {
        // Check for new arrivals
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time <= current_time && 
                processes[i].remaining_time > 0 && 
                !processes[i].in_ready_queue) {
                enqueue(&ready_queue, &processes[i]);
                processes[i].in_ready_queue = 1;
            }
        }
        
        // If no process is running, get one from the queue
        if (current_process == NULL) {
            current_process = dequeue(&ready_queue);
            if (current_process != NULL) {
                current_process->in_ready_queue = 0;
                time_slice = 0;
                
                // Record response time for first run
                if (current_process->first_run) {
                    current_process->response_time = current_time - current_process->arrival_time;
                    current_process->first_run = 0;
                }
            }
        }
        
        // If we have a process to run
        if (current_process != NULL) {
            printf("%4d | %7s | %8d\n", current_time, current_process->name, current_process->remaining_time);
            
            current_process->remaining_time--;
            time_slice++;
            
            // Check if process is completed
            if (current_process->remaining_time == 0) {
                current_process->finish_time = current_time + 1;
                current_process->turnaround_time = current_process->finish_time - current_process->arrival_time;
                current_process->waiting_time = current_process->turnaround_time - current_process->cpu_burst;
                
                printf("%4d | %7s | Completed\n", current_time + 1, current_process->name);
                
                current_process = NULL;
                completed_processes++;
            }
            // Check if time quantum expired
            else if (time_slice >= quantum) {
                enqueue(&ready_queue, current_process);
                current_process->in_ready_queue = 1;
                current_process = NULL;
            }
        }
        
        current_time++;
        
        // If CPU is idle but processes haven't arrived yet
        if (current_process == NULL && isEmpty(&ready_queue) && completed_processes < n) {
            int next_arrival = __INT_MAX__;
            for (int i = 0; i < n; i++) {
                if (processes[i].remaining_time > 0 && processes[i].arrival_time < next_arrival) {
                    next_arrival = processes[i].arrival_time;
                }
            }
            
            if (next_arrival > current_time) {
                printf("%4d | %7s | %8s\n", current_time, "IDLE", "-");
                current_time = next_arrival;
            }
        }
    }
    
    // Print performance metrics
    printf("\nPerformance Metrics:\n");
    printf("Process | Turnaround Time | Waiting Time | Response Time\n");
    printf("-------------------------------------------------------\n");
    
    float avg_turnaround = 0, avg_waiting = 0, avg_response = 0;
    
    for (int i = 0; i < n; i++) {
        printf("%7s | %15d | %12d | %12d\n", 
               processes[i].name, 
               processes[i].turnaround_time, 
               processes[i].waiting_time, 
               processes[i].response_time);
        
        avg_turnaround += processes[i].turnaround_time;
        avg_waiting += processes[i].waiting_time;
        avg_response += processes[i].response_time;
    }
    
    avg_turnaround /= n;
    avg_waiting /= n;
    avg_response /= n;
    
    printf("\nAverage Turnaround Time: %.2f\n", avg_turnaround);
    printf("Average Waiting Time: %.2f\n", avg_waiting);
    printf("Average Response Time: %.2f\n", avg_response);
    printf("System Throughput: %.2f processes/unit time\n", (float)n / current_time);
}

int main() {
    // Create a data file based on the provided format
    FILE* dataFile = fopen("process_data.txt", "w");
    if (!dataFile) {
        printf("Error creating data file.\n");
        return 1;
    }
    
    fprintf(dataFile, "P0;0;24;2;5\n");
    fprintf(dataFile, "P1;3;17;3;6\n");
    fprintf(dataFile, "P2;8;50;2;5\n");
    fprintf(dataFile, "P3;15;10;3;6\n");
    fclose(dataFile);
    
    Process processes[MAX_PROCESSES];
    int n = readProcessesFromFile("input.txt", processes);
    
    if (n == 0) {
        printf("No processes read from file.\n");
        return 1;
    }
    
    printf("Round Robin CPU Scheduling\n");
    printf("Loaded %d processes:\n", n);
    printf("Process | Arrival Time | CPU Burst | I/O Units | CPU Units Before I/O\n");
    printf("----------------------------------------------------------------\n");
    
    for (int i = 0; i < n; i++) {
        printf("%7s | %12d | %9d | %8d | %18d\n", 
               processes[i].name, 
               processes[i].arrival_time, 
               processes[i].cpu_burst, 
               processes[i].io_units, 
               processes[i].cpu_units_before_io);
    }
    
    // Simulate Round Robin with quantum = 5
    simulateRoundRobin(processes, n, 5);
    
    return 0;
}