// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Show statistics variables
extern int frameCounter;

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
    int fd = llopen(linkLayer); // Start the connection

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
        int fileStart = ftell(file);
        fseek(file, 0, SEEK_END);
        int fileEnd = ftell(file);
        fseek(file, fileStart, SEEK_SET);

        unsigned int packetSize = 0;
        unsigned char* controlPacket = mountControlPacket(1, (long int) (fileEnd - fileStart), filename, &packetSize);
        
        if (llwrite(controlPacket, packetSize) < 0)
        {
            printf("Exit: error in START Control Packet\n");
            exit(-1);
        }
        

        unsigned char buffer[MAX_PAYLOAD_SIZE]; // Set the max amount to send on each frame
        int AccRead; // Accumulator of read bytes
        
        while ( ( AccRead = fread ( buffer , 1 , MAX_PAYLOAD_SIZE , file ) ) > 0 ) //Loop to send the file
        {
            int BytesWritten = 0;
            BytesWritten = llwrite ( buffer , AccRead ); //Sends a part of the file
            frameCounter++;
            

            if ( BytesWritten < 0 ) // Case of an error ocurring when sending the file
            {
                printf ( "\n Error sending the data frame. \n" );
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

        unsigned char packet[MAX_PAYLOAD_SIZE]; // Variable so the packet comes with the same size as the one sent
        int packetSize = 0;

        while (true)
        {
            packetSize = llread(packet);
            if (packetSize > 0)
                break;
        }

        unsigned long rcFileSize = 0;
        char* receiverFilename = readControlPacket(packet,&rcFileSize);


        FILE* file = fopen ( filename , "wb" ); // Open the file to send the data received

        if ( file == NULL ) // If wasn't possible to open the file
        { 
            printf ( "\n Could not open file to write the information. \n" );
            llclose ( TRUE );
            exit ( -1 );
        }

        int AccRead; //Accumalator with the bytes readed
        
        while ( TRUE )
        {
            AccRead = llread(packet);
            if (AccRead > 0)
            {
                int Write_file = fwrite ( packet , 1 , AccRead , file ); // Writes the information received on the file

                if ( Write_file < 0 ) // Produces a message in case of an error
                {
                    printf ( " \n Error writing the chunk in the file. \n " );
                    fclose ( file );
                    llclose ( TRUE );
                    exit ( -1 );
                }
            }
            else if (AccRead == 0)
            {
                break;
            }
            else
            {
                printf ( " \n Error on receiving the chunk. \n " );
                continue;
            }
        }

        printf("Finished copying file %s\n", receiverFilename);
        fclose ( file ); // Close the file once all the information has been written
    }    
}

unsigned char* mountControlPacket(unsigned char C, long int fileSize,const char* filename, unsigned int* packetSize)
{
    unsigned char fileSizeL = (int) ceil(log2f((float)fileSize)/8.0);
    unsigned char filenameL = strlen(filename);
    *packetSize = 5 + filenameL + fileSizeL;
    unsigned char* packet = (unsigned char*) malloc(*packetSize);
    packet[0] = C;
    packet[1] = 0;
    packet[2] = fileSizeL;

    memcpy(packet + 3,&fileSize,fileSizeL);
    int packetPos = 3 + fileSizeL;
    packet[packetPos++] = 1;
    packet[packetPos++] = filenameL;
    memcpy(packet + packetPos, filename, filenameL);
    return packet;
}

char* readControlPacket(unsigned char* packet, unsigned long int *fileSize)
{
    if (packet[1] != 0)
    {
        printf("Expected filesize in control packet but didn't find it\n");
    }
    unsigned char fileSizeBytes = packet[2];
    *fileSize = packet[3];

    for(unsigned int i = 1; i < fileSizeBytes; i++)
    {
        *fileSize <<= 8;
        *fileSize |= packet[3+i];
    }
    const unsigned int filenameInfo = 3+fileSizeBytes;
    if (packet[filenameInfo] != 1)
    {
        printf("Expected filename in control packet but didn't find it\n");
    }
    unsigned char fileNameBytes = packet[filenameInfo+1];
    char *filename = (char*)malloc(fileNameBytes);
    memcpy(filename, packet + filenameInfo + 2, fileNameBytes);
    
    return filename;
}
