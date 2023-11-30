# Threads Synchronization for News Broadcasting Simulation

**Objective:** Implemented a concurrent programming project to simulate a news broadcasting system. Explored synchronization mechanisms to manage multiple threads efficiently.

**Key Components:**
- **Producers:** Generated strings representing news stories of various types.
- **Dispatcher:** Sorted and distributed news stories to respective editors.
- **Co-Editors:** Edited received messages before forwarding to the screen manager.
- **Screen Manager:** Displayed edited news stories to the screen and managed final output.

**Technical Details:**
- Implemented a bounded buffer with thread-safe operations for inter-thread communication.
- Utilized synchronization constructs such as semaphores to manage access to shared resources.
- System configuration specified in a file format, allowing flexibility in defining producer tasks and queue sizes.
  
**Note:** Communication between threads handled via a shared bounded buffer. Global buffer declaration or passing buffer pointers during thread creation are suggested solutions.

**Submission Command:**
```bash
ex3.out config.txt
```
