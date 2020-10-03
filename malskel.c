// Author: Christian Bargraser

// Error messages will not be included in the final version
// The host should not get these messages

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LAUNCH  1
#define MONITOR 2
#define PAYLOAD 3

int main(int argc, char *argv[]) {

    // a mode must be provided    
    if( argc <= 1 ) {

        // -- remove later --
        printf("\nNot enough arguments\n");
        exit(-1);
    }

    // get the mode
    int iMode;
    char *strMode = argv[1];

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

    // action to take based on the mode
    if( LAUNCH == iMode ) {

        printf("\n  -- LAUNCH -- \n");
    }

    else if ( MONITOR == iMode && argc >= 3 ) {

        printf("\n  -- MONITOR -- \n");
    }

    else if ( PAYLOAD == iMode && argc >= 3 ) {

        printf("\n  -- PAYLOAD -- \n");
    }

    else {

        // -- remove later --
        printf("Invalid iMode\n");

        exit(-1);
    }
}
