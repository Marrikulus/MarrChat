#include <iostream>
#include <vector>

#include "RakNet/RakPeerInterface.h"
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/BitStream.h"
#include "RakNet/RakNetTypes.h"

typedef char i8;
typedef short i16;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define global static

#define MAX_CLIENTS 10
#define SERVER_PORT 60000

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

enum {
	ID_MARR_MESSAGE = ID_USER_PACKET_ENUM+1,
	ID_MARR_WRITING,
};


const int MAX_TEXT_LENGTH = 256;
const int MAX_NAME_LENGTH = 4;

#pragma pack(1)
struct Message
{
	u8 type;
	i8 text[MAX_TEXT_LENGTH];
	i8 name[MAX_NAME_LENGTH+1];
};
