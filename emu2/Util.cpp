#include <ctime>
#include <iostream>

#include "Util.h"

uint32_t cyIDFromString(const std::string &str)
{
    const uint8_t cyIDCharToIndex[]
    {
        0xFF, // 0
        0xFF, // 1
          24, // 2
          25, // 3
          26, // 4
          27, // 5
          28, // 6
          29, // 7
          30, // 8
          31, // 9
        0xFF, // :
        0xFF, // ;
        0xFF, // <
        0xFF, // =
        0xFF, // >
        0xFF, // ?
        0xFF, // @
           0, // A
           1, // B
           2, // C
           3, // D
           4, // E
           5, // F
           6, // G
           7, // H
        0xFF, // I
          23, // J
           8, // K
           9, // L
          10, // M
          11, // N
        0xFF, // O
          12, // P
          22, // Q
          13, // R
          14, // S
          15, // T
          16, // U
          17, // V
          18, // W
          19, // X
          20, // Y
          21, // Z
    };

    uint32_t cyID = 0;

    // 5 bits per char
    for(int i = 0; i < 6; i++)
    {
        int c = str[i] - '0';
        if(c < 0 || c >= int(sizeof(cyIDCharToIndex)) || cyIDCharToIndex[c] == 0xFF)
        {
            std::cerr << "Invalid character \"" << str[i] << "\" in CyID!\n";
            c = 'A' - '0';
        }

        cyID |= cyIDCharToIndex[c] << (26 - i * 5);
    }

    // only one bit for final char
    if(str[6] == 'B')
        cyID |= 1;
    else if(str[6] != 'A')
        std::cerr << "Invalid final character \"" << str[6] << "\" in CyID!\n";

    // this seems to be a second bit for the final char, don't know if it has special meaning though
    // cyID |= 1 << 31;

    return cyID;
}

std::string cyIDToString(uint32_t id)
{
    std::string ret(7, 'A');

    ret[6] = id & 1 ? 'B' : 'A';

    // may or may not be special
    // CyOS does display it though
    if(id >> 31)
        ret[6] += 2;

    uint32_t shifted = id >> 1;
    const char *idChars = "ABCDEFGHKLMNPRSTUVWXYZQJ23456789";
    for(int i = 5; i >= 0; i--)
    {
        ret[i] = idChars[shifted & 0x1F];
        shifted >>= 5;
    }

    return ret;
}

uint32_t cybikoTime()
{
    auto t = time(nullptr);

    auto tm = localtime(&t);

    // then convert back to a timestamp, but in "cybiko time"
    // nobody will be using these on February 6th 2036, right?

    // start with years (with not-so-accurate leap year handling)
    int days = (tm->tm_year / 4) * 1461; /* four years */

    for(int i = 0; i < tm->tm_year % 4; i++)
    {
        if(i == 0)
            days += 366;
        else
            days += 365;
    }

    // months
    for(int i = 0; i < tm->tm_mon; i++)
    {
        switch(i + 1)
        {
            case 1:
            case 3:
            case 5:
            case 7:
            case 8:
            case 10:
            case 12:
                days += 31;
                break;
            case 2:
                // still deliberately wrong
                if(tm->tm_year % 4 == 0)
                    days += 29;
                else
                    days += 28;
                break;

            case 4:
            case 6:
            case 9:
            case 11:
                days += 30;
                break;
        }
    }

    // and finally the day
    days += tm->tm_mday - 1;

    return ((days * 24 + tm->tm_hour) * 60 + tm->tm_min) * 60 + tm->tm_sec;
}