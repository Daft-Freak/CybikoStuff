# RF Packets/Messages

Over serial there's a prefix of `0x30` (short packet) or `0xCF` (long packet), then another byte. The data sent out over RF has a preamble of `0xAA` six times, then `0x00` and finally `0x32` (short) or `0xC8` (long). All multi-byte fields are in the CPU's byte order (big-endian).

A short packet has 14 bytes of data, a long one has 104

```c
uint32_t dstAddr;  // cyid of destination (0xFFFFFFFF is broadcast)
uint32_t srcAddr;  // cyid of source
uint8_t channel;   // | 0xC0 (?)
uint8_t typeFlags; // type << 5 | flags
uint8_t index;     // 0 if only single packet
uint8_t sequence;  // ack required if non-zero
uint16_t dataCRC;  // CRC16 of data
uint16_t headCRC;  // CRC16 of the previous 14 bytes
// [data] (14/104 bytes)
// [ecc] (20/80 bytes)
```

## Type 0 - ping

Header flags
```
(1 << 0) = valid profile? not set for classic vcywig
(1 << 1) = standby?
(1 << 2) = gate/server mode
(1 << 3) = power?
(1 << 4)
```

Example packet ("Daft", male, 45)
```
2A                      random? incremented sometimes
00 00 00
44 61 66 74 00 77 6E 00 name
2D                      age | gender << 7
00
```

## Type 1 - message

First packet:
```c
// always present
uint16_t msgid;
uint16_t flags;
// optional
uint32_t param0;
uint32_t param1;
// [null-terminated string] or [16 bit number]

uint8_t bufLen; // if complete buffer, otherwise implicitly the rest of tha packet
// [buffer data]
```

Other non-final packets have more data for the buffer. The final packet has a byte prefixed for the remaining length.

Message flags
```
(1 << 13) = has name (otherwise name is a 16-bit number)
(1 << 12) = has buffer
(1 << 11) = has param0
(1 << 10) = has param1
(1 <<  9) = partial buf?
(1 <<  8) = last part of buf
```

## Type 2 - ack?

Sent in reply to massage packets with a non-zero sequence number. There's some value in the upper half of the first byte.

## Messages

### App 0002 (finder)
 - `1000` - profile
 - `1001` - MSG_USER_FOLK in SDK
 - `100A` - request cypage
 - `100F` - request bcard
 - `1014` - request profile
 - `1016` - request photo
 - `1019` - send bcard



#### Profile message

```
10 00                   flags (has buf)
10 00                   msgid
00 02                   dst app
5A                      buf len
buf
44 61 66 74 00 77 6E 00 name
00 00 00 00 00 00 00 00 "my" secret fields?
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 "your" secret fields
00 00 00 00 00 00 00 00 (8 chars each)
00 00 00 00 00 00 00 00 (...)
2D                      age
B0                      height (cm)
A5                      weight (lbs)
0F 1E                   "your" age range
A0 B4                   "your" height range (cm)
96 B4                   "your" weight range (lbs)
01 0C 0B                "my" hobbies?
01 0C 0B                "your" hobbies
44                      gender prio << 4 | age prio
33                      height prio << 4 | weight prio
34                      hobby 1 prio << 4 | ?
34                      hobby 2 prio << 4 | ?
34                      hobby 3 prio << 4 | ?
00 00 00 00 FF 7E 7E 00 00 00
00                      visibility mask
10                      gender preference << 4 | gender ?
B1                      "your" purpose (only 2 bits?) | priority << 4 | "my" purpose visible << 7
09                      "my" purpose << 3 | "your" purpose?
```

priority (degree in the SDK docs)
- 0 very undesirable
- 1 undesirable
- 2 neutral
- 3 desirable
- 4 mandatory

visibility
- 01 age
- 02 height
- 04 weight
- 08 hobby 1
- 10 hobby 2
- 20 hobby 3

purpose
- 0 business
- 1 romance
- 2 sports
- 3 chat
- 4 other

gender
- 0 male
- 1 female
- 2 doesn't matter (only for "you")

hobbies
- 00 not specified
- 01 cybiko
- 02 soccer
- 03 chess
- 04 rollerblading
- 05 skateboarding
- 06 skydiving
- 07 bmx
- 08 music
- 09 gaming
- 0A dance
- 0B internet
- 0C computer
- 0D sports
- 0E arts
- 0F science/tech
- 10 travel
- 11 mountain biking
- 12 books
- 13 movies
- 14 skiing
- 15 girls
- 16 boys
- 17 doesn't matter
