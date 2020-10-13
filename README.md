# Malskel

### Description

Make a program "unkillable".

### High Level Explanation

The process will fork. Each process will check on the other. 

One process will be in MONITOR mode and the other will be in PAYLOAD mode.

The process in MONITOR mode only checks on the process in PAYLOAD mode.

The process in PAYLOAD mode does two things. It executes the payload and checks on the process in MONITOR mode. 

If the process in MONITOR mode dies, the process in PAYLOAD mode will notice, fork, and put the child in MONITOR mode.

If the process in PAYLOAD mode dies, the process in MONITOR mode will notice and fork. The child will switch to PAYLOAD mode and restart the payload.

### Low Level Explanation

Malskel is started. 

The blue squiggly line is the default thread. 

// picture 01

P1 will then fork.

// picture 02

P1 is placed in MONITOR mode and P1 is placed in PAYLOAD mode. 

// picture 03

P1 will use its default thread to monitor P2.

P2 will create a thread to monitor P1.

P2 will create a thread to start the payload. 

// picture 04

At this point, there are now two different scenarios:

1) The process in MONITOR mode may be killed.

2) The process in PAYLOAD mode may be killed. 

##### If the process in PAYLOAD mode (P2) gets killed. 

Remember that P1 is in MONITOR mode and was using its default thread to monitor P2.

P1 will notice that P2 has been killed and will fork.

The child, denoted by P3, will be placed in PAYLOAD mode.

P3 will use its default thread to monitor P1.

P3 will create a new thread to restart the payload. 

P1 will now start monitoring P3.

// image 05

##### If the process in MONITOR mode (P1) gets killed. 

Remember that P2 is in PAYLOAD mode as was using one thread to monitor P1 and using the other thread to execute the payload.

P2 will notice that P1 has been killed and will fork.

It is important to note that the fork takes place inside of the thread that monitors P1.

This means that the child will not get a copy of the payload thread. The child only gets a copy of the monitor thread. 

The parent will still have its own copies of the payload thread and monitor thread. 

The parent still has the payload running.

P2's monitor thread will now start monitoring P3.

P3's monitor thread will now start monitoring P2.

// image 06


