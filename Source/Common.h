#include "RakNet/RakPeerInterface.h"
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/BitStream.h"
#include "RakNet/RakNetTypes.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;


#define MAX_CLIENTS 10
#define SERVER_PORT 60000

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

enum {
	ID_MARR_MESSAGE = ID_USER_PACKET_ENUM+1,
	ID_MARR_WRITING,
};