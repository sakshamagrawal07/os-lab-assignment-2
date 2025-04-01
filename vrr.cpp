#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <sstream>

#define LOG_TICK(ticks) std::cout << ticks;
#define LOG(tick, device, procData) \
  std::cout << tick << "\t" << device << "\t\t" << procData << "\n";
#define LOG_DEBUG(name, label, info) \
  std::cout << name << "\n\t\t" << label "\t" << info;

typedef struct Process {
  enum State { READY, RUNNING, BLOCKED, TERMINATED };
  std::string procName;
  size_t arrivalTime = SIZE_MAX;
  size_t burstTimeCPU = SIZE_MAX;
  size_t burstTimeIO = SIZE_MAX;
  size_t burstTimeRate = SIZE_MAX;  // IO burst after every n CPU bursts
  size_t startTime = SIZE_MAX;
  size_t completionTime;
  size_t burstRemainCPU = SIZE_MAX;
  size_t lastIOBurst = 0;
  int saveContextOfq = 0;
  State state;

  Process() {}
  Process(std::string&& name,
          size_t at,
          size_t btCPU,
          size_t btIO,
          size_t btr) {
    procName = std::move(name);
    arrivalTime = at;
    burstTimeCPU = btCPU;
    burstRemainCPU = btCPU;
    burstTimeIO = btIO;
    burstTimeRate = btr;
  }
  State exec() {
    state = State::RUNNING;
    if (--burstRemainCPU <= 0) {
      state = State::TERMINATED;
    } else if (++lastIOBurst >= burstTimeRate) {
      refreshIOBurst();
      state = State::BLOCKED;
    }
    return state;
  }
  void refreshIOBurst() { lastIOBurst = 0; }
  size_t turnAroundTime() { return completionTime - arrivalTime; }
  size_t waitingTime() { return turnAroundTime() - burstTimeCPU; }
  size_t responseTime() { return startTime - arrivalTime; }
} Process;
typedef std::vector<Process> Processes;

class Device {
 public:
  Device() {}
  void init(Processes& procs) {
    this->procs = procs;
    totalProc = procs.size();
  }

  void processor() {
    Process execProc;
    int q = 0;
    LOG("Time (tick)", "Device", "Process Served")
    while (totalProc) {
      LOG_TICK(ticksCPU)
      if (isCPUIdle) {
        LOG("\t", "CPU", "-");
      }
      FreshArrivals();

      if (!isCPUIdle) {
        execProc.exec();
        if (execProc.state == Process::State::TERMINATED) {
          LOG("\t", "CPU", execProc.procName << "[Comp]");
          isCPUIdle = true;
          totalProc--;
          execProc.completionTime = ticksCPU;
          completedProcs.push_back(execProc);
          execProc = {};
        } else if (execProc.state == Process::State::BLOCKED) {
          LOG("\t", "CPU",
              execProc.procName << "[Q IO]:" << execProc.burstRemainCPU);
          execProc.saveContextOfq = (q + 1) % timeQuantum;
          ioQ.push(execProc);
          isCPUIdle = true;
          execProc = {};
        } else {
          LOG("\t", "CPU", execProc.procName << ":" << execProc.burstRemainCPU)
        }
      }

      bool toSchedule = (!readyQ.empty() || !auxQ.empty()) &&
                        (isCPUIdle || q + 1 >= timeQuantum);
      if (toSchedule) {
        Process proc;
        if (!auxQ.empty()) {
          proc = auxQ.front();
          auxQ.pop();
          q = proc.saveContextOfq - 1;
        } else {
          proc = readyQ.front();
          readyQ.pop();
          q = -1;
        }
        if (!isCPUIdle) {
          readyQ.push(execProc);
        }
        LOG("\t", "CPU", proc.procName << "[Sched]#q=" << q + 1)
        execProc = proc;
        execProc.startTime = std::min(execProc.startTime, ticksCPU);
        isCPUIdle = false;
      }

      ioDevice();
      ticksCPU++;
      q++;
      std::cout << "\n";
    }
  }

  void ioDevice() {
    if (!isIOIdle) {
      if (++countIOBurst >= execProcIO.burstTimeIO) {
        LOG("\t", "IO", execProcIO.procName << "[Comp]:" << countIOBurst)
        auxQ.push(execProcIO);
        execProcIO = {};
        isIOIdle = true;
      } else {
        LOG("\t", "IO", execProcIO.procName << ":" << countIOBurst)
      }
    }

    if (isIOIdle && !ioQ.empty()) {
      execProcIO = ioQ.front();
      ioQ.pop();
      countIOBurst = 0;
      isIOIdle = false;
      LOG("\t", "IO", execProcIO.procName << "[Sched]:" << countIOBurst)
    }
  }

  void debug() {
    for (auto& proc : completedProcs) {
      LOG_DEBUG(proc.procName, "Arrival Time:\t", proc.arrivalTime)
      LOG_DEBUG("", "Start Time:\t", proc.startTime)
      LOG_DEBUG("", "Response Time:\t", proc.responseTime())
      LOG_DEBUG("", "Completion Time:", proc.completionTime)
      LOG_DEBUG("", "Turnaround Time:", proc.turnAroundTime())
      LOG_DEBUG("", "Waiting Time:\t", proc.waitingTime() << "\n")
    }
    std::cout << "Avg Waiting Time: " << avgWaitingTime();
  }

  double avgWaitingTime() {
    double sum = 0;
    for (auto& proc : completedProcs) {
      sum += proc.waitingTime();
    }
    return (double)(sum / completedProcs.size());
  }

 private:
  Processes completedProcs = {};
  Processes procs = {};
  size_t totalProc = 0;
  size_t ticksCPU = 0;
  size_t timeQuantum = 5;
  bool isCPUIdle = true;

  size_t countIOBurst = 0;
  bool isIOIdle = true;
  Process execProcIO;

  std::queue<Process> readyQ;
  std::queue<Process> auxQ;
  std::queue<Process> ioQ;

  void FreshArrivals() {
    int index = 0;
    for (auto& proc : procs) {
      if (proc.arrivalTime == ticksCPU) {
        LOG("\t", "CPU", proc.procName << "[Arrive]")
        proc.state = Process::State::READY;
        readyQ.push(proc);
        procs.erase(procs.begin() + index);
        continue;
      }
      index++;
    }
  }
};

// Function to read processes from input file
Processes readProcessesFromFile(const std::string& filename) {
  Processes processes;
  std::ifstream inputFile(filename);
  
  if (!inputFile.is_open()) {
    std::cerr << "Error: Unable to open file " << filename << std::endl;
    return processes;
  }
  
  std::string line;
  while (std::getline(inputFile, line)) {
    std::stringstream ss(line);
    std::string procName;
    size_t arrivalTime, burstTimeCPU, burstTimeIO, burstTimeRate;
    
    // Parse line with format: P0;0;24;2;5
    std::getline(ss, procName, ';');
    
    std::string value;
    std::getline(ss, value, ';');
    arrivalTime = std::stoul(value);
    
    std::getline(ss, value, ';');
    burstTimeCPU = std::stoul(value);
    
    std::getline(ss, value, ';');
    burstTimeIO = std::stoul(value);
    
    std::getline(ss, value, ';');
    burstTimeRate = std::stoul(value);
    
    // Create and add the process
    processes.push_back(Process(std::move(procName), arrivalTime, burstTimeCPU, burstTimeIO, burstTimeRate));
  }
  
  inputFile.close();
  return processes;
}

int main() {
  // Read processes from input file
  Processes procs = readProcessesFromFile("input.txt");
  
  // Check if processes were successfully read
  if (procs.empty()) {
    std::cerr << "No processes were read from the input file." << std::endl;
    return 1;
  }
  
  Device d;
  d.init(procs);
  d.processor();
  d.debug();

  return 0;
}