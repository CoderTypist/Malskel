// Author: Christian Bargraser

// Error messages will not be included in the final version
// The host should not get these messages

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LAUNCH  1
#define MONITOR 2
#define PAYLOAD 3

void* monitor(void *arg);
void* payload(void *arg);

int main(int argc, char *argv[]) {

    // a mode must be provided    
    if( argc <= 1 ) {

        // -- remove later --
        printf("\nNot enough arguments\n");
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

        // -- remove later --
        printf("Invalid strMode\n");

        exit(-1);
    }

    pthread_t tid_payload;
    pthread_t tid_monitor;
    
    // action to take based on the mode
    if( LAUNCH == iMode ) {

        fprintf(stderr, "\n  -- LAUNCH -- \n");

        pid_t pid = fork();

        if( pid < 0 ) {
            
            // -- remove later --
            fprintf(stderr, "\nFork failed\n");
            exit(-1);
        }

        // execute payload and monitor the process monitoring this process
        else if( 0 == pid ) {
            
            iMode = PAYLOAD;

            // execute the payload
            pthread_create(&tid_payload, (void*)NULL, payload, (void*)NULL);

            // monitor the parent
            pid_t pid_parent = getppid();
            pthread_create(&tid_monitor, (void*)NULL, monitor, &pid_parent);

            pthread_join(tid_payload, (void*)NULL);
            pthread_join(tid_monitor, (void*)NULL);
        }

        // monitor the process executing the payload
        else {
            
            iMode = MONITOR;

            // monitor the child (payload)
            pthread_create(&tid_monitor, (void*)NULL, monitor, &pid);
            pthread_join(tid_monitor, (void*)NULL);

            while( -1 != wait(NULL) );
        }
    }

    else if ( MONITOR == iMode && argc >= 3 ) {

        fprintf(stderr, "\n  -- MONITOR -- \n");

        pthread_t pid_monitor = (pid_t)atoi(argv[2]);
        pthread_create(&tid_monitor, (void*)NULL, monitor, &pid_monitor);
        pthread_join(tid_monitor, NULL);
    }

    else if ( PAYLOAD == iMode && argc >= 3 ) {
        
        fprintf(stderr, "\n  -- PAYLOAD -- \n");

        pthread_create(&tid_payload, (void*)NULL, payload, (void*)NULL);
        pthread_join(tid_payload, NULL);
    }

    else {

        // -- remove later --
        printf("Invalid iMode\n");

        exit(-1);
    }
}

void* monitor(void *arg) {
    
    fprintf(stderr, "I am the monitor\n");
    return NULL;
}

void* payload(void *arg) {

    fprintf(stderr, "I am the payload\n");
    return NULL;
}