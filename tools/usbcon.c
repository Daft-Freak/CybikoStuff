#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb.h>

#include "encoding.h"

#define min(a, b) ((a < b) ? a : b)

#define CYBIKO_VID 0x0b66
#define CYBIKO_BOOTLOADER_PID 0x0040
#define CYBIKO_CYOS_PID 0x0041


// crc calc
static uint32_t crc32Table[256] = {0};
static uint16_t crc16Table[256] = {0};

static void initCRCTables()
{
    for(int i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for(int j = 0; j < 8; j++)
        {
            bool bit = crc & 1;
            crc >>= 1;

            if(bit)
                crc ^= 0xEDB88320;
        }

        crc32Table[i] = crc;
    }

    for(int i = 0; i < 256; i++)
    {
        uint16_t crc = i;
        for(int j = 0; j < 8; j++)
        {
            bool bit = crc & 1;
            crc >>= 1;

            if(bit)
                crc ^= 0xA001;
        }

        crc16Table[i] = crc;
    }
}

static uint32_t crc32(uint8_t *data, int length)
{
    uint32_t crc = 0xFFFFFFFF;

    for(int i = 0; i < length; i++)
    {
        uint8_t v = (crc & 0xFF) ^ data[i];
        crc = (crc >> 8) ^ crc32Table[v];
    }

    return crc;
}

static uint32_t crc16(uint8_t *data, int length)
{
    uint16_t crc = 0xFFFF;

    for(int i = 0; i < length; i++)
    {
        uint8_t v = (crc & 0xFF) ^ data[i];
        crc = (crc >> 8) ^ crc16Table[v];
    }

    return crc;
}

// usb helpers
static bool getDeviceId(libusb_device *device, libusb_device_handle *handle, unsigned char *data, int length)
{
    // cyid and serial number are stored in the configuration descriptor string
    struct libusb_config_descriptor *config;
    int err = libusb_get_config_descriptor(device, 0, &config);

    if(err != 0)
        return false;

    if(config->iConfiguration == 0)
    {
        libusb_free_config_descriptor(config);
        return false;
    }

    // open the device to read the string
    bool openedHandle = false;
    if(!handle)
    {
        err = libusb_open(device, &handle);
        if(err != 0)
        {
            libusb_free_config_descriptor(config);
            return false;
        }
        openedHandle = true;
    }

    err = libusb_get_string_descriptor_ascii(handle, config->iConfiguration, data, length);

    if(openedHandle)
        libusb_close(handle);

    libusb_free_config_descriptor(config);
    return err >= 0;
}

static libusb_device_handle *findDevice(int pid)
{
    libusb_device **list;
    ssize_t numDevices = libusb_get_device_list(NULL, &list);

    struct libusb_device_descriptor devDesc;

    if(numDevices < 0)
    {
        fprintf(stderr, "Failed to list USB devices(%zi)\n", numDevices);
        return NULL;
    }

    libusb_device **matching = malloc(sizeof(libusb_device *) * numDevices);
    int numMatching = 0;

    for(ssize_t i = 0; i < numDevices; i++)
    {
        int err = libusb_get_device_descriptor(list[i], &devDesc);
        if(err != 0)
            continue;

        if(devDesc.idVendor != CYBIKO_VID)
            continue;

        if(devDesc.idProduct != pid)
            continue;

        matching[numMatching++] = list[i];
    }

    libusb_device *chosen = NULL;
    libusb_device_handle *chosenHandle = NULL;

    // only one found, use it
    if(numMatching == 1)
        chosen = matching[0];
    else if(numMatching > 1)
    {
        unsigned char idStr[23] = {0};

        printf("Multiple devices found: \n");
        for(int i = 0; i < numMatching; i++)
        {
            if(pid == CYBIKO_CYOS_PID && getDeviceId(matching[i], NULL, idStr, 23))
                printf("\t %i %s\n", i, idStr);
            else
                printf("\t %i UNKNOWN\n", i);
        }

        int index = -1;

        while(index < 0 || index >= numMatching)
        {
            printf("\nEnter device number: ");
            scanf("%i", &index);
        }

        chosen = matching[index];
    }

    // attempt to open
    if(chosen)
    {
        int err = libusb_open(chosen, &chosenHandle);
        if(err != 0)
            fprintf(stderr, "Failed to open device(%i)\n", err);
    }

    libusb_free_device_list(list, 1);
    return chosenHandle;
}

// cyos helpers
typedef struct CyOSPacket
{
    uint8_t a; // 0x11 = data, 0x12 = control?
    uint8_t b; // channel?
    uint8_t c;
    uint8_t *data;
    uint16_t length;
} CyOSPacket;

typedef struct CyOSCmdState
{
    uint8_t sendNo;
    uint8_t recvNo;
} CyOSCmdState;

static int writeCyOSPacket(libusb_device_handle *dev, const CyOSPacket *packet)
{
    uint16_t length = packet->length;
    unsigned char *buf = malloc(length + 11);

    // header
    buf[0] = 0x4D;
    buf[1] = 0x4D;
    buf[2] = packet->a;
    buf[3] = packet->b;
    buf[4] = packet->c;
    buf[5] = length >> 8;
    buf[6] = length & 0xFF;

    memcpy(buf + 7, packet->data, length);

    // checksum
    uint32_t crc = ~crc32(buf, length + 7);
    buf[length + 7] = crc >> 24;
    buf[length + 8] = (crc >> 16) & 0xFF;
    buf[length + 9] = (crc >> 8) & 0xFF;
    buf[length + 10] = crc & 0xFF;

    int ret = libusb_bulk_transfer(dev, 0x02, buf, length + 11, NULL, 0);

    if(ret != 0)
       fprintf(stderr, "Error sending packet (%i)\n", ret);

    free(buf);

    return ret;
}

static int readCyOSPacket(libusb_device_handle *dev, CyOSPacket *packet, int maxLen)
{
    unsigned char *buf = malloc(maxLen + 11);

    int transferred = 0;
    int ret = libusb_bulk_transfer(dev, 0x81, buf, maxLen + 11, &transferred, 0);

    if(ret != 0)
    {
       fprintf(stderr, "Error receiving packet (%i)\n", ret);
       free(buf);
       return ret;
    }

    bool valid = true;

    // check length and header
    if(transferred < 11 || buf[0] != 0x4D || buf[1] != 0x4D)
    {
        valid = false;
        fprintf(stderr, "Received invalid packet (%i, %x, %x)\n", transferred, buf[0], buf[1]);
    }

    // check crc
    if(valid)
    {
        uint32_t exCRC = ~crc32(buf, transferred - 4);
        uint32_t crc = (buf[transferred - 4] << 24) | (buf[transferred - 3] << 16) | (buf[transferred - 2] << 8) | (buf[transferred - 1]);

        if(crc != exCRC)
        {
            valid = false;
            fprintf(stderr, "Received invalid packet CRC (%u != %u)\n", crc, exCRC);
        }
    }

    if(valid)
    {
        packet->a = buf[2];
        packet->b = buf[3];
        packet->c = buf[4];
        packet->length = (buf[5] << 8) | buf[6];
        memcpy(packet->data, buf + 7, packet->length);
    }

    free(buf);

    if(!valid)
        return LIBUSB_ERROR_OTHER;

    return ret;
}

static bool sendCyOSCommand(libusb_device_handle *dev, CyOSCmdState *state, const char *cmd, int len)
{
    // setup to read output
    CyOSPacket pkt;
    uint8_t pktData[] = {2, state->recvNo, 0, 0};
    pkt.a = 0x12;
    pkt.b = 2;
    pkt.c = 0;
    pkt.data = pktData;
    pkt.length = 4;
    writeCyOSPacket(dev, &pkt);
    state->recvNo++;

    // prepare the command packet
    pkt.a = 0x11;
    pkt.b = 1;
    pkt.c = state->sendNo;
    pkt.data = (uint8_t *)cmd;
    pkt.length = len + 1;

    CyOSPacket readPkt;
    uint8_t readData[1024];
    readPkt.data = readData;

    // send the command until we get a response
    int err;
    while(true)
    {
        err = writeCyOSPacket(dev, &pkt);
        if(err != 0)
            break;

        err = readCyOSPacket(dev, &readPkt, 1024);
        if(err != 0)
            break;

        // start of output
        if(readPkt.a == 0x11)
        {
            printf("%s", readPkt.data);
            break;
        }
    }

    if(err != 0)
    {
        fprintf(stderr, "Failed to send command(%i)\n", err);
        return false;
    }

    state->sendNo++;

    // continue reading output
    pkt.a = 0x12;
    pkt.b = 2;
    pkt.c = 0;
    pkt.data = pktData;
    pkt.length = 4;
    while(true)
    {
        pktData[1] = state->recvNo;
        err = writeCyOSPacket(dev, &pkt);
        if(err != 0)
            break;

        err = readCyOSPacket(dev, &readPkt, 1024);
        if(err != 0)
            break;

        // wait for correct item
        if(readPkt.a == 0x11 && readPkt.c != state->recvNo)
            continue;

        fprintf(stderr, "recv %x %x %x len %x\n", readPkt.a, readPkt.b, readPkt.c, readPkt.length); //!

        state->recvNo++;

        // no more data
        // FIXME: this just means not ready, need to do this in the background...
        if(readPkt.a != 0x11)
            break;

        printf("%s", readPkt.data);
    }

    return err == 0;
}

static bool initCyOSConsole(libusb_device_handle *dev)
{
    unsigned char idStr[23] = {0};
    getDeviceId(libusb_get_device(dev), dev, idStr, 23);
    printf("Opened %s\n", idStr);

    // make sure device is configured
    int cfg = 0;
    int err = libusb_get_configuration(dev, &cfg);
    if(cfg != 1)
        err = libusb_set_configuration(dev, 1);

    if(err != 0)
    {
        fprintf(stderr, "Failed to configure device(%i)\n", err);
        return false;
    }

    err = libusb_claim_interface(dev, 0);

    if(err != 0)
    {
        fprintf(stderr, "Failed to claim interface(%i)\n", err);
        return false;
    }

    // init
    uint8_t initData[] = {4, 0, 0, 0};
    CyOSPacket pkt;
    pkt.a = 0x12;
    pkt.b = 2;
    pkt.c = 0;
    pkt.data = initData;
    pkt.length = 4;
    err = writeCyOSPacket(dev, &pkt);
    if(err != 0)
        return false;

    pkt.b = 1;
    writeCyOSPacket(dev, &pkt);
    if(err != 0)
        return false;

    return true;
}

static void usbConsole(libusb_device_handle *dev)
{
    if(!initCyOSConsole(dev))
        return;

    // execute commands
    CyOSCmdState state = {0};
    char *line = NULL;
    size_t size = 0;

    while(true)
    {
        printf("> ");

        int len = getline(&line, &size, stdin);
        if(len != -1)
        {
            line[len - 1] = 0; // remove newline
            if(!sendCyOSCommand(dev, &state, line, len - 1))
                break;
        }
    }

    free(line);
}

static bool sendSingleCyOSCommand(libusb_device_handle *dev, const char *cmd, int len)
{
    if(!initCyOSConsole(dev))
        return false;

    // execute command
    CyOSCmdState state = {0};
    return sendCyOSCommand(dev, &state, cmd, len);
}

// requires that device is booted to os
// FIXME: error checking
static void sendFile(libusb_device_handle *dev, const char *localName, const char *deviceName)
{
    FILE *f = fopen(localName, "rb");
    if(!f)
        return;

    fseek(f, 0, SEEK_END);
    int fileLen = ftell(f);
    fseek(f, 0, SEEK_SET);

    char cmd[1024];
    snprintf(cmd, 1024, "rcv $data_pipe=100 \"%s\" %i", deviceName, fileLen);

    initCyOSConsole(dev);

    CyOSCmdState state = {0};
    sendCyOSCommand(dev, &state, cmd, strlen(cmd));
    printf("\n");

    bool done = false;

    while(true)
    {
        CyOSPacket readPkt;
        uint8_t readData[1024];
        readPkt.data = readData;

        readCyOSPacket(dev, &readPkt, 1024);

        if(readPkt.a == 0x12 && readPkt.b == 199 /* data_pipe * 2 - 1? */)
        {
            const int chunkSize = 512;
            char buf[chunkSize];

            int offset = readPkt.data[1] * chunkSize;

            if(done && !offset)
            {
                // another offset 0 is sent at the end
                printf("Done!\n");

                /*
                    host -> 1.67.2
                    12 c7 00 len 0004 csum 2d141b3f data 05 00 00 00
                    ??
                */
            }
            else if(offset < fileLen)
            {
                printf("sending file %i/%i...\r", offset, fileLen);
                fseek(f, SEEK_SET, offset);
                int len = fread(buf, 1, chunkSize, f);

                CyOSPacket pkt;
                pkt.a = 0x11;
                pkt.b = readPkt.b;
                pkt.c = readPkt.data[1];

                pkt.length = len;
                pkt.data = (uint8_t *)buf;
                writeCyOSPacket(dev, &pkt);
            }
            else
            {
                // past end - done
                printf("\n");

                // this is sent at the end...
                uint8_t endData[] = {5, 0, 0, 0};
                CyOSPacket pkt;
                pkt.a = 0x12;
                pkt.b = readPkt.b + 1;
                pkt.c = 0;
                pkt.length = 4;
                pkt.data = endData;
                writeCyOSPacket(dev, &pkt);
                done = true;
            }
        }
        else if(readPkt.a == 0x11 && readPkt.b == 2)
        {
            // probably success message / ready
            printf("%s", readPkt.data);

            if(strcmp((char *)readPkt.data, "\nReady >\n ") == 0)
                break;

            // attempt to get next msg
            uint8_t pktData[] = {2, /*state->recvNo*/readPkt.c + 1, 0, 0};
            CyOSPacket pkt;
            pkt.a = 0x12;
            pkt.b = 2;
            pkt.c = 0;
            pkt.data = pktData;
            pkt.length = 4;

            writeCyOSPacket(dev, &pkt);
        }
        else
            fprintf(stderr, "send file recv %x %x %x len %x\n", readPkt.a, readPkt.b, readPkt.c, readPkt.length); //!
    }

    fclose(f);
}

static void usage()
{
    printf("usage: usbcon [-b file]\n");
    printf("       usbcon -s file [-d name]\n\n");
    printf("\t-b: boot file\n");
    printf("\t-s: send file\n");
    printf("\t-d: dest name for file (defaults to source name)\n");
}

static libusb_device_handle *findBootloaderDevice()
{
    libusb_device_handle *dev = findDevice(CYBIKO_BOOTLOADER_PID);
    if(!dev)
    {
        printf("No bootloader devices found, looking for booted devices...\n");
        dev = findDevice(CYBIKO_CYOS_PID);

        if(dev)
        {
            sendSingleCyOSCommand(dev, "reboot", 6); // this will fail (device disconnects while rebooting)
            libusb_close(dev);

            printf("Waiting for device to reboot...\n");
        }
        else
            printf("None found, waiting for a device to appear...\n");

        for(int i = 0; i < 10; i++)
        {
            dev = findDevice(CYBIKO_BOOTLOADER_PID);
            if(dev)
                break;

            sleep(1);
        }
    }
    return dev;
}

static void bootDevice(libusb_device_handle *dev, const char *filename)
{
    // make sure device is configured
    int cfg = 0;
    int err = libusb_get_configuration(dev, &cfg);
    if(cfg != 1)
        err = libusb_set_configuration(dev, 1);

    if(err != 0)
    {
        fprintf(stderr, "Failed to configure device(%i)\n", err);
        return;
    }

    err = libusb_claim_interface(dev, 0);

    if(err != 0)
    {
        fprintf(stderr, "Failed to claim interface(%i)\n", err);
        return;
    }

    // tell the bootloader we're going to send it a file
    uint8_t data = 0x1E;
    err = libusb_bulk_transfer(dev, 0x02, &data, 1, NULL, 0);
    if(err != 0)
        return;

    // wait for response
    int transferred = 0;
    uint8_t recvBuf[12];
    while(err == 0 && transferred == 0)
        err = libusb_bulk_transfer(dev, 0x81, recvBuf, 12, &transferred, 0);

    // TODO: use this response? (looks like it contains the load address)

    if(err != 0)
    {
        fprintf(stderr, "Failed to send boot command (%i)\n", err);
        return;
    }

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
    if(fileData[0] == 0x12 && fileData[1] == 0x34 && fileData[2] == 0xAB && fileData[3] == 0xCD)
    {
        // plain image, need to encode
        printf("Encoding image...\n");

        // create new buffer with room for the header
        uint8_t *newBuf = malloc(size + 12);
        // add the header
        newBuf[0] = 0xFF;
        newBuf[1] = 0xC0;
        newBuf[2] = 0xFF;
        newBuf[3] = 0xAB;
        newBuf[4] = newBuf[ 8] = size >> 24;
        newBuf[5] = newBuf[ 9] = size >> 16;
        newBuf[6] = newBuf[10] = size >> 8;
        newBuf[7] = newBuf[11] = size;

        encodeBoot(fileData, newBuf + 12, size);

        // swap to encoded image
        free(fileData);
        fileData = newBuf;
        size += 12;
    }
    else if(fileData[0] != 0xFF || fileData[1] != 0xC0 || fileData[2] != 0xFF || fileData[3] != 0xAB)
    {
        fprintf(stderr, "Invalid boot image header(%02X%02X%02X%02X), expected 1234ABCD or FFC0FFAB\n", fileData[0], fileData[1], fileData[2], fileData[3]);
        free(fileData);
        return;
    }

    uint32_t checksum = ~crc32(fileData, size);

    printf("Attempting to boot %s (size: %li, checksum: %08X)\n", filename, size, checksum);

    // unlike the os packets, this is little-endian
    uint8_t buf[13];
    buf[0] = 0x3C;
    buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x48; buf[4] = 0x00; // load address: 0x480000
    // size
    buf[5] = size & 0xFF;
    buf[6] = (size >> 8) & 0xFF;
    buf[7] = (size >> 16) & 0xFF;
    buf[8] = (size >> 24) & 0xFF;
    // checksum
    buf[9] = checksum & 0xFF;
    buf[10] = (checksum >> 8) & 0xFF;
    buf[11] = (checksum >> 16) & 0xFF;
    buf[12] = (checksum >> 24) & 0xFF;

    err = libusb_bulk_transfer(dev, 0x02, buf, 13, NULL, 0);
    if(err != 0)
    {
        fprintf(stderr, "Failed to send boot params(%i)\n", err);
        free(fileData);
        return;
    }

    const int chunkSize = 4096;
    uint8_t packetBuf[chunkSize + 7];
    packetBuf[0] = 0x5A;

    while(true)
    {
        transferred = 0;
        while(err == 0 && transferred == 0)
            err = libusb_bulk_transfer(dev, 0x81, recvBuf, 12, &transferred, 0);

        if(err != 0)
            break;

        if(transferred == 1 && recvBuf[0] == 0x69)
        {
            printf("Done!             \n");
            break; //done
        }

        int chunkIndex = recvBuf[1] | (recvBuf[2] << 8);
        int chunkOff = chunkIndex * chunkSize;
        int thisChunkSize = min(chunkSize, size - chunkOff);
        uint16_t chunkCRC = crc16(fileData + chunkOff, thisChunkSize);

        printf("Sending %i/%li\r", chunkOff, size);

        // copy over the index
        packetBuf[1] = recvBuf[1];
        packetBuf[2] = recvBuf[2];
        packetBuf[3] = (thisChunkSize + 2) & 0xFF;
        packetBuf[4] = (thisChunkSize + 2) >> 8;

        // copy the data
        memcpy(packetBuf + 5, fileData + chunkOff, thisChunkSize);

        packetBuf[thisChunkSize + 5] = chunkCRC & 0xFF;
        packetBuf[thisChunkSize + 6] = chunkCRC >> 8;

        err = libusb_bulk_transfer(dev, 0x02, packetBuf, thisChunkSize + 7, NULL, 0);

        if(err != 0)
        {
            fprintf(stderr, "Failed to send boot file chunk %i (%i)\n", chunkIndex, err);
            break;
        }
    }

    if(err != 0)
        fprintf(stderr, "Failed to send boot file (%i)\n", err);

    free(fileData);
}

int main(int argc, char *argv[])
{
    const char *bootFile = NULL, *sendFilename = NULL, *dstName = NULL;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-b") == 0 && i + 1 < argc)
            bootFile = argv[++i];
        else if(strcmp(argv[i], "-s") == 0 && i + 1 < argc)
            sendFilename = argv[++i];
        else if(strcmp(argv[i], "-d") == 0 && i + 1 < argc)
            dstName = argv[++i];
        else
        {
            usage();
            return 1;
        }
    }

    int err = libusb_init(NULL);
    if(err != 0)
    {
        fprintf(stderr, "Failed to init libusb(%i)\n", err);
        return 1;
    }

    initCRCTables();

    if(bootFile)
    {
        libusb_device_handle *dev = findBootloaderDevice();
        if(!dev)
        {
            fprintf(stderr, "No device found\n");
            return 1;
        }

        bootDevice(dev, bootFile);
        libusb_close(dev);
    }
    else if(sendFilename)
    {
        libusb_device_handle *dev = findDevice(CYBIKO_CYOS_PID);
        if(dev)
            sendFile(dev, sendFilename, dstName ? dstName : sendFilename);
    }
    else
    {
        // default to console
        libusb_device_handle *dev = findDevice(CYBIKO_CYOS_PID);

        if(dev)
        {
            usbConsole(dev);
            libusb_close(dev);
        }
    }

    libusb_exit(NULL);
    return 0;
}
