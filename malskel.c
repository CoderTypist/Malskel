// Author: Christian Bargraser

// Before deployment:
// - set bDebug to false

// To make static binary analysis more difficult consider:
// - removing all DEBUG print statements

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

// monitor() sleep time within spinlock
#define NAP_TIME 0.25

#define DEBUG if( true == bDebug )
bool bDebug = true;

typedef struct monitor_args {
    int iMode;
    pid_t pid_monitor; // pid to monitor
} monitor_args;

typedef struct payload_args {
    int argc;
    char **argv;
} payload_args;

void* monitor(void *arg);
void* payload(void *arg);
payload_args* get_payload_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {

    pthread_t tid_payload;
    pthread_t tid_monitor;
    pid_t pid;

    DEBUG fprintf(stderr, "Mode: LAUNCH\n");

    pid = fork();

    if( pid < 0 ) {
        
        DEBUG fprintf(stderr, "\nFork failed\n");
        exit(-1);
    }

    // child: execute payload and monitor the parent
    else if( 0 == pid ) {

        // execute the payload
        pthread_create(&tid_payload, (void*)NULL, payload, (void*)get_payload_args(argc, argv));
        DEBUG fprintf(stderr, "    Thread (payload->payload): tid = %ld from pid = %d\n", tid_payload, getpid());

        // monitor the parent
        pid_t pid_parent = getppid();
        monitor_args ma = { PAYLOAD, pid_parent };
        pthread_create(&tid_monitor, (void*)NULL, monitor, &ma);
        DEBUG fprintf(stderr, "    Thread (payload->monitor): tid = %ld from pid = %d\n", tid_monitor, getpid());

        pthread_join(tid_payload, (void*)NULL);
        pthread_join(tid_monitor, (void*)NULL);
    }

    // parent: monitor the the child (payload)
    else {

        monitor_args ma = { MONITOR, pid };
        pthread_create(&tid_monitor, (void*)NULL, monitor, &ma);
        DEBUG fprintf(stderr, "    Thread (monitor->monitor): tid = %ld from pid = %d\n", tid_monitor, getpid());
        pthread_join(tid_monitor, (void*)NULL);

        while( -1 != wait(NULL) );
    }
}

void* monitor(void *arg) {
    
    monitor_args ma = *((monitor_args*)arg);
    pid_t pid_monitor = ma.pid_monitor;
    DEBUG fprintf(stderr, "Monitor: pid = %d -> pid = %d\n", getpid(), ma.pid_monitor);

    while ( true ) {

        // free the PCB if the process is dead
        waitpid(pid_monitor, NULL, WNOHANG);

        // check to see if the process is dead
        if( 0 != kill(pid_monitor, 0) ) {

            // if the current process is a PAYLAOD
            if( MONITOR == ma.iMode ) {
                fprintf(stderr, "Monitor: payload process was killed\n");

                // WORKS! Dead process is detected
                // payload_args
                // fork -> payload thread AND monitor thread
            }

            // if the current process is a MONITOR
            else if ( PAYLOAD == ma.iMode ) {
                fprintf(stderr, "Payload: monitor process was killed\n");

                // WORKS! Dead process is detected
                // monitor_args
                // fork -> call monitor (no need for a thread)
            }

            // unexpected behavior
            else {
                DEBUG fprintf(stderr, "Unexpected beavior\n");
                exit(-1);
            }
        }

        // adjust sleep time as needed
        sleep(NAP_TIME);
    }

    return NULL;
}

void* payload(void *arg) {

    // process arg
    payload_args pa = *((payload_args*)arg);
    int argc = pa.argc;
    char** argv = pa.argv;

    // -- remove later --
    // WORKS! Arguments are received
    // fprintf(stderr, "Payload: argc = %d\n", argc);
    // fprintf(stderr, "Payload: argv[0] = %s\n", argv[0]);

    DEBUG fprintf(stderr, "Payload: pid = %d\n", getpid());

    sleep(100);

    return NULL;
}

payload_args* get_payload_args(int argc, char *argv[]) {

    payload_args *ppa = (payload_args*)malloc(sizeof(payload_args));

    if( NULL == ppa ) {
        DEBUG fprintf(stderr, "Error: Malloc for ppa failed\n");
        exit(-1);
    }

    // if there are no payload arguments
    if( 1 == argc ) {
        ppa->argc = 0;
        ppa->argv = NULL;
        return ppa;
    }

    // char** that will point to payload arguments in argv
    fprintf(stderr, "argc: %d\n", argc);
    char** p_argv = (char**)malloc( (argc-1) * sizeof(char*) );

    if( NULL == p_argv ) {
        DEBUG fprintf(stderr, "Error: Malloc for p_argv failed\n");
        exit(-1);
    }

    // make the char** point to the arguments in argv
    int i;
    for( i = 0 ; i < argc-1 ; i++ ) {
        p_argv[i] = argv[i+1];
        // fprintf(stderr, "p_argv[%d] = %s\n", i, p_argv[i]);
    }

    ppa->argc = argc-1;
    ppa->argv = p_argv;
    
    return ppa;
}