#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"

void rotl32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32_1[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000002, 0x80},
        {0x40000000, 0x80000000, 0x88}, // N
        {0x80000000, 0x00000001, 0x81}, // C
        {0xC0000000, 0x80000001, 0x89}, // NC
    };

    static const TestValue32 tests32_2[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000004, 0x80},
        {0x20000000, 0x80000000, 0x88}, // N
        {0x40000000, 0x00000001, 0x81}, // C
        {0x60000000, 0x80000001, 0x89}, // NC
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("ROTL 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotl %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_1)
    {
        char buf[60];
        auto &test = tests32_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTL 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotl #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_2)
    {
        char buf[60];
        auto &test = tests32_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotl16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16_1[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0002, 0x80},
        {0x4000, 0x8000, 0x88}, // N
        {0x8000, 0x0001, 0x81}, // C
        {0xC000, 0x8001, 0x89}, // NC
    };

    static const TestValue16 tests16_2[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0004, 0x80},
        {0x2000, 0x8000, 0x88}, // N
        {0x4000, 0x0001, 0x81}, // C
        {0x6000, 0x8001, 0x89}, // NC
    };
    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("ROTL 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotl %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_1)
    {
        char buf[60];
        auto &test = tests16_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTL 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotl #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_2)
    {
        char buf[60];
        auto &test = tests16_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotl8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char cc;
    };

    static const TestValue8 tests8_1[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x02, 0x80},
        {0x40, 0x80, 0x88}, // N
        {0x80, 0x01, 0x81}, // C
        {0xC0, 0x81, 0x89}, // NC
    };

    static const TestValue8 tests8_2[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x04, 0x80},
        {0x20, 0x80, 0x88}, // N
        {0x40, 0x01, 0x81}, // C
        {0x60, 0x81, 0x89}, // NC
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("ROTL 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotl %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_1)
    {
        char buf[60];
        auto &test = tests8_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTL 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotl #2, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_2)
    {
        char buf[60];
        auto &test = tests8_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotr32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32_1[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x80000000, 0x89}, // NC
        {0x00000002, 0x00000001, 0x80},
    };

    static const TestValue32 tests32_2[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x40000000, 0x80},
        {0x00000002, 0x80000000, 0x89}, // NC
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("ROTR 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotr %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_1)
    {
        char buf[60];
        auto &test = tests32_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTR 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotr #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_2)
    {
        char buf[60];
        auto &test = tests32_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotr16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16_1[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x8000, 0x89}, // NC
        {0x0002, 0x0001, 0x80},
    };

    static const TestValue16 tests16_2[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x4000, 0x80},
        {0x0002, 0x8000, 0x89}, // NC
    };
    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("ROTR 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotr %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_1)
    {
        char buf[60];
        auto &test = tests16_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTR 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotr #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_2)
    {
        char buf[60];
        auto &test = tests16_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotr8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char cc;
    };

    static const TestValue8 tests8_1[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x80, 0x89}, // NC
        {0x02, 0x01, 0x80},
    };

    static const TestValue8 tests8_2[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x40, 0x80},
        {0x02, 0x80, 0x89}, // NC
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("ROTR 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotr %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_1)
    {
        char buf[60];
        auto &test = tests8_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTR 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "rotr #2, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_2)
    {
        char buf[60];
        auto &test = tests8_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotxl32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char inCC, cc;
    };

    static const TestValue32 tests32_1[]
    {
        // C = 0
        {0x00000000, 0x00000000, 0x80, 0x84}, // Z
        {0x00000001, 0x00000002, 0x80, 0x80},
        {0x40000000, 0x80000000, 0x80, 0x88}, // N
        {0x80000000, 0x00000000, 0x80, 0x85}, // ZC
        {0x80000001, 0x00000002, 0x80, 0x81}, // C
        {0xC0000000, 0x80000000, 0x80, 0x89}, // NC
        // C = 1
        {0x00000000, 0x00000001, 0x81, 0x80},
        {0x40000000, 0x80000001, 0x81, 0x88}, // N
        {0x80000000, 0x00000001, 0x81, 0x81}, // C
        {0xC0000000, 0x80000001, 0x81, 0x89}, // NC
    };

    static const TestValue32 tests32_2[]
    {
        // C = 0
        {0x00000000, 0x00000000, 0x80, 0x84}, // Z
        {0x00000001, 0x00000004, 0x80, 0x80},
        {0x20000000, 0x80000000, 0x80, 0x88}, // N
        {0x40000000, 0x00000000, 0x80, 0x85}, // ZC
        {0x40000001, 0x00000004, 0x80, 0x81}, // C
        {0x60000000, 0x80000000, 0x80, 0x89}, // NC
        {0x80000000, 0x00000001, 0x80, 0x80},

        // C = 1
        {0x00000000, 0x00000002, 0x81, 0x80},
        {0x20000000, 0x80000002, 0x81, 0x88}, // N
        {0x40000000, 0x00000002, 0x81, 0x81}, // C
        {0x60000000, 0x80000002, 0x81, 0x89}, // NC
        {0x80000000, 0x00000003, 0x81, 0x80},
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("ROTXL 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxl %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_1)
    {
        char buf[60];
        auto &test = tests32_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTXL 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxl #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_2)
    {
        char buf[60];
        auto &test = tests32_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotxl16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char inCC, cc;
    };

    static const TestValue16 tests16_1[]
    {
        // C = 0
        {0x0000, 0x0000, 0x80, 0x84}, // Z
        {0x0001, 0x0002, 0x80, 0x80},
        {0x4000, 0x8000, 0x80, 0x88}, // N
        {0x8000, 0x0000, 0x80, 0x85}, // ZC
        {0x8001, 0x0002, 0x80, 0x81}, // C
        {0xC000, 0x8000, 0x80, 0x89}, // NC
        // C = 1
        {0x0000, 0x0001, 0x81, 0x80},
        {0x4000, 0x8001, 0x81, 0x88}, // N
        {0x8000, 0x0001, 0x81, 0x81}, // C
        {0xC000, 0x8001, 0x81, 0x89}, // NC
    };

    static const TestValue16 tests16_2[]
    {
        // C = 0
        {0x0000, 0x0000, 0x80, 0x84}, // Z
        {0x0001, 0x0004, 0x80, 0x80},
        {0x2000, 0x8000, 0x80, 0x88}, // N
        {0x4000, 0x0000, 0x80, 0x85}, // ZC
        {0x4001, 0x0004, 0x80, 0x81}, // C
        {0x6000, 0x8000, 0x80, 0x89}, // NC
        {0x8000, 0x0001, 0x80, 0x80},

        // C = 1
        {0x0000, 0x0002, 0x81, 0x80},
        {0x2000, 0x8002, 0x81, 0x88}, // N
        {0x4000, 0x0002, 0x81, 0x81}, // C
        {0x6000, 0x8002, 0x81, 0x89}, // NC
        {0x8000, 0x0003, 0x81, 0x80},
    };
    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("ROTXL 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxl %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_1)
    {
        char buf[60];
        auto &test = tests16_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTXL 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxl #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_2)
    {
        char buf[60];
        auto &test = tests16_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotxl8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char inCC, cc;
    };

    static const TestValue8 tests8_1[]
    {
        // C = 0
        {0x00, 0x00, 0x80, 0x84}, // Z
        {0x01, 0x02, 0x80, 0x80},
        {0x40, 0x80, 0x80, 0x88}, // N
        {0x80, 0x00, 0x80, 0x85}, // ZC
        {0x81, 0x02, 0x80, 0x81}, // C
        {0xC0, 0x80, 0x80, 0x89}, // NC
        // C = 1
        {0x00, 0x01, 0x81, 0x80},
        {0x40, 0x81, 0x81, 0x88}, // N
        {0x80, 0x01, 0x81, 0x81}, // C
        {0xC0, 0x81, 0x81, 0x89}, // NC
    };

    static const TestValue8 tests8_2[]
    {
        // C = 0
        {0x00, 0x00, 0x80, 0x84}, // Z
        {0x01, 0x04, 0x80, 0x80},
        {0x20, 0x80, 0x80, 0x88}, // N
        {0x40, 0x00, 0x80, 0x85}, // ZC
        {0x41, 0x04, 0x80, 0x81}, // C
        {0x60, 0x80, 0x80, 0x89}, // NC
        {0x80, 0x01, 0x80, 0x80},

        // C = 1
        {0x00, 0x02, 0x81, 0x80},
        {0x20, 0x82, 0x81, 0x88}, // N
        {0x40, 0x02, 0x81, 0x81}, // C
        {0x60, 0x82, 0x81, 0x89}, // NC
        {0x80, 0x03, 0x81, 0x80},
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("ROTXL 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxl %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_1)
    {
        char buf[60];
        auto &test = tests8_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTXL 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxl #2, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_2)
    {
        char buf[60];
        auto &test = tests8_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotxr32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char inCC, cc;
    };

    static const TestValue32 tests32_1[]
    {
        // C = 0
        {0x00000000, 0x00000000, 0x80, 0x84}, // Z
        {0x00000001, 0x00000000, 0x80, 0x85}, // ZC
        {0x00000002, 0x00000001, 0x80, 0x80},
        {0x00000003, 0x00000001, 0x80, 0x81}, // C
        // C = 1
        {0x00000000, 0x80000000, 0x81, 0x88}, // N
        {0x00000001, 0x80000000, 0x81, 0x89}, // NC
    };

    static const TestValue32 tests32_2[]
    {
        // C = 0
        {0x00000000, 0x00000000, 0x80, 0x84}, // Z
        {0x00000001, 0x80000000, 0x80, 0x88}, // N
        {0x00000002, 0x00000000, 0x80, 0x85}, // ZC
        {0x00000003, 0x80000000, 0x80, 0x89}, // NC
        {0x00000004, 0x00000001, 0x80, 0x80},
        {0x00000006, 0x00000001, 0x80, 0x81}, // C

        // C = 1
        {0x00000000, 0x40000000, 0x81, 0x80},
        {0x00000001, 0xC0000000, 0x81, 0x88}, // N
        {0x00000002, 0x40000000, 0x81, 0x81}, // C
        {0x00000003, 0xC0000000, 0x81, 0x89}, // NC
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("ROTXR 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxr %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_1)
    {
        char buf[60];
        auto &test = tests32_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTXR 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxr #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests32_2)
    {
        char buf[60];
        auto &test = tests32_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %08lX (%02X), got %08lX (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotxr16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char inCC, cc;
    };

    static const TestValue16 tests16_1[]
    {
        // C = 0
        {0x0000, 0x0000, 0x80, 0x84}, // Z
        {0x0001, 0x0000, 0x80, 0x85}, // ZC
        {0x0002, 0x0001, 0x80, 0x80},
        {0x0003, 0x0001, 0x80, 0x81}, // C
        // C = 1
        {0x0000, 0x8000, 0x81, 0x88}, // N
        {0x0001, 0x8000, 0x81, 0x89}, // NC
    };

    static const TestValue16 tests16_2[]
    {
        // C = 0
        {0x0000, 0x0000, 0x80, 0x84}, // Z
        {0x0001, 0x8000, 0x80, 0x88}, // N
        {0x0002, 0x0000, 0x80, 0x85}, // ZC
        {0x0003, 0x8000, 0x80, 0x89}, // NC
        {0x0004, 0x0001, 0x80, 0x80},
        {0x0006, 0x0001, 0x80, 0x81}, // C

        // C = 1
        {0x0000, 0x4000, 0x81, 0x80},
        {0x0001, 0xC000, 0x81, 0x88}, // N
        {0x0002, 0x4000, 0x81, 0x81}, // C
        {0x0003, 0xC000, 0x81, 0x89}, // NC
    };
    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("ROTXR 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxr %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_1)
    {
        char buf[60];
        auto &test = tests16_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTXR 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxr #2, %2\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests16_2)
    {
        char buf[60];
        auto &test = tests16_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %04X (%02X), got %04X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}

void rotxr8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char inCC, cc;
    };

    static const TestValue8 tests8_1[]
    {
        // C = 0
        {0x00, 0x00, 0x80, 0x84}, // Z
        {0x01, 0x00, 0x80, 0x85}, // ZC
        {0x02, 0x01, 0x80, 0x80},
        {0x03, 0x01, 0x80, 0x81}, // C
        // C = 1
        {0x00, 0x80, 0x81, 0x88}, // N
        {0x01, 0x80, 0x81, 0x89}, // NC
    };

    static const TestValue8 tests8_2[]
    {
        // C = 0
        {0x00, 0x00, 0x80, 0x84}, // Z
        {0x01, 0x80, 0x80, 0x88}, // N
        {0x02, 0x00, 0x80, 0x85}, // ZC
        {0x03, 0x80, 0x80, 0x89}, // NC
        {0x04, 0x01, 0x80, 0x80},
        {0x06, 0x01, 0x80, 0x81}, // C

        // C = 1
        {0x00, 0x40, 0x81, 0x80},
        {0x01, 0xC0, 0x81, 0x88}, // N
        {0x02, 0x40, 0x81, 0x81}, // C
        {0x03, 0xC0, 0x81, 0x89}, // NC
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("ROTXR 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxr %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_1)
    {
        char buf[60];
        auto &test = tests8_1[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");

    writeString("ROTXR 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc %3l, ccr\n"
            "rotxr #2, %2l\n"
            "stc ccr, %1l\n"
            : "=r" (result), "=r" (cc)
            : "0" (test.a), "r" (test.inCC)
            :
        );

        if(result != test.result || cc != test.cc)
            break;

        i++;
    }

    if(i < numTests8_2)
    {
        char buf[60];
        auto &test = tests8_2[i];
        snprintf(buf, sizeof(buf), " failed at %i. Expected %02X (%02X), got %02X (%02X)\n", i + 1, test.result, test.cc, result, cc);
        writeString(buf);
    }
    else
        writeString(".\n");
}
