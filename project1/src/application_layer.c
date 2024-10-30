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
    if (strcmp("tx",role) == 0)
        linkLayer.role = LlTx; else linkLayer.role = LlRx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;
    printf("will call llopen\n");
    int fd = llopen(linkLayer); // Start the connection
    printf("\nReturned llopen\n");

    if (fd < 0) // Case of an error on estabilishing a connection
    {
        printf ("\n llopen Failed \n");
        exit ( -1 );
    }


    if ( linkLayer.role == LlTx ) // Section related to the Transmitter
    {
        FILE* file = fopen ( filename , "rb" ); //Try to open the file

        if (file == NULL) // If wasn't possible to open the file
        {
            printf ( "\n Could not perform fopen\n" );
            llclose ( TRUE );
            exit ( -1 );
        }

        unsigned char buffer[MAX_PAYLOAD_SIZE]; // Set the max amount to send on each frame
        int AccRead; // Accumulator of read bytes

        while ( ( AccRead = fread ( buffer , 1 , MAX_PAYLOAD_SIZE , file ) ) > 0 ) //Loop to send the file
        {
            int BytesWritten = llwrite ( buffer , AccRead ); //Sends a part of the file

            if ( BytesWritten < 0 ) // Case of an error ocurring when sending the file
            {
                printf ( "\n Error sending the dataframe. \n" );
                fclose ( file );
                llclose ( TRUE );
                exit ( -1 );
            }
        }

        fclose(file); // Close the file once the loop is over and was well sent
        llclose(TRUE); // Close connection after sending the file and signals the receiver to close the connection
    }
    else if ( linkLayer.role == LlRx ) // Section related to the Receiver
    {
        FILE* file = fopen ( filename , "wb" ); // Open the file to send the data received

        if ( file == NULL ) // If wasn't possible to open the file
        { 
            printf ( "\n Could not open file to write the information. \n" );
            llclose ( TRUE );
            exit ( -1 );
        }

        unsigned char packet[MAX_PAYLOAD_SIZE]; // Variable so the packet comes with the same size as the one sent
        int AccRead; //Accumalator with the bytes readed
        
        while ( ( AccRead = llread ( packet ) ) > 0 ) // Loop to receive the file and sending acknolgdement
        {
            if ( AccRead < 0) // Case when an error occurs receiving the bit of infromation
            {
                printf ( " \n Error on receiving the chunk. \n " );
                fclose ( file );
                llclose ( TRUE );
                exit (-1);
            }
            
            int Write_file = fwrite ( packet , 1 , AccRead , file ); // Writes the information received on the file, and 

            if ( Write_file < 0 ) // Produces a message in case of an error
            {
                printf ( " \n Error writing the chunk in the file. \n " );
                fclose ( file );
                llclose ( TRUE );
                exit ( -1 );
            }
        }
        
        fclose ( file ); // Close the file once all the information has been written
        llclose ( TRUE ); // Close connection when the file is over receiveing
    }
    
}