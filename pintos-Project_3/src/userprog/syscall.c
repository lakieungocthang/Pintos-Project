#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/off_t.h"

static void syscall_handler (struct intr_frame *);

struct lock filesys_lock;

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // Switch based on the system call number.
  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT:
      halt(); // System call to halt the system.
      break;
    case SYS_EXIT:
      validate(f->esp + 4); // Validate the user pointer.
      exit(*(uint32_t *)(f->esp + 4)); // System call to exit with a status.
      break;
    case SYS_EXEC:
      validate(f->esp + 4); // Validate the user pointer.
      f->eax = exec((const char *)*(uint32_t *)(f->esp + 4)); // System call to execute a process.
      break;
    case SYS_WAIT:
      validate(f->esp + 4); // Validate the user pointer.
      f->eax = wait((pid_t)*(uint32_t *)(f->esp + 4)); // System call to wait for a child process.
      break;
    case SYS_CREATE:
      validate(f->esp + 4); // Validate the user pointer.
      validate(f->esp + 8); // Validate the user pointer.
      f->eax = create((const char *)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8)); // System call to create a file.
      break;
    case SYS_REMOVE:
      validate(f->esp + 4); // Validate the user pointer.
      f->eax = remove((const char*)*(uint32_t *)(f->esp + 4)); // System call to remove a file.
      break;
    case SYS_OPEN:
      validate(f->esp + 4); // Validate the user pointer.
      f->eax = open((const char*)*(uint32_t *)(f->esp + 4)); // System call to open a file.
      break;
    case SYS_FILESIZE:
      validate(f->esp + 4); // Validate the user pointer.
      f->eax = filesize((int)*(uint32_t *)(f->esp + 4)); // System call to get the size of a file.
      break;
    case SYS_READ:
      validate(f->esp + 4); // Validate the user pointer.
      validate(f->esp + 8); // Validate the user pointer.
      validate(f->esp + 12); // Validate the user pointer.
      f->eax = read((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12))); // System call to read from a file.
      break;
    case SYS_WRITE:
      f->eax = write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12))); // System call to write to a file.
      break;
    case SYS_SEEK:
      validate(f->esp + 4); // Validate the user pointer.
      validate(f->esp + 8); // Validate the user pointer.
      seek((int)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8)); // System call to change the current position in a file.
      break;
    case SYS_TELL:
      validate(f->esp + 4); // Validate the user pointer.
      f->eax = tell((int)*(uint32_t *)(f->esp + 4)); // System call to get the current position in a file.
      break;
    case SYS_CLOSE:
      validate(f->esp + 4); // Validate the user pointer.
      close((int)*(uint32_t *)(f->esp + 4)); // System call to close a file.
      break;
  }
  // printf ("system call!\n");
  // thread_exit ();
}

struct file
{
  struct inode *inode;   /* Pointer to the file's inode structure. */
  off_t pos;             /* Current position within the file. */
  bool deny_write;       /* Flag indicating if file_deny_write() has been called. */
};

// Validates if a user virtual address is within the user space.
static void validate(const void *vaddr) {
    if (!is_user_vaddr(vaddr)) {
        exit(-1);
    }
}

// Halts the system.
void halt(void) {
    shutdown_power_off();
}

// Exits the current thread with a given status.
void exit(int status) {
    int i;

    // Print exit message with thread name and status.
    printf("%s: exit(%d)\n", thread_name(), status);
    
    // Set exit status for the current thread.
    thread_current()->exit_status = status;

    // Close all open files for the current thread.
    for (i = 3; i < 128; i++) {
        if (thread_current()->fd[i] != NULL) {
            close(i);
        }
    }

    // Exit the current thread.
    thread_exit();
}

// Executes a new process with the provided command line.
pid_t exec(const char *cmd_line) {
    return process_execute(cmd_line);
}

// Waits for a child process with a given PID to terminate.
int wait(pid_t pid) {
    return process_wait(pid);
}

// Creates a new file with a given name and initial size.
bool create(const char *file, unsigned initial_size) {
    // Ensure file name is not NULL and is within valid user space.
    if (file == NULL) {
        exit(-1);
    }
    validate(file);

    // Attempt to create the file using the file system.
    return filesys_create(file, initial_size);
}

// Removes a file with a given name.
bool remove(const char *file) {
    // Ensure file name is not NULL and is within valid user space.
    if (file == NULL) {
        exit(-1);
    }
    validate(file);

    // Attempt to remove the file using the file system.
    return filesys_remove(file);
}

// Opens a file with a given name and returns its file descriptor.
int open(const char *file) {
    int i;
    int ret = -1;
    struct file *fp;

    // Ensure file name is not NULL and is within valid user space.
    if (file == NULL) {
        exit(-1);
    }
    validate(file);

    // Acquire file system lock to perform file operations.
    lock_acquire(&filesys_lock);

    // Attempt to open the file and deny write if the file has the same name as the current thread.
    fp = filesys_open(file);
    if (fp == NULL) {
        ret = -1;
    } else {
        for (i = 3; i < 128; i++) {
            if (thread_current()->fd[i] == NULL) {
                if (strcmp(thread_current()->name, file) == 0) {
                    file_deny_write(fp);
                }
                thread_current()->fd[i] = fp;
                ret = i;
                break;
            }
        }
    }

    // Release file system lock after completing file operations.
    lock_release(&filesys_lock);
    return ret;
}

// Returns the size of a file with a given file descriptor.
int filesize(int fd) {
    // Ensure file descriptor is valid.
    if (thread_current()->fd[fd] == NULL) {
        exit(-1);
    }

    // Return the length (size) of the file.
    return file_length(thread_current()->fd[fd]);
}

// Reads data from a file into a buffer.
int read(int fd, void *buffer, unsigned size) {
    int i;
    int ret;

    // Validate buffer address.
    validate(buffer);

    // Acquire file system lock to perform file operations.
    lock_acquire(&filesys_lock);

    if (fd == 0) {
        // Reads from the console (stdin).
        for (i = 0; i < size; i++) {
            if (((char *)buffer)[i] == '\0') {
                break;
            }
        }
        ret = i;
    } else if (fd > 2) {
        // Reads from a file.
        if (thread_current()->fd[fd] == NULL) {
            exit(-1);
        }
        ret = file_read(thread_current()->fd[fd], buffer, size);
    }

    // Release file system lock after completing file operations.
    lock_release(&filesys_lock);
    return ret;
}

// Writes data from a buffer to a file.
int write(int fd, const void *buffer, unsigned size) {
    int ret = -1;

    // Validate buffer address.
    validate(buffer);

    // Acquire file system lock to perform file operations.
    lock_acquire(&filesys_lock);

    if (fd == 1) {
        // Writes to the console (stdout).
        putbuf(buffer, size);
        ret = size;
    } else if (fd > 2) {
        // Writes to a file.
        if (thread_current()->fd[fd] == NULL) {
            lock_release(&filesys_lock);
            exit(-1);
        }
        if (thread_current()->fd[fd]->deny_write) {
            file_deny_write(thread_current()->fd[fd]);
        }
        ret = file_write(thread_current()->fd[fd], buffer, size);
    }

    // Release file system lock after completing file operations.
    lock_release(&filesys_lock);
    return ret;
}
