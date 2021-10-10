#include <stdio.h>

#include "Registers.hpp"

#include "helpers.h"


void shal32Test()
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
        {0x40000000, 0x80000000, 0x8A}, // NV
        {0x80000000, 0x00000000, 0x87}, // ZVC
        {0x80000001, 0x00000002, 0x83}, // VC
        {0xC0000000, 0x80000000, 0x89}, // NC
    };

    static const TestValue32 tests32_2[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000004, 0x80},
        {0x20000000, 0x80000000, 0x8A}, // NV
        {0x40000000, 0x00000000, 0x87}, // ZVC
        {0x40000001, 0x00000004, 0x83}, // VC
        {0x60000000, 0x80000000, 0x8B}, // NVC
        {0x80000000, 0x00000000, 0x86}, // ZV
        {0x80000001, 0x00000004, 0x82}, // V
        {0xE0000000, 0x80000000, 0x89}, // NC
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("SHAL 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shal %2\n"
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

    writeString("SHAL 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shal #2, %2\n"
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

void shal16Test()
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
        {0x4000, 0x8000, 0x8A}, // NV
        {0x8000, 0x0000, 0x87}, // ZVC
        {0x8001, 0x0002, 0x83}, // VC
        {0xC000, 0x8000, 0x89}, // NC
    };

    static const TestValue16 tests16_2[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0004, 0x80},
        {0x2000, 0x8000, 0x8A}, // NV
        {0x4000, 0x0000, 0x87}, // ZVC
        {0x4001, 0x0004, 0x83}, // VC
        {0x6000, 0x8000, 0x8B}, // NVC
        {0x8000, 0x0000, 0x86}, // ZV
        {0x8001, 0x0004, 0x82}, // V
        {0xE000, 0x8000, 0x89}, // NC
    };
    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("SHAL 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shal %2\n"
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

    writeString("SHAL 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shal #2, %2\n"
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

void shal8Test()
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
        {0x40, 0x80, 0x8A}, // NV
        {0x80, 0x00, 0x87}, // ZVC
        {0x81, 0x02, 0x83}, // VC
        {0xC0, 0x80, 0x89}, // NC
    };

    static const TestValue8 tests8_2[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x04, 0x80},
        {0x20, 0x80, 0x8A}, // NV
        {0x40, 0x00, 0x87}, // ZVC
        {0x41, 0x04, 0x83}, // VC
        {0x60, 0x80, 0x8B}, // NVC
        {0x80, 0x00, 0x86}, // ZV
        {0x81, 0x04, 0x82}, // V
        {0xE0, 0x80, 0x89}, // NC
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("SHAL 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shal %2l\n"
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

    writeString("SHAL 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shal #2, %2l\n"
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

void shar32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32_1[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000000, 0x85}, // ZC
        {0x00000002, 0x00000001, 0x80},
        {0x00000003, 0x00000001, 0x81}, // C
        {0x80000000, 0xC0000000, 0x88}, // N
        {0x80000001, 0xC0000000, 0x89}, // NC
    };

    static const TestValue32 tests32_2[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000002, 0x00000000, 0x85}, // ZC
        {0x00000004, 0x00000001, 0x80},
        {0x00000006, 0x00000001, 0x81}, // C
        {0x80000000, 0xE0000000, 0x88}, // N
        {0x80000002, 0xE0000000, 0x89}, // NC
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("SHAR 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shar %2\n"
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

    writeString("SHAR 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shar #2, %2\n"
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

void shar16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16_1[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0000, 0x85}, // ZC
        {0x0002, 0x0001, 0x80},
        {0x0003, 0x0001, 0x81}, // C
        {0x8000, 0xC000, 0x88}, // N
        {0x8001, 0xC000, 0x89}, // NC
    };

    static const TestValue16 tests16_2[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0002, 0x0000, 0x85}, // ZC
        {0x0004, 0x0001, 0x80},
        {0x0006, 0x0001, 0x81}, // C
        {0x8000, 0xE000, 0x88}, // N
        {0x8002, 0xE000, 0x89}, // NC
    };

    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("SHAR 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shar %2\n"
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

    writeString("SHAR 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shar #2, %2\n"
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

void shar8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char cc;
    };

    static const TestValue8 tests8_1[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x00, 0x85}, // ZC
        {0x02, 0x01, 0x80},
        {0x03, 0x01, 0x81}, // C
        {0x80, 0xC0, 0x88}, // N
        {0x81, 0xC0, 0x89}, // NC
    };

    static const TestValue8 tests8_2[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x02, 0x00, 0x85}, // ZC
        {0x04, 0x01, 0x80},
        {0x06, 0x01, 0x81}, // C
        {0x80, 0xE0, 0x88}, // N
        {0x82, 0xE0, 0x89}, // NC
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("SHAR 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shar %2l\n"
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

    writeString("SHAR 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shar #2, %2l\n"
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


void shll32Test()
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
        {0x80000000, 0x00000000, 0x85}, // ZC
        {0x80000001, 0x00000002, 0x81}, // C
        {0xC0000000, 0x80000000, 0x89}, // NC
    };

    static const TestValue32 tests32_2[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000004, 0x80},
        {0x20000000, 0x80000000, 0x88}, // N
        {0x40000000, 0x00000000, 0x85}, // ZC
        {0x40000001, 0x00000004, 0x81}, // VC
        {0x60000000, 0x80000000, 0x89}, // NC
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("SHLL 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shll %2\n"
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

    writeString("SHLL 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shll #2, %2\n"
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

void shll16Test()
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
        {0x8000, 0x0000, 0x85}, // ZC
        {0x8001, 0x0002, 0x81}, // C
        {0xC000, 0x8000, 0x89}, // NC
    };

    static const TestValue16 tests16_2[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0004, 0x80},
        {0x2000, 0x8000, 0x88}, // N
        {0x4000, 0x0000, 0x85}, // ZC
        {0x4001, 0x0004, 0x81}, // VC
        {0x6000, 0x8000, 0x89}, // NC
    };
    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("SHLL 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shll %2\n"
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

    writeString("SHLL 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shll #2, %2\n"
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

void shll8Test()
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
        {0x80, 0x00, 0x85}, // ZC
        {0x81, 0x02, 0x81}, // C
        {0xC0, 0x80, 0x89}, // NC
    };

    static const TestValue8 tests8_2[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x04, 0x80},
        {0x20, 0x80, 0x88}, // N
        {0x40, 0x00, 0x85}, // ZC
        {0x41, 0x04, 0x81}, // VC
        {0x60, 0x80, 0x89}, // NC
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("SHLL 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shll %2l\n"
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

    writeString("SHLL 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shll #2, %2l\n"
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

void shlr32Test()
{
    struct TestValue32
    {
        unsigned long a, result;
        unsigned char cc;
    };

    static const TestValue32 tests32_1[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000001, 0x00000000, 0x85}, // ZC
        {0x00000002, 0x00000001, 0x80},
        {0x00000003, 0x00000001, 0x81}, // C
        {0x80000000, 0x40000000, 0x80},
    };

    static const TestValue32 tests32_2[]
    {
        {0x00000000, 0x00000000, 0x84}, // Z
        {0x00000002, 0x00000000, 0x85}, // ZC
        {0x00000004, 0x00000001, 0x80},
        {0x00000006, 0x00000001, 0x81}, // C
        {0x80000000, 0x20000000, 0x80},
    };

    static const int numTests32_1 = sizeof(tests32_1) / sizeof(tests32_1[0]);
    static const int numTests32_2 = sizeof(tests32_2) / sizeof(tests32_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned long result;

    writeString("SHLR 1 (32)");

    for(auto &test : tests32_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shlr %2\n"
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

    writeString("SHLR 2 (32)");
    i = 0;

    for(auto &test : tests32_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shlr #2, %2\n"
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

void shlr16Test()
{
    struct TestValue16
    {
        unsigned int a, result;
        unsigned char cc;
    };

    static const TestValue16 tests16_1[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0001, 0x0000, 0x85}, // ZC
        {0x0002, 0x0001, 0x80},
        {0x0003, 0x0001, 0x81}, // C
        {0x8000, 0x4000, 0x80},
    };

    static const TestValue16 tests16_2[]
    {
        {0x0000, 0x0000, 0x84}, // Z
        {0x0002, 0x0000, 0x85}, // ZC
        {0x0004, 0x0001, 0x80},
        {0x0006, 0x0001, 0x81}, // C
        {0x8000, 0x2000, 0x80},
    };

    static const int numTests16_1 = sizeof(tests16_1) / sizeof(tests16_1[0]);
    static const int numTests16_2 = sizeof(tests16_2) / sizeof(tests16_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned int result;

    writeString("SHLR 1 (16)");

    for(auto &test : tests16_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shlr %2\n"
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

    writeString("SHLR 2 (16)");
    i = 0;

    for(auto &test : tests16_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shlr #2, %2\n"
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

void shlr8Test()
{
    struct TestValue8
    {
        unsigned char a, result;
        unsigned char cc;
    };

    static const TestValue8 tests8_1[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x01, 0x00, 0x85}, // ZC
        {0x02, 0x01, 0x80},
        {0x03, 0x01, 0x81}, // C
        {0x80, 0x40, 0x80},
    };

    static const TestValue8 tests8_2[]
    {
        {0x00, 0x00, 0x84}, // Z
        {0x02, 0x00, 0x85}, // ZC
        {0x04, 0x01, 0x80},
        {0x06, 0x01, 0x81}, // C
        {0x80, 0x20, 0x80},
    };
    static const int numTests8_1 = sizeof(tests8_1) / sizeof(tests8_1[0]);
    static const int numTests8_2 = sizeof(tests8_2) / sizeof(tests8_2[0]);

    int i = 0;

    unsigned char cc;
    unsigned char result;

    writeString("SHLR 1 (8)");

    for(auto &test : tests8_1)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shlr %2l\n"
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

    writeString("SHLR 2 (8)");
    i = 0;

    for(auto &test : tests8_2)
    {
        asm volatile(
            "ldc #0x80, ccr\n"
            "shlr #2, %2l\n"
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

