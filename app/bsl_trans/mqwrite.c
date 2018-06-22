#include <stdio.h>

int main(void)
{
    int fileno;
    int written;
    char pUserBuffer[0x1000];
    int writesize = 0x1000;

    fileno = open( "xxx.xxx", O_RDWR, 0667 );
    if( fileno == -1 )
    {
        printf("%s: open error - %s\n",__FUNCTION__, extfilename );
        return;
    }

    for( int i=0; i<100; i++ )
    {
        written = write( fileno, pUserBuffer, writesize );
        if( written != writesize )
        {
            printf("%s:%d written %d is different from transfersize=%d\n", __FUNCTION__, __LINE__, written, transfersize );
        }
        usleep(900000);
    }

    close( fileno );
    return 0;
} 
