#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>   
#include <iostream>
#include <ostream>
#include <queue>
#include <sstream>    
#include <string>
#include <utility>
#include <vector>

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
  size_t burstTimeRate = SIZE_MAX;  
  size_t startTime = SIZE_MAX;
  size_t completionTime = 0;
  size_t burstRemainCPU = SIZE_MAX;
  size_t lastIOBurst = 0;
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
    state = READY;
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
  size_t turnAroundTime() const { return completionTime - arrivalTime; }
  size_t waitingTime() const { return turnAroundTime() - burstTimeCPU; }
  size_t responseTime() const { return startTime - arrivalTime; }
} Process;

typedef std::vector<Process> Processes;

struct CompareProcess {
  bool operator()(const Process& left, const Process& right) const {
    if (left.burstTimeCPU != right.burstTimeCPU) {
      return left.burstTimeCPU > right.burstTimeCPU;
    }
    return left.arrivalTime > right.arrivalTime;
  }
};

class Device {
 public:
  Device() : readyQ(CompareProcess()), ioQ(CompareProcess()) {}

  void init(const Processes& procs) {
    this->procs = procs;
    totalProc = procs.size();
  }

  void processor() {
    Process execProc;  
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
          LOG("\t", "CPU", execProc.procName << "[Q IO]:" << execProc.burstRemainCPU);
          ioQ.push(execProc);
          isCPUIdle = true;
          execProc = {};
        } else {
          LOG("\t", "CPU", execProc.procName << ":" << execProc.burstRemainCPU)
        }
      }

      if (isCPUIdle && !readyQ.empty()) {
        execProc = readyQ.top();
        readyQ.pop();
        LOG("\t", "CPU", execProc.procName << "[Sched]")
        if (execProc.startTime == SIZE_MAX) {
          execProc.startTime = ticksCPU;
        }
        isCPUIdle = false;
      }

      ioDevice();

      ticksCPU++;
      std::cout << "\n";
    }
  }

  // Debug info after all processes finish
  void debug() {
    for (auto& proc : completedProcs) {
      LOG_DEBUG(proc.procName, "Arrival Time:\t", proc.arrivalTime)
      LOG_DEBUG("", "Start Time:\t\t", proc.startTime)
      LOG_DEBUG("", "Response Time:\t", proc.responseTime())
      LOG_DEBUG("", "Completion Time:", proc.completionTime)
      LOG_DEBUG("", "Turnaround Time:", proc.turnAroundTime())
      LOG_DEBUG("", "Waiting Time:\t", proc.waitingTime() << "\n")
    }
    std::cout << "Avg Waiting Time: " << avgWaitingTime() << "\n";
    std::cout << "Avg Reponse Time: " << avgResponseTime() << "\n";
  }

  double avgWaitingTime() {
    double sum = 0.0;
    for (auto& proc : completedProcs) {
      sum += proc.waitingTime();
    }
    return (completedProcs.empty()) ? 0.0 : sum / completedProcs.size();
  }

  double avgResponseTime() {
    double sum = 0.0;
    for (auto& proc : completedProcs) {
      sum += proc.responseTime();
    }
    return (completedProcs.empty()) ? 0.0 : sum / completedProcs.size();
  }

 private:
  Processes completedProcs;               
  Processes procs;                        
  size_t totalProc = 0;                   
  size_t ticksCPU = 0;                    
  bool isCPUIdle = true;                  

  size_t countIOBurst = 0;                
  bool isIOIdle = true;                   
  Process execProcIO;                     

  std::priority_queue<Process, Processes, CompareProcess> readyQ;  
  std::priority_queue<Process, Processes, CompareProcess> ioQ;

  void ioDevice() {
    if (!isIOIdle) {
      if (++countIOBurst >= execProcIO.burstTimeIO) {
        LOG("\t", "IO", execProcIO.procName << "[Comp]:" << countIOBurst)
        readyQ.push(execProcIO);
        execProcIO = {};
        isIOIdle = true;
      } else {
        LOG("\t", "IO", execProcIO.procName << ":" << countIOBurst)
      }
    }

    if (isIOIdle && !ioQ.empty()) {
      execProcIO = ioQ.top();
      ioQ.pop();
      countIOBurst = 0;
      isIOIdle = false;
      LOG("\t", "IO", execProcIO.procName << "[Sched]:" << countIOBurst)
    }
  }

  void FreshArrivals() {
    int index = 0;
    while (index < (int)procs.size()) {
      if (procs[index].arrivalTime == ticksCPU) {
        LOG("\t", "CPU", procs[index].procName << "[Arrive]")
        procs[index].state = Process::State::READY;
        readyQ.push(procs[index]);
        procs.erase(procs.begin() + index);
      } else {
        index++;
      }
    }
  }
};

Processes readProcessesFromFile(const std::string& filename) {
  Processes procs;
  std::ifstream fin(filename);
  if (!fin) {
    std::cerr << "Error opening file: " << filename << "\n";
    return procs; 
  }

  std::string line;
  while (std::getline(fin, line)) {
    if (line.empty()) continue;

    
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, ';')) {
      tokens.push_back(token);
    }
    if (tokens.size() == 5) {
      std::string name = tokens[0];
      size_t arrTime   = std::stoul(tokens[1]);
      size_t cpuTime   = std::stoul(tokens[2]);
      size_t ioTime    = std::stoul(tokens[3]);
      size_t chunkRate = std::stoul(tokens[4]);

      procs.push_back(Process(std::move(name), arrTime, cpuTime, ioTime, chunkRate));
    }
  }

  return procs;
}

int main() {
  Processes procs = readProcessesFromFile("processes.txt");

  Device d;
  d.init(procs);
  d.processor();
  d.debug();

  return 0;
}
