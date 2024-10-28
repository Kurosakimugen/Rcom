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
            exit ( -1 );
        }

        unsigned char buffer[MAX_PAYLOAD_SIZE]; // Set the max amount to send on each frame
        int AccRead; // Accumulator of read bytes

        int Start_of_Transmission = llwrite ( STARTBuff , STARTBuff_Size ); //Send the start control package to signal the start of transmission

        if ( Start_of_Transmission < 0)
        {
            printf ( "\n Failed to send the start control package. \n" );
            fclose ( file );
            exit ( -1 );
        }
        

        while ( ( AccRead = fread ( buffer , 1 , MAX_PAYLOAD_SIZE , file ) ) > 0 ) //Loop to send the file
        {
            int Signal_To_Continue = 0; // Variable to control the acknolodgment signal
            while ( !Signal_To_Continue ) //Only stops when was correctly sent
            {
                int BytesWritten = llwrite ( buffer , AccRead ); //Sends a part of the file

                if ( BytesWritten < 0 ) // Case of an error ocurring when sending the file
                {
                    printf ( "\n Error sending the dataframe. \n" );
                    fclose ( file );
                    exit ( -1 );
                }
            }
            
            unsigned char* Answer_Signal; // Package to where store the answer signal
            int ASBytes = llread ( Answer_Signal ); //Reads the signal of the receiver part
            
            if ( ASBytes > 0 && /* Something to check if was properly received */ ) 
            { //Case when information was well received
                Signal_To_Continue = 1;
            } else // Case when information was not received
            {
                printf ( "\n Acknowledgment not received, retrying. \n");
                fclose ( file );
                exit ( -1 );
            }
        }

        int End_of_Transmission = llwrite ( ENDBuff , ENDBuff_Size ); //Send the end control package to signal the end of transmission

        if ( End_of_Transmission < 0)
        {
            printf ( "\n Failed to send the end control package. \n" );
        }

        fclose(file); // Close the file once the loop is over and was well sent
        llclose(TRUE); // Close connection after sending the file
    }
    else if ( linkLayer.role == LlRx ) // Section related to the Receiver
    {
        FILE* file = fopen ( filename , "wb" ); // Open the file to send the data received

        if ( file == NULL ) // If wasn't possible to open the file
        { 
            printf ( "\n Could not open file to write the information. \n" );
            exit ( -1 );
        }

        unsigned char packet[MAX_PAYLOAD_SIZE]; // Variable so the packet comes with the same size as the one sent
        int AccRead; //Accumalator with the bytes readed
        int Receiving_Flag = 1; //Flag to control if it is supposed to still receive inforamtion
        
        while ( Receiving_Flag ) // Loop to receive the file and sending acknolgdement
        {
            AccRead = llread ( packet ); //Receives a file and try to read

            if ( AccRead < 0) // Case when an error occurs receiving the bit of infromation
            {
                printf ( " \n Error on receiving the chunk. \n " );
                fclose ( file );
                exit (-1);
            }

            if ( AccRead == 1 && /* Check if is the start package */) // Case when is the start package
            {
                printf ( " \n Start of transmission of the file. \n " );
            }
            
            else if ( AccRead == 1 && /* Check if is the end package */) // Case when is the end package
            {
                printf ( " \n End of transmission of the file. \n " );
            }
            
            else 
            {
                if ( fwrite ( packet , 1 , AccRead , file ) ) //Writes the information received on the file
                {
                    printf ( " \n Error writing the chunk in the file. \n " );
                    fclose ( file );
                    exit ( -1 );
                }

                unsigned char Answer_Type;
                if ( llwrite ( buf , bufSize ) < 0 ) //Sends answer based on what received
                {
                    printf ( " \n Error sending the answer. \n " );
                    fclose ( file );
                    exit ( -1 );
                }
            }
        }
        
        fclose(file); // Close the file once all the information has been written
        llclose(TRUE); // Close connection when the file is over receiveing
    }
}