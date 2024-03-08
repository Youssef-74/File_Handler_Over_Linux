#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "../Logger/Logger.hpp"


const char* PIPE_NAME = "/tmp/named_Pipe";
const int SHARED_MEMORY_SIZE = 1024;
struct SharedMemory {
  char data[SHARED_MEMORY_SIZE];
};

/* semaphore for log file to synchronize the writing in run_logs.og firl between the two programs */
sem_t* sem_log = sem_open("/sem_log", 0);


int main(int argc, char* argv[]) {
  if (argc < 3) {
    if(sem_wait(sem_log) == 1)
    {
        std::cerr << "Info: Usage: ./requester <-l dir> <-r filename>" << std::endl;
        Log_Message("Info: Usage: ./requester <-l dir> <-r filename>");
        sem_post(sem_log);
    }
    sem_close(sem_log);
    sem_unlink("/sem_log");
    return 1;
  }

  /*Extract the Command and the dir*/
  std::string command = argv[1];
  std::string path = argv[2];
  /*connecting to the named pipe*/
  int pipe_fd = open(PIPE_NAME, O_WRONLY);
  if (pipe_fd == -1) {
    //std::cerr << "Error: Failed to open named pipe." << std::endl;
    Log_Message("Error: Failed to open named pipe.");
    sem_close(sem_log);
    sem_unlink("/sem_log");
    return 1;
  }
  /*Creat a semaphore for mutual execlusion and synch*/
  sem_t* semaphore = sem_open("/sem_task",O_CREAT,0644, 0 );
  if (semaphore == SEM_FAILED) {
    //std::cerr << "Error:Failed to create/open semaphore." << std::endl;
    Log_Message("Error:Failed to create/open semaphore.");
    sem_close(sem_log);
    sem_unlink("/sem_log");
    sem_close(sem_log);
    sem_unlink("/sem_log");
    close(pipe_fd);
    return 1;
  }
  /* shared memory */
  int shm_fd = shm_open("/shm_task", O_RDWR, 0644); /* empty to begin */
  if (shm_fd == -1) {
    //std::cerr << "Can't get file descriptor to the shared memory" << std::endl;
    Log_Message("Error:Can't get file descriptor to the shared memory");
    sem_close(sem_log);
    sem_unlink("/sem_log");
    sem_close(semaphore);
    sem_unlink("/sem_task");
    close(pipe_fd);
    unlink("/shm_task");
    return 1;
  }
  /*Accessing the shared memory*/
  SharedMemory* shared_memory = static_cast<SharedMemory*>(
      mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE,
           MAP_SHARED, shm_fd, 0));
  if (shared_memory == MAP_FAILED) {
    //std::cerr << "Failed to map shared memory." << std::endl;
    Log_Message("Error: Failed to map shared memory." );
    sem_close(sem_log);
    sem_unlink("/sem_log");
    sem_close(semaphore);
    sem_unlink("/sem_task");
    close(pipe_fd);
    unlink("/shm_task");
    return 1;
  }
  // Send the request to the File Handler
  std::string request = command + " " + path;
  if (write(pipe_fd, request.c_str(), request.length() + 1) == -1) {
   // std::cerr << "Failed to write to the named pipe." << std::endl;
   Log_Message("Error: Failed to write to the named pipe.");
    munmap(shared_memory, sizeof(SharedMemory));
    sem_close(sem_log);
    sem_unlink("/sem_log");
    sem_close(semaphore);
    sem_unlink("/sem_task");
    close(pipe_fd);
    unlink("/shm_task");
    return 1;
  } else {
    //std::cout << "The Request has been written to the pipe successfully!"<<std::endl;
    Log_Message("Info: The Request has been written to the pipe successfully.");
  }
  // Wait for the response from the File Handler
  if (sem_wait(semaphore) == -1) {
    //std::cerr << "Failed to wait on the semaphore." << std::endl;
    Log_Message("Error: Failed to wait on the semaphore.");
    munmap(shared_memory, sizeof(SharedMemory));
    sem_close(sem_log);
    sem_unlink("/sem_log");
    sem_close(semaphore);
    sem_unlink("/sem_task");
    close(pipe_fd);
    unlink("/shm_task");
    return 1;
  }
    //std::cout << "The Request has been handled successfully!" << std::endl;
    Log_Message("Info: The Request has been handled successfully.");
    // Read the response from shared memory
    std::cout << shared_memory->data << std::endl;

  /*Clean Up*/
  munmap(shared_memory, sizeof(SharedMemory));
  sem_close(sem_log);   
  sem_unlink("/sem_log");
  sem_close(semaphore);
  sem_unlink("/sem_task");
  close(pipe_fd);
  unlink("/shm_task");
  return 0;
}