#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stropts.h>

int main ( int argc, char ** argv ) {

    // int foo[16], bar[16];
    FILE * stream;

    struct winsize ws;
    ioctl ( 0, TIOCGWINSZ, &ws );

    /*
    for ( int i = 0; i < 16; i++ ) {
        foo[i] = i; bar[i] = 0;
    }
    */

    stream = fopen ( argv[1], "r" );

    // fprintf ( stdout, "fwrite\n" );
    // fwrite ( &foo[0], sizeof ( int ), 16, stream );

    // fprintf ( stdout, "rewind\n" );
    // rewind ( stream );

    fseek ( stream, 0, SEEK_END );
    long len = ftell ( stream );
    rewind ( stream );

    unsigned int * bar = calloc ( len, sizeof ( unsigned int ) );

    fread ( bar, sizeof ( unsigned int ), len / sizeof ( unsigned int ), stream );

    fclose ( stream );

    fprintf ( stdout, "# of values: %li\n", len / sizeof ( unsigned int ) );

    int cmp ( const void *a, const void *b ) {
        return *(unsigned int *)a > *(unsigned int *)b;
    }

    qsort ( bar
          , len / sizeof ( unsigned int )
          , sizeof ( unsigned int )
          , cmp
          );

    for ( int i = 0; i < len / sizeof ( unsigned int ); i++ ) {
        if ( bar[i] < 10 ) { bar[i] = 0; continue; }
        int dup = 0;
        for ( int j = 0; j < len / sizeof ( unsigned int ); j++ ) {
            if ( bar[i] == bar[j] && dup++ > 0 ) bar[i] = 0;
        }
    }

    int count = 0;
    unsigned int * foo = NULL;
    for ( int i = 0; i < len / sizeof ( unsigned int ); i++ ) {
        if ( bar[i] != 0 ) {
            foo = realloc ( foo, ++count * sizeof ( unsigned int ) );
            foo[count-1] = bar[i];
        }
    }

    free ( bar );
    len = count * sizeof ( unsigned int );
    bar = foo;

    unsigned int max = 0;
    for ( int i = 0; i < len / sizeof ( unsigned int ); i++ ) {
        if ( bar[i] > max ) max = bar[i];
    }

    unsigned int maxlen = 0;
    while ( max > 0 ) {
        maxlen++; max /= 10;
    }
    maxlen += 1;

    int printed = 0;
    for ( int i = 0; i < len / sizeof ( unsigned int ); i++ ) {
        if ( printed >= ws.ws_col - maxlen ) {
            fprintf ( stdout, "\n" );
            printed = 0;
        }
        printed += fprintf ( stdout, "%-*u ", maxlen, bar[i] );
    }
    fprintf ( stdout, "\n" );

    // uncomment this to write the cleared list back
    /*
    stream = fopen ( argv[1], "w" );

    fwrite ( &foo[0], sizeof ( unsigned int ), count, stream );

    fclose ( stream );
    */

    free ( foo );

    // free ( bar );

    return 0;
}
