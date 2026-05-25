#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libserialport.h>

#include "crc.h"

static bool checkOSConsole(struct sp_port *port, unsigned timeout)
{
    // write empty command with zero checksum

    sp_blocking_write(port, &"\n\0\0", 3, 0);

    // result should be 2
    uint8_t b = 0;
    if(sp_blocking_read(port, &b, 1, timeout) != 1)
    {
        fprintf(stderr, "Console check timed out!\n");
        return false;
    }

    return b == 2;
}

static uint16_t serialChecksum(uint8_t *data, int length)
{
    uint32_t ret = 0;

    for(int i = 0; i < length; i++)
    {
        ret = ret ^ data[i];
        ret = ret << 1 | ((ret >> 16) & 1);
    }

    return ret;
}

static int serialReadLine(struct sp_port *port, char *buf, int maxLen)
{
    int offset = 0;

    while(true)
    {
        if(sp_blocking_read(port, buf + offset, 1, 10000) != 1)
            return offset ? offset : -1;

        char c = buf[offset++];

        if(c == '\n' || offset == maxLen)
            return offset;
    }
}

static bool waitOSConsoleReady(struct sp_port *port, bool output)
{
    // read data until we get the ready message
    const char *readyStr = "Ready >\r\n";

    char readBuf[1024];
    while(true)
    {
        int len = serialReadLine(port, readBuf, sizeof(readBuf));

        if(len > 0)
        {
            if(output)
                fwrite(readBuf, 1, len, stdout);

            if(memcmp(readBuf, readyStr, strlen(readyStr)) == 0)
            {
                // there's an extra space after the message
                sp_blocking_read(port, readBuf, 1, 10000);
                return true;
            }
        }
        else if(len < 0)
            return false;
    }
}

// `cmd` includes newline
static bool sendCyOSCommand(struct sp_port *port, const char *cmd, int len)
{
    // checksum doesn't include newline
    uint16_t checksum = serialChecksum((uint8_t *)cmd, len - 1);

    sp_blocking_write(port, cmd, len, 0);

    char buf[2];
    buf[0] = checksum & 0xFF;
    buf[1] = checksum >> 8;
    sp_blocking_write(port, buf, 2, 0);

    // read response
    sp_blocking_read(port, buf, 1, 1000);

    if(buf[0] != 2)
    {
        fprintf(stderr, "Failed to send command! (%i)\n", buf[0]);
        return false;
    }

    return true;
}

static void serialConsole(struct sp_port *port)
{
    char *line = NULL;
    size_t size = 0;

    if(!checkOSConsole(port, 10000))
    {
        fprintf(stderr, "OS console does not appear to be running.\n");
        return;
    }

    // above empty command will result in some output
    waitOSConsoleReady(port, false);

    while(true)
    {
        // get command
        printf("> ");

        int len = getline(&line, &size, stdin);
        if(len != -1)
        {
            if(!sendCyOSCommand(port, line, len))
                break;

            if(!waitOSConsoleReady(port, true))
                break;
        }
    }

    free(line);
}

static void usage()
{
    printf("usage: sercon [-b file]\n");
    printf("       sercon -s file [-d name]\n\n");
    printf("\t-b: boot file\n");
    printf("\t-s: send file\n");
    printf("\t-d: dest name for file (defaults to source name)\n");
}

static void bootDevice(struct sp_port *port, const char *filename)
{   
    // open file
    FILE *f = fopen(filename, "rb");
    if(!f)
    {
        fprintf(stderr, "Failed to open boot image file\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // read boot file
    uint8_t *fileData = malloc(size);
    fread(fileData, 1, size, f);
    fclose(f);

    // check header
    // dont need to encode, a plain file can be booted
    if(fileData[0] != 0x12 || fileData[1] != 0x34 || fileData[2] != 0xAB || fileData[3] != 0xCD)
    {
        fprintf(stderr, "Invalid boot image header(%02X%02X%02X%02X), expected 1234ABCD\n", fileData[0], fileData[1], fileData[2], fileData[3]);
        free(fileData);
        return;
    }

    uint32_t checksum = ~crc32(fileData, size);

    printf("Attempting to boot %s (size: %li, checksum: %08X)\n", filename, size, checksum);
    
    // wait for prepare message
    const char *loadMessage = "Preparing to load CyOS\r\n";

    char buf[1024];
    while(true)
    {
        int len = serialReadLine(port, buf, sizeof(buf));

        if(len > 0)
        {
            if(memcmp(buf, loadMessage, strlen(loadMessage)) == 0)
                break;
        }
        else if(len < 0)
        {
            fprintf(stdout, "Failed waiting for load message!\n");
            return;
        }
    }

    // send initial command
    int len = snprintf(buf, sizeof(buf), "rcv file.boot %li\n", size);

    if(sp_blocking_write(port, buf, len, 0) < 0)
    {
        fprintf(stderr, "Failed to send boot command\n");
        free(fileData);
        return;
    }

    // ignored checksum
    buf[0] = buf[1] = 0;
    sp_blocking_write(port, buf, 2, 0);

    // read lines and wait for "send"
    while(true)
    {
        int len = serialReadLine(port, buf, sizeof(buf));

        if(len > 0)
        {
            if(memcmp(buf, "send", 4) == 0)
            {
                // send a chunk

                int chunkIndex = atoi(buf + 4);
                const int maxChunkSize = 512;

                int chunkSize = maxChunkSize;
                int chunkOff = chunkIndex * maxChunkSize;

                // check the chunk is in bounds
                if(chunkIndex < 0 || chunkOff >= size)
                {
                    fprintf(stdout, "Invalid chunk index (%i)!\n", chunkIndex);
                    break;
                }

                // clamp last chunk
                if((chunkIndex + 1) * maxChunkSize > size)
                    chunkSize = size - chunkOff;

                printf("Sending %i/%li\r", chunkOff, size);

                buf[0] = 'C';
                buf[1] = chunkIndex & 0xFF;
                buf[2] = chunkIndex >> 8;
                buf[3] = chunkSize & 0xFF;
                buf[4] = chunkSize >> 8;

                memcpy(buf + 5, fileData + chunkOff, chunkSize);

                uint32_t crc = ~crc32((uint8_t *)buf + 1, chunkSize + 4);

                buf[chunkSize + 5] = (crc >>  0) & 0xFF;
                buf[chunkSize + 6] = (crc >>  8) & 0xFF;
                buf[chunkSize + 7] = (crc >> 16) & 0xFF;
                buf[chunkSize + 8] = (crc >> 24) & 0xFF;

                if(sp_blocking_write(port, buf, chunkSize + 9, 0) < 0)
                    fprintf(stderr, "Failed to send boot file chunk %i\n", chunkIndex);
            }
            else if(memcmp(buf, "done", 4) == 0)
                break;
            else if(len > 2 || buf[0] != '\r')
            {
                printf("\nreceived (%i): %.*s", len, len, buf);
            }
        }
        else if(len < 0)
        {
            fprintf(stdout, "Failed reading boot output!\n");
            break;
        }
    }

    free(fileData);
}

struct sp_port *openPort(const char *portName, int baud)
{
    struct sp_port *port;
    enum sp_return result = sp_get_port_by_name(portName, &port);

    if(result != SP_OK)
    {
        fprintf(stderr, "Failed to get port!\n");
        return NULL;
    }

    result = sp_open(port, SP_MODE_READ_WRITE);
    if(result != SP_OK)
    {
        fprintf(stderr, "Failed to open port!\n");
        sp_free_port(port);
        return NULL;
    }

    // configure
    result = sp_set_baudrate(port, baud);
    if(result != SP_OK)
    {
        fprintf(stderr, "Failed to set baud rate!\n");
        sp_close(port);
        sp_free_port(port);
        return NULL;
    }

    return port;
}

int main(int argc, char *argv[])
{
    char *portName = NULL;
    const char *bootFile = NULL, *sendFilename = NULL, *dstName = NULL;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-b") == 0 && i + 1 < argc)
            bootFile = argv[++i];
        else if(strcmp(argv[i], "-s") == 0 && i + 1 < argc)
            sendFilename = argv[++i];
        else if(strcmp(argv[i], "-d") == 0 && i + 1 < argc)
            dstName = argv[++i];
        else if(strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            portName = strdup(argv[++i]);
        else
        {
            usage();
            return 1;
        }
    }

    // default to the first serial port
    if(!portName)
    {
        struct sp_port **portList;

        enum sp_return result = sp_list_ports(&portList);

        if(result == SP_OK)
        {
            for(int i = 0; portList[i]; i++)
            {
                portName = strdup(sp_get_port_name(portList[i]));
                break;
            }

            sp_free_port_list(portList);
        }
    }

    // open the port
    printf("Using port %s\n", portName);

    struct sp_port *port = openPort(portName, 57600);

    if(!port)
    {
        free(portName);
        return 1;
    }

    sp_flush(port, SP_BUF_BOTH);

    if(bootFile)
    {
        // if the OS is running, attempt to reboot
        if(checkOSConsole(port, 1000))
        {
            printf("Attempting reboot...\n");
            waitOSConsoleReady(port, false);
            sendCyOSCommand(port, "reboot\n", 7);
        }

        bootDevice(port, bootFile);
    }
    else if(sendFilename)
    {
        // TODO
    }
    else
    {
        // default to console
        serialConsole(port);
    }

    sp_close(port);
    sp_free_port(port);
    free(portName);

    return 0;
}
