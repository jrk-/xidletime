#include "stdio.h"
#include <stdlib.h>

#include "kMeans.h"

#define NVALUES 10
#define CLUSTERSIZE 8

void readMeans ( void (*func )( unsigned int * ), char * fileName );

int main ( int argc, char ** argv ) {

    cluster_t cluster;
    makeCluster ( &cluster, strtol ( argv[1], NULL, 10 ), argv[2] );

    printMeans ( &cluster );
    finalizeCluster ( &cluster );

}

void readMeans ( void (*func )( unsigned int * ), char * fileName ) {

    FILE * stream;

    stream = fopen ( fileName, "r" );

    fseek ( stream, 0, SEEK_END );
    long len = ftell ( stream );
    rewind ( stream );

    unsigned int * buf = calloc ( len, sizeof ( unsigned int ) );

    fread ( buf
          , sizeof ( unsigned int )
          , len / sizeof ( unsigned int )
          , stream );

    fclose ( stream );

    func ( buf );

    free ( buf );

}
