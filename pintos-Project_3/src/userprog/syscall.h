#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"

// Initializes the system call interface.
void syscall_init(void);

// Shuts down the operating system.
void halt(void);

// Exits the current user program with a status code.
void exit(int status);

// Executes a new program.
pid_t exec(const char *cmd_line);

// Waits for a child process to terminate.
int wait(pid_t pid);

// Creates a new file.
bool create(const char *file, unsigned initial_size);

// Removes a file.
bool remove(const char *file);

// Opens a file.
int open(const char *file);

// Retrieves the size of a file.
int filesize(int fd);

// Reads data from a file.
int read(int fd, void *buffer, unsigned size);

// Writes data to a file.
int write(int fd, const void *buffer, unsigned size);

// Changes the next byte to be read or written in an open file.
void seek(int fd, unsigned position);

// Returns the position of the next byte to be read or written in an open file.
unsigned tell(int fd);

// Closes a file.
void close(int fd);

#endif /* userprog/syscall.h */
