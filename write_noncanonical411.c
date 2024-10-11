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
    STOP = FALSE;

    (void)signal(SIGALRM, alarmHandler); //Allow alarm handler
    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    unsigned int flag = 0x7E;
    unsigned int A    = 0x04;
    unsigned int C    = 0x03;
    unsigned int BCC  = A ^ C;
    buf[0] = flag;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC;
    buf[4] = flag;

    int writeBytes = 0, readBytes = 0;

    while (alarmCount < 4)
    {
        writeBytes = write(fd, buf, BUF_SIZE);
        sleep(1);
        if (alarmEnabled == FALSE)
        {
            alarm(1); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
            readBytes = read(fd, UA, BUF_SIZE);
        }
        if (readBytes == 5)
            break;
    }
    if (alarmCount >= 4)
    {
        printf("UA timeout, sorry :(\n");
    }else{
        alarm(0);
        printf("%d bytes written\n", writeBytes);
        printf("UA bytes\n");

        // Loop for UA
        while (STOP == FALSE)
        {
            UA[readBytes] = '\0'; // Set end of string to '\0', so we can printf

            if (UA[0] == flag)
                STOP = TRUE;

            for (int i = 0; i < readBytes; i++)
            {
                printf("buf = 0x%02X\n", UA[i]);
            }
        }
        // Wait until all bytes have been written to the serial port
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
