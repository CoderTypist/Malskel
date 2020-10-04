// Author: Christian Bargraser

// Before deployment:
// - set bDebug to false

// To make static binary analysis more difficult consider:
// - removing all print statements
// - getting rid of the usage function

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LAUNCH  1
#define MONITOR 2
#define PAYLOAD 3

#define DEBUG if( true == bDebug )
bool bDebug = true;

void* monitor(void *arg);
void* payload(void *arg);
void usage();

int main(int argc, char *argv[]) {

    // a mode must be provided    
    if( argc <= 1 ) {

        DEBUG { 
            printf("Not enough arguments\n\n");
            usage();
        }

        exit(-1);
    }

    // get the mode
    int iMode;

    if( 0 == strcmp(argv[1], "-l") || 0 == strcmp(argv[1], "--launch") ) {
        iMode = LAUNCH;
    }

    else if( 0 == strcmp(argv[1], "-m") || 0 == strcmp(argv[1], "--monitor") ) {
        iMode = MONITOR;
    }

    else if( 0 == strcmp(argv[1], "-p") || 0 == strcmp(argv[1], "--payload") ) {
        iMode = PAYLOAD;
    }

    else {

        DEBUG printf("Invalid mode: %s\n", argv[1]);
        exit(-1);
    }
    
    // action to take based on the mode
    if( LAUNCH == iMode ) {

        DEBUG fprintf(stderr, "Mode: LAUNCH\n");

        pthread_t tid_payload;
        pthread_t tid_monitor;
        pid_t pid = fork();

        if( pid < 0 ) {
            
            DEBUG fprintf(stderr, "\nFork failed\n");
            exit(-1);
        }

        // execute payload and monitor the process monitoring this process
        else if( 0 == pid ) {
            
            iMode = PAYLOAD;

            // execute the payload
            pthread_create(&tid_payload, (void*)NULL, payload, (void*)NULL);
            DEBUG fprintf(stderr, "Thread (payload->payload): tid = %ld from pid = %d\n", tid_payload, getpid());

            // monitor the parent
            pid_t pid_parent = getppid();
            pthread_create(&tid_monitor, (void*)NULL, monitor, &pid_parent);
            DEBUG fprintf(stderr, "Thread (payload->monitor): tid = %ld from pid = %d\n", tid_monitor, getpid());

            pthread_join(tid_payload, (void*)NULL);
            pthread_join(tid_monitor, (void*)NULL);
        }

        // monitor the process executing the payload
        else {
            
            iMode = MONITOR;

            // monitor the child (payload)
            pthread_create(&tid_monitor, (void*)NULL, monitor, &pid);
            DEBUG fprintf(stderr, "Thread (monitor->monitor): tid = %ld from pid = %d\n", tid_monitor, getpid());
            pthread_join(tid_monitor, (void*)NULL);

            while( -1 != wait(NULL) );
        }
    }

    else if ( MONITOR == iMode && argc >= 3 ) {

        DEBUG fprintf(stderr, "Mode: MONITOR\n");
        pid_t pid_monitor = (pid_t)atoi(argv[2]);
        monitor(&pid_monitor);
    }

    else if ( PAYLOAD == iMode && argc >= 3 ) {
        
        DEBUG fprintf(stderr, "Mode: PAYLOAD\n"); 
        payload((void*)NULL);
    }

    else {

        DEBUG {
            printf("Invalid mode and/or arguments\n\n");
            usage();
        }

        exit(-1);
    }
}

void* monitor(void *arg) {
    
    pid_t pid_monitor = *((pid_t*)arg);
    DEBUG fprintf(stderr, "Monitor: pid = %d -> pid = %d\n", getpid(), pid_monitor);
    return NULL;
}

void* payload(void *arg) {

    DEBUG fprintf(stderr, "Payload: pid = %d\n", getpid());
    return NULL;
}

void usage() {

    printf("./malskel -l/--launch <payload_args>\n");
    printf("./malskel -m/--monitor <pid_monitor> <payload_args>\n");
    printf("./malskel -p/--payload <pid_monitor> <payload_args>\n");
}