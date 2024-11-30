// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>



// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256
#define UA_SIZE 5


volatile int STOP = FALSE;


// ALARM
int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}


int main(int argc, char *argv[])
{
    
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 30; 
    newtio.c_cc[VMIN] = 0; 

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
    
    //Ready response
    unsigned char UA[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
    int status = 0;
    STOP = FALSE;

    (void)signal(SIGALRM, alarmHandler); //Allow alarm handler
    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};  //TODO try other positions in buf[]
    unsigned char buffRes[6] = {0};

    unsigned int flag = 0x7E;
    unsigned int A    = 0x03;
    unsigned int C    = 0x03;
    unsigned int BCC  = A ^ C;
    buf[21] = flag;
    buf[22] = A;
    buf[23] = C;
    buf[24] = BCC;
    buf[25] = flag;

    int writeBytes = 0, readBytes = 0;

    while ((alarmCount < 4) && (STOP == FALSE))
    {
        writeBytes = write(fd, buf, BUF_SIZE);
        if (alarmEnabled == FALSE)
        {
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }
        while (STOP == FALSE)
        {
            int bytes = read(fd, UA, 1);
            if (alarmEnabled == FALSE)
            {
                break;
            }
            switch(UA[0])
            {
                default:
                    status = 0;
                    break;
                case 0x7E:
                    if (status == 0)
                    {
                        status = 1;
                        buffRes[0] = 0x7E;
                    }
                    else if (status == 4) 
                    {
                        buffRes[4] = 0x7E;
                        buffRes[5] = '\0';  // Set end of string to '\0', so we can printf
                        STOP = TRUE;
                    }   
                    break;
                case 0x01:
                    if (status == 1)
                    {
                        status = 2;
                        buffRes[1] = 0x01;
                        break;
                    }
                    status = 0;
                    break;
                case 0x07:
                    if (status == 2) 
                    {
                        status = 3;
                        buffRes[2] = 0x07;
                        break;
                    }
                    status = 0;
                    break;
                case 0x01^0x07:
                    if (status == 3) 
                    {
                        status = 4;
                        buffRes[3] = 0x01^0x07;
                        break;
                    }
                    status = 0;
                    break;
            }
        }
    }
    if (alarmCount >= 4)
    {
        printf("UA timeout, sorry :(\n");
    }else{

        alarmCount = 0;
        alarm(0);
        printf("%d bytes written\n", writeBytes);
        printf("UA bytes\n");
        for (int i = 0; i < UA_SIZE; i++)
        {
            printf("buf = 0x%02X\n", buffRes[i]);
        }
    }


    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
