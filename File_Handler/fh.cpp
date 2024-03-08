#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "../Logger/Logger.hpp"

const char* PIPE_NAME = "/tmp/named_Pipe";
const int SHARED_MEMORY_SIZE = 1024;
struct SharedMemory {
  char data[SHARED_MEMORY_SIZE];
};
void HandelIncomingRequest(std::string request, SharedMemory* shm);
/* semaphore for log file to synchronize the writing in run_logs.og firl between the two programs */
sem_t* sem_log = sem_open("/sem_log", O_CREAT, 0644, 0);

int main() {
  mkfifo(PIPE_NAME, 0666);
  int pipe_fd = open(PIPE_NAME, O_RDONLY);
  if (pipe_fd == -1) {
    // std::cerr << "Failed to open named pipe." << std::endl;
    Log_Message("Error: Failed to open named pipe.");
    sem_close(sem_log);
    sem_unlink("/sem_log");
    return 1;
    } else {
    std::cout << "the Named Pipe has been created successfully!" << std::endl;
    Log_Message("Info: The Named Pipe has been created successfully");
    }
    sem_post(sem_log);
  
  
  sem_t* semaphore = sem_open("/sem_task", O_CREAT, 0644, 0);
  if (semaphore == SEM_FAILED) {
    // std::cerr << "Failed to create/open semaphore." << std::endl;
    Log_Message("Error: Failed to create/open semaphore.");
    sem_close(sem_log);
    sem_unlink("/sem_log");
    close(pipe_fd);
    return 1;
  }
  /*Shared Memory*/
  int shm_fd =
      shm_open("/shm_task", O_CREAT | O_RDWR, 0644); /* empty to begin */
  if (shm_fd == -1) {
    // std::cerr << "Can't get file descriptor to the shared memory" <<
    // std::endl;
    Log_Message("Error: Can't get file descriptor to the shared memory");

    sem_close(sem_log);
    sem_unlink("/sem_log");
    sem_close(semaphore);
    sem_unlink("/sem_task");
    close(pipe_fd);
    unlink("/shm_task");
    return 1;
  }
  ftruncate(shm_fd, sizeof(SharedMemory));
  SharedMemory* shared_memory = static_cast<SharedMemory*>(
      mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED,
           shm_fd, 0));
  if (shared_memory == MAP_FAILED) {
    // std::cerr << "Failed to map shared memory." << std::endl;
    Log_Message("Error : Failed to map shared memory.");
    sem_close(sem_log);      
    sem_unlink("/sem_log");
    sem_close(semaphore);
    sem_unlink("/sem_task");
    close(pipe_fd);
    unlink("/shm_task");
    return 1;
  }
  while (true) {
    char buffer[256];
    ssize_t bytes_read = 0;
    // Wait for a request from the requester
    do {
      ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer));
      if (bytes_read == -1) {
        // std::cerr << "Failed to read from the named pipe." << std::endl;
        Log_Message("Error: Failed to read from the named pipe.");
        break;
      } else if (bytes_read > 0) {
        // Process the request
        std::string request(buffer);
        // removing the content of the pipe after reading it
        int New_PipeFd = open(PIPE_NAME, O_WRONLY | O_TRUNC);
        if (New_PipeFd == -1) {
          // std::cerr << "Failed to clear pipe contents." << std::endl;
          Log_Message("Error: Failed to clear pipe contents.");
          return 1;
        }
        close(New_PipeFd);
        /*std::cout
            << "The Request is received and has been deleted from the pipe"
            << std::endl;*/
        Log_Message(
            "Info: The Request is received and has been deleted from the "
            "pipe.");
        // Signal the requester that the response is ready
        HandelIncomingRequest(request, shared_memory);
        if (sem_post(semaphore) == -1) {
          // std::cerr << "Failed to post on the semaphore." << std::endl;
          Log_Message("Error: Failed to post on the semaphore.");
          break;
        }
      }
    } while (bytes_read > 0);
    // Sleep to prevent busy waiting.
    sleep(1);
  }

  munmap(shared_memory, sizeof(SharedMemory));
  sem_close(sem_log); 
  sem_unlink("/sem_log");
  sem_close(semaphore);
  sem_unlink("/sem_task");
  close(pipe_fd);
  unlink(PIPE_NAME);
  unlink("/shm_task");

  return 0;
}
void HandelIncomingRequest(std::string request, SharedMemory* shm) {
  std::string command = request.substr(0, 2);  // Extract the command type
  std::string argument =
      request.substr(3);  // Extract the directory or file path
  if (command == "-l") {  // List command
    std::vector<std::string> files;
    DIR* dir = opendir(argument.c_str());
    if (dir) {
      struct dirent* entry;
      while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
          files.push_back(entry->d_name);
        }
      }
      closedir(dir);

      std::string file_list;
      for (const auto& file : files) {
        file_list += file + "\n";
      }
      strncpy(shm->data, file_list.c_str(), SHARED_MEMORY_SIZE);
    } else {
      // std::cerr << "Failed to open directory: " << argument << std::endl;
      strncpy(shm->data, "Error: Failed to open directory.",
              SHARED_MEMORY_SIZE);
      Log_Message("Error: Failed to open directory.");
    }
  } else if (command == "-r") {  // Read command
    std::ifstream file(argument);
    if (file) {
      std::string contents;
      std::string line;
      while (std::getline(file, line)) {
        contents += line + "\n";
      }
      file.close();

      strncpy(shm->data, contents.c_str(), SHARED_MEMORY_SIZE);
    } else {
      // std::cerr << "Failed to open file: " << argument << std::endl;
      strncpy(shm->data, "Error: Failed to open file.", SHARED_MEMORY_SIZE);
      Log_Message("Error: Failed to open file.");
    }
  } else {
    // std::cerr << "Invalid command: " << command << std::endl;
    strncpy(shm->data, "Error: Invalid command.", SHARED_MEMORY_SIZE);
    Log_Message("Error: Invalid command.");
  }
}