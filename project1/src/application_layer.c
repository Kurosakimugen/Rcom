// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort, serialPort);
    if (strcmp("tx",role))
        linkLayer.role = LlTx; else linkLayer.role = LlRx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    int fd = llopen(linkLayer);

    if (fd < 0)
    {
        printf("\nllopen Failed\n");
        exit(-1);
    }


    if (linkLayer.role == LlTx)
    {
        FILE* file = fopen(filename,"rb");
        if (file == NULL)
        {
            printf("\nCould not perform fopen\n");
            exit(-1);
        }
    }
    else if (linkLayer.role == LlRx)
    {

    }
}
