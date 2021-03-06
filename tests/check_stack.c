#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "ua_transport.h"
#include "ua_connection.h"
#include "server/ua_securechannel_manager.h"
#include "server/ua_session_manager.h"
#include "check.h"

#define MAXMSG 512
#define BUFFER_SIZE 8192

/** @brief this structure holds all the information for the stack test fixture */
typedef struct stackTestFixture {
	/** @brief the actual buffer to receive the response msg */
	UA_Byte respMsgBuffer[BUFFER_SIZE];
	/** @brief the management structure (initialized to point to respMsgBuffer in create */
	UA_ByteString respMsg;
	/** @brief the management data structure for the fake connection */
	UA_Connection *connection;
} stackTestFixture;

/** @brief the maximum number of parallel fixtures.
 * ( MAX_FIXTURES needs to match the number of bytes of fixturesMap )*/
#define MAX_FIXTURES 32
/** @brief this map marks the slots in fixtures as free (bit=0) or in use (bit=1) */
UA_Int32 fixturesMap = 0;
/** @brief the array of pointers to the fixtures */
stackTestFixture *fixtures[MAX_FIXTURES];

/** @brief search first free handle, set and return */
UA_Int32 stackTestFixture_getAndMarkFreeHandle() {
	UA_UInt32 freeFixtureHandle = 0;
	for(freeFixtureHandle = 0;freeFixtureHandle < MAX_FIXTURES;freeFixtureHandle++) {
		if(!(fixturesMap & (1 << freeFixtureHandle))) { // when free
			fixturesMap |= (1 << freeFixtureHandle);    // then set
			return freeFixtureHandle;
		}
	}
	return UA_ERR_NO_MEMORY;
}

/** @brief clear bit in fixture map */
UA_Int32 stackTestFixture_markHandleAsFree(UA_Int32 fixtureHandle) {
	if(fixtureHandle >= 0 && fixtureHandle < MAX_FIXTURES) {
		fixturesMap &= ~(1 << fixtureHandle);  // clear bit
		return UA_SUCCESS;
	}
	return UA_ERR_INVALID_VALUE;
}

UA_Int32 closerCallback(UA_Connection *connection) {
	return UA_SUCCESS;
}

/** @brief get a handle to a free slot and create a new stackTestFixture */
UA_Int32 stackTestFixture_create(UA_Connection_writeCallback write, UA_Connection_closeCallback close) {
	UA_UInt32 fixtureHandle = stackTestFixture_getAndMarkFreeHandle();
	stackTestFixture *fixture = fixtures[fixtureHandle];
	UA_alloc((void**)&fixture->connection, sizeof(UA_Connection));
	fixture->respMsg.data   = fixture->respMsgBuffer;
	fixture->respMsg.length = 0;
	UA_Connection_init(fixture->connection, UA_ConnectionConfig_standard, fixture, close, write);
	return fixtureHandle;
}

/** @brief free the allocated memory of the stackTestFixture associated with the handle */
UA_Int32 stackTestFixture_delete(UA_UInt32 fixtureHandle) {
	if(fixtureHandle < MAX_FIXTURES) {
		UA_free(fixtures[fixtureHandle]);
		stackTestFixture_markHandleAsFree(fixtureHandle);
		return UA_SUCCESS;
	}
	return UA_ERR_INVALID_VALUE;
}

/** @brief return the fixture associated with the handle */
stackTestFixture *stackTestFixture_getFixture(UA_UInt32 fixtureHandle) {
	if(fixtureHandle < MAX_FIXTURES && ( fixturesMap & (1 << fixtureHandle)))
		return fixtures[fixtureHandle];
	return UA_NULL;
}

/** @brief write message provided in the gather buffers to the buffer of the fixture */
UA_Int32 responseMsg(UA_Int32 connectionHandle, UA_ByteString const **gather_buf, UA_Int32 gather_len) {
	stackTestFixture *fixture = stackTestFixture_getFixture(connectionHandle);
	UA_Int32  retval    = UA_SUCCESS;
	UA_UInt32 total_len = 0;

	for(UA_Int32 i = 0;i < gather_len && retval == UA_SUCCESS;++i) {
		if(total_len + gather_buf[i]->length < BUFFER_SIZE) {
			memcpy(&(fixture->respMsg.data[total_len]), gather_buf[i]->data, gather_buf[i]->length);
			total_len += gather_buf[i]->length;
		} else
			retval = UA_ERR_NO_MEMORY;
	}
	fixture->respMsg.length = total_len;
	return UA_SUCCESS;
}

void indicateMsg(UA_Int32 handle, UA_ByteString *slMessage) {
	printf("indicate: %d", TL_Process((stackTestFixture_getFixture(handle)->connection), slMessage));
}

UA_Byte pkt_HEL[] = {
	0x48, 0x45, 0x4c, 0x46, 0x39, 0x00,             /*   HELF9. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /* ........ */
	0x00, 0x01, 0x88, 0x13, 0x00, 0x00, 0x19, 0x00, /* ........ */
	0x00, 0x00, 0x6f, 0x70, 0x63, 0x2e, 0x74, 0x63, /* ..opc.tc */
	0x70, 0x3a, 0x2f, 0x2f, 0x31, 0x30, 0x2e, 0x30, /* p://10.0 */
	0x2e, 0x35, 0x34, 0x2e, 0x37, 0x37, 0x3a, 0x34, /* .54.77:4 */
	0x38, 0x34, 0x32                                /* 842 */
};
UA_Byte pkt_OPN[] = {
	0x4f, 0x50, 0x4e, 0x46, 0x85, 0x00,             /*   OPNF.. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x00, /* ....../. */
	0x00, 0x00, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, /* ..http:/ */
	0x2f, 0x6f, 0x70, 0x63, 0x66, 0x6f, 0x75, 0x6e, /* /opcfoun */
	0x64, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2e, 0x6f, /* dation.o */
	0x72, 0x67, 0x2f, 0x55, 0x41, 0x2f, 0x53, 0x65, /* rg/UA/Se */
	0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x50, 0x6f, /* curityPo */
	0x6c, 0x69, 0x63, 0x79, 0x23, 0x4e, 0x6f, 0x6e, /* licy#Non */
	0x65, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* e....... */
	0xff, 0x33, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, /* .3...... */
	0x00, 0x01, 0x00, 0xbe, 0x01, 0x00, 0x00, 0x40, /* .......@ */
	0xaf, 0xfc, 0xe8, 0xa1, 0x76, 0xcf, 0x01, 0x00, /* ....v... */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, /* ........ */
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, /* ........ */
	0x00, 0x00, 0x00, 0x80, 0xee, 0x36, 0x00        /* .....6. */
};

UA_Byte pkt_CLO[] = {
	0x43, 0x4c, 0x4f, 0x46, 0x39, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,  /* CLOF9........... */ /*19 here assusmes channelid=25 ! */
	0xea, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x01, 0x00, 0xc4, 0x01, 0x00, 0x00, 0x4d, 0x65,  /* ..............Me */
	0x16, 0x3b, 0x47, 0x99, 0xcf, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,  /* .;G............. */
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00                                             /* ......... */
};

UA_Byte pkt_MSG_CreateSession[] = {
	0x4d, 0x53, 0x47, 0x46, 0xb4, 0x05, 0x00, 0x00, /* MSGF.... */
	0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /* QQ...... // assumes fixed secureChannelID=25 ! */
	0x34, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, /* 4....... */
	0x01, 0x00, 0xcd, 0x01, 0x00, 0x00, 0x50, 0xd6, /* ......P. */
	0xfc, 0xe8, 0xa1, 0x76, 0xcf, 0x01, 0x01, 0x00, /* ...v.... */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, /* ........ */
	0xff, 0xff, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, /* ...'.... */
	0x00, 0x2d, 0x00, 0x00, 0x00, 0x75, 0x72, 0x6e, /* .-...urn */
	0x3a, 0x6d, 0x72, 0x74, 0x2d, 0x56, 0x69, 0x72, /* :mrt-Vir */
	0x74, 0x75, 0x61, 0x6c, 0x42, 0x6f, 0x78, 0x3a, /* tualBox: */
	0x55, 0x6e, 0x69, 0x66, 0x69, 0x65, 0x64, 0x41, /* UnifiedA */
	0x75, 0x74, 0x6f, 0x6d, 0x61, 0x74, 0x69, 0x6f, /* utomatio */
	0x6e, 0x3a, 0x55, 0x61, 0x45, 0x78, 0x70, 0x65, /* n:UaExpe */
	0x72, 0x74, 0x1e, 0x00, 0x00, 0x00, 0x75, 0x72, /* rt....ur */
	0x6e, 0x3a, 0x55, 0x6e, 0x69, 0x66, 0x69, 0x65, /* n:Unifie */
	0x64, 0x41, 0x75, 0x74, 0x6f, 0x6d, 0x61, 0x74, /* dAutomat */
	0x69, 0x6f, 0x6e, 0x3a, 0x55, 0x61, 0x45, 0x78, /* ion:UaEx */
	0x70, 0x65, 0x72, 0x74, 0x02, 0x1b, 0x00, 0x00, /* pert.... */
	0x00, 0x55, 0x6e, 0x69, 0x66, 0x69, 0x65, 0x64, /* .Unified */
	0x20, 0x41, 0x75, 0x74, 0x6f, 0x6d, 0x61, 0x74, /*  Automat */
	0x69, 0x6f, 0x6e, 0x20, 0x55, 0x61, 0x45, 0x78, /* ion UaEx */
	0x70, 0x65, 0x72, 0x74, 0x01, 0x00, 0x00, 0x00, /* pert.... */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* ........ */
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, /* ........ */
	0x19, 0x00, 0x00, 0x00, 0x6f, 0x70, 0x63, 0x2e, /* ....opc. */
	0x74, 0x63, 0x70, 0x3a, 0x2f, 0x2f, 0x31, 0x30, /* tcp://10 */
	0x2e, 0x30, 0x2e, 0x35, 0x34, 0x2e, 0x37, 0x37, /* .0.54.77 */
	0x3a, 0x34, 0x38, 0x34, 0x32, 0x2d, 0x00, 0x00, /* :4842-.. */
	0x00, 0x75, 0x72, 0x6e, 0x3a, 0x6d, 0x72, 0x74, /* .urn:mrt */
	0x2d, 0x56, 0x69, 0x72, 0x74, 0x75, 0x61, 0x6c, /* -Virtual */
	0x42, 0x6f, 0x78, 0x3a, 0x55, 0x6e, 0x69, 0x66, /* Box:Unif */
	0x69, 0x65, 0x64, 0x41, 0x75, 0x74, 0x6f, 0x6d, /* iedAutom */
	0x61, 0x74, 0x69, 0x6f, 0x6e, 0x3a, 0x55, 0x61, /* ation:Ua */
	0x45, 0x78, 0x70, 0x65, 0x72, 0x74, 0x20, 0x00, /* Expert . */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
	0x00, 0x00, 0x72, 0x04, 0x00, 0x00, 0x30, 0x82, /* ..r...0. */
	0x04, 0x6e, 0x30, 0x82, 0x03, 0xd7, 0xa0, 0x03, /* .n0..... */
	0x02, 0x01, 0x02, 0x02, 0x04, 0x53, 0x1b, 0x10, /* .....S.. */
	0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, /* .0...*.H */
	0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, /* ........ */
	0x30, 0x81, 0x93, 0x31, 0x0b, 0x30, 0x09, 0x06, /* 0..1.0.. */
	0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x44, 0x45, /* .U....DE */
	0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, /* 1.0...U. */
	0x08, 0x13, 0x08, 0x41, 0x6e, 0x79, 0x77, 0x68, /* ...Anywh */
	0x65, 0x72, 0x65, 0x31, 0x11, 0x30, 0x0f, 0x06, /* ere1.0.. */
	0x03, 0x55, 0x04, 0x07, 0x13, 0x08, 0x41, 0x6e, /* .U....An */
	0x79, 0x77, 0x68, 0x65, 0x72, 0x65, 0x31, 0x13, /* ywhere1. */
	0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, /* 0...U... */
	0x0a, 0x54, 0x55, 0x20, 0x44, 0x72, 0x65, 0x73, /* .TU Dres */
	0x64, 0x65, 0x6e, 0x31, 0x36, 0x30, 0x34, 0x06, /* den1604. */
	0x03, 0x55, 0x04, 0x0b, 0x13, 0x2d, 0x43, 0x68, /* .U...-Ch */
	0x61, 0x69, 0x72, 0x20, 0x66, 0x6f, 0x72, 0x20, /* air for  */
	0x50, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x20, /* Process  */
	0x43, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, /* Control  */
	0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x73, 0x20, /* Systems  */
	0x45, 0x6e, 0x67, 0x69, 0x6e, 0x65, 0x65, 0x72, /* Engineer */
	0x69, 0x6e, 0x67, 0x31, 0x11, 0x30, 0x0f, 0x06, /* ing1.0.. */
	0x03, 0x55, 0x04, 0x03, 0x13, 0x08, 0x55, 0x61, /* .U....Ua */
	0x45, 0x78, 0x70, 0x65, 0x72, 0x74, 0x30, 0x1e, /* Expert0. */
	0x17, 0x0d, 0x31, 0x34, 0x30, 0x33, 0x30, 0x38, /* ..140308 */
	0x31, 0x32, 0x34, 0x34, 0x31, 0x35, 0x5a, 0x17, /* 124415Z. */
	0x0d, 0x31, 0x34, 0x30, 0x33, 0x30, 0x38, 0x31, /* .1403081 */
	0x33, 0x34, 0x34, 0x31, 0x35, 0x5a, 0x30, 0x81, /* 34415Z0. */
	0x93, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, /* .1.0...U */
	0x04, 0x06, 0x13, 0x02, 0x44, 0x45, 0x31, 0x11, /* ....DE1. */
	0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, /* 0...U... */
	0x08, 0x41, 0x6e, 0x79, 0x77, 0x68, 0x65, 0x72, /* .Anywher */
	0x65, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, /* e1.0...U */
	0x04, 0x07, 0x13, 0x08, 0x41, 0x6e, 0x79, 0x77, /* ....Anyw */
	0x68, 0x65, 0x72, 0x65, 0x31, 0x13, 0x30, 0x11, /* here1.0. */
	0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0a, 0x54, /* ..U....T */
	0x55, 0x20, 0x44, 0x72, 0x65, 0x73, 0x64, 0x65, /* U Dresde */
	0x6e, 0x31, 0x36, 0x30, 0x34, 0x06, 0x03, 0x55, /* n1604..U */
	0x04, 0x0b, 0x13, 0x2d, 0x43, 0x68, 0x61, 0x69, /* ...-Chai */
	0x72, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x50, 0x72, /* r for Pr */
	0x6f, 0x63, 0x65, 0x73, 0x73, 0x20, 0x43, 0x6f, /* ocess Co */
	0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x53, 0x79, /* ntrol Sy */
	0x73, 0x74, 0x65, 0x6d, 0x73, 0x20, 0x45, 0x6e, /* stems En */
	0x67, 0x69, 0x6e, 0x65, 0x65, 0x72, 0x69, 0x6e, /* gineerin */
	0x67, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, /* g1.0...U */
	0x04, 0x03, 0x13, 0x08, 0x55, 0x61, 0x45, 0x78, /* ....UaEx */
	0x70, 0x65, 0x72, 0x74, 0x30, 0x81, 0x9f, 0x30, /* pert0..0 */
	0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, /* ...*.H.. */
	0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x81, /* ........ */
	0x8d, 0x00, 0x30, 0x81, 0x89, 0x02, 0x81, 0x81, /* ..0..... */
	0x00, 0xb3, 0xb3, 0xc9, 0x97, 0xb7, 0x4f, 0x6d, /* ......Om */
	0x6f, 0x72, 0x48, 0xe2, 0x5f, 0x8c, 0x89, 0x0f, /* orH._... */
	0xc3, 0x47, 0x17, 0x4c, 0xd2, 0x8c, 0x2a, 0x85, /* .G.L..*. */
	0xf6, 0x80, 0xb1, 0x9e, 0xf4, 0x90, 0xff, 0x0f, /* ........ */
	0xff, 0x42, 0x74, 0x75, 0xcd, 0xd5, 0xe0, 0x8f, /* .Btu.... */
	0x7f, 0xa1, 0x41, 0x86, 0x83, 0xcf, 0x2c, 0xef, /* ..A...,. */
	0xbd, 0xb7, 0xbf, 0x50, 0xa9, 0x5c, 0xfa, 0x39, /* ...P.\.9 */
	0x84, 0xbb, 0x7e, 0xc9, 0x7e, 0x5b, 0xc8, 0x1b, /* ..~.~[.. */
	0x19, 0xfc, 0x31, 0x05, 0xa9, 0x0c, 0x31, 0x3c, /* ..1...1< */
	0x1a, 0x86, 0x50, 0x17, 0x45, 0x0a, 0xfd, 0xfe, /* ..P.E... */
	0xa0, 0xc4, 0x88, 0x93, 0xff, 0x1c, 0xf3, 0x60, /* .......` */
	0x06, 0xc6, 0xdf, 0x7c, 0xc6, 0xcd, 0x95, 0x7d, /* ...|...} */
	0xf8, 0x3b, 0x7a, 0x53, 0x15, 0xbb, 0x2e, 0xcf, /* .;zS.... */
	0xd1, 0x63, 0xae, 0x5a, 0x30, 0x48, 0x67, 0x5f, /* .c.Z0Hg_ */
	0xa8, 0x30, 0x7f, 0x35, 0xe4, 0x43, 0x94, 0xa3, /* .0.5.C.. */
	0xc1, 0xfe, 0x69, 0xcd, 0x5c, 0xd7, 0x88, 0xc0, /* ..i.\... */
	0xa5, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x82, /* ........ */
	0x01, 0xcb, 0x30, 0x82, 0x01, 0xc7, 0x30, 0x0c, /* ..0...0. */
	0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, /* ..U..... */
	0x04, 0x02, 0x30, 0x00, 0x30, 0x50, 0x06, 0x09, /* ..0.0P.. */
	0x60, 0x86, 0x48, 0x01, 0x86, 0xf8, 0x42, 0x01, /* `.H...B. */
	0x0d, 0x04, 0x43, 0x16, 0x41, 0x22, 0x47, 0x65, /* ..C.A"Ge */
	0x6e, 0x65, 0x72, 0x61, 0x74, 0x65, 0x64, 0x20, /* nerated  */
	0x77, 0x69, 0x74, 0x68, 0x20, 0x55, 0x6e, 0x69, /* with Uni */
	0x66, 0x69, 0x65, 0x64, 0x20, 0x41, 0x75, 0x74, /* fied Aut */
	0x6f, 0x6d, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, /* omation  */
	0x55, 0x41, 0x20, 0x42, 0x61, 0x73, 0x65, 0x20, /* UA Base  */
	0x4c, 0x69, 0x62, 0x72, 0x61, 0x72, 0x79, 0x20, /* Library  */
	0x75, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x4f, 0x70, /* using Op */
	0x65, 0x6e, 0x53, 0x53, 0x4c, 0x22, 0x30, 0x1d, /* enSSL"0. */
	0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, /* ..U..... */
	0x14, 0x37, 0x39, 0x34, 0x93, 0xa2, 0x65, 0xef, /* .794..e. */
	0xb9, 0xd4, 0x71, 0x21, 0x4b, 0x77, 0xdb, 0x28, /* ..q!Kw.( */
	0xc8, 0xfa, 0x03, 0xfe, 0x05, 0x30, 0x81, 0xc3, /* .....0.. */
	0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x81, 0xbb, /* ..U.#... */
	0x30, 0x81, 0xb8, 0x80, 0x14, 0x37, 0x39, 0x34, /* 0....794 */
	0x93, 0xa2, 0x65, 0xef, 0xb9, 0xd4, 0x71, 0x21, /* ..e...q! */
	0x4b, 0x77, 0xdb, 0x28, 0xc8, 0xfa, 0x03, 0xfe, /* Kw.(.... */
	0x05, 0xa1, 0x81, 0x99, 0xa4, 0x81, 0x96, 0x30, /* .......0 */
	0x81, 0x93, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, /* ..1.0... */
	0x55, 0x04, 0x06, 0x13, 0x02, 0x44, 0x45, 0x31, /* U....DE1 */
	0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x08, /* .0...U.. */
	0x13, 0x08, 0x41, 0x6e, 0x79, 0x77, 0x68, 0x65, /* ..Anywhe */
	0x72, 0x65, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, /* re1.0... */
	0x55, 0x04, 0x07, 0x13, 0x08, 0x41, 0x6e, 0x79, /* U....Any */
	0x77, 0x68, 0x65, 0x72, 0x65, 0x31, 0x13, 0x30, /* where1.0 */
	0x11, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0a, /* ...U.... */
	0x54, 0x55, 0x20, 0x44, 0x72, 0x65, 0x73, 0x64, /* TU Dresd */
	0x65, 0x6e, 0x31, 0x36, 0x30, 0x34, 0x06, 0x03, /* en1604.. */
	0x55, 0x04, 0x0b, 0x13, 0x2d, 0x43, 0x68, 0x61, /* U...-Cha */
	0x69, 0x72, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x50, /* ir for P */
	0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x20, 0x43, /* rocess C */
	0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x53, /* ontrol S */
	0x79, 0x73, 0x74, 0x65, 0x6d, 0x73, 0x20, 0x45, /* ystems E */
	0x6e, 0x67, 0x69, 0x6e, 0x65, 0x65, 0x72, 0x69, /* ngineeri */
	0x6e, 0x67, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, /* ng1.0... */
	0x55, 0x04, 0x03, 0x13, 0x08, 0x55, 0x61, 0x45, /* U....UaE */
	0x78, 0x70, 0x65, 0x72, 0x74, 0x82, 0x04, 0x53, /* xpert..S */
	0x1b, 0x10, 0x9f, 0x30, 0x0e, 0x06, 0x03, 0x55, /* ...0...U */
	0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, /* ........ */
	0x02, 0x02, 0xf4, 0x30, 0x20, 0x06, 0x03, 0x55, /* ...0 ..U */
	0x1d, 0x25, 0x01, 0x01, 0xff, 0x04, 0x16, 0x30, /* .%.....0 */
	0x14, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, /* ...+.... */
	0x07, 0x03, 0x01, 0x06, 0x08, 0x2b, 0x06, 0x01, /* .....+.. */
	0x05, 0x05, 0x07, 0x03, 0x02, 0x30, 0x4e, 0x06, /* .....0N. */
	0x03, 0x55, 0x1d, 0x11, 0x04, 0x47, 0x30, 0x45, /* .U...G0E */
	0x86, 0x2d, 0x75, 0x72, 0x6e, 0x3a, 0x6d, 0x72, /* .-urn:mr */
	0x74, 0x2d, 0x56, 0x69, 0x72, 0x74, 0x75, 0x61, /* t-Virtua */
	0x6c, 0x42, 0x6f, 0x78, 0x3a, 0x55, 0x6e, 0x69, /* lBox:Uni */
	0x66, 0x69, 0x65, 0x64, 0x41, 0x75, 0x74, 0x6f, /* fiedAuto */
	0x6d, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x3a, 0x55, /* mation:U */
	0x61, 0x45, 0x78, 0x70, 0x65, 0x72, 0x74, 0x82, /* aExpert. */
	0x0e, 0x6d, 0x72, 0x74, 0x2d, 0x56, 0x69, 0x72, /* .mrt-Vir */
	0x74, 0x75, 0x61, 0x6c, 0x42, 0x6f, 0x78, 0x87, /* tualBox. */
	0x04, 0xc0, 0xa8, 0x02, 0x73, 0x30, 0x0d, 0x06, /* ....s0.. */
	0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, /* .*.H.... */
	0x01, 0x05, 0x05, 0x00, 0x03, 0x81, 0x81, 0x00, /* ........ */
	0x77, 0x2c, 0x9c, 0x23, 0x60, 0x13, 0x3f, 0xa5, /* w,.#`.?. */
	0xc8, 0xb3, 0x20, 0x27, 0x64, 0xda, 0x7f, 0xaa, /* .. 'd... */
	0xc5, 0x86, 0xfa, 0xd7, 0x24, 0x2e, 0xbe, 0xa0, /* ....$... */
	0xfc, 0x49, 0x8f, 0xc0, 0xef, 0xfb, 0x9a, 0xe6, /* .I...... */
	0x50, 0xe6, 0xb3, 0x53, 0x91, 0x91, 0x89, 0xd3, /* P..S.... */
	0x5a, 0xa5, 0xc9, 0x9c, 0xf6, 0x7b, 0x8f, 0x93, /* Z....{.. */
	0xb4, 0x98, 0xc3, 0x92, 0x26, 0x49, 0x8a, 0x96, /* ....&I.. */
	0x6e, 0x8f, 0xf5, 0x93, 0x48, 0x90, 0x9e, 0x7e, /* n...H..~ */
	0x1d, 0xad, 0x63, 0xbb, 0x5e, 0x1c, 0x0c, 0x86, /* ..c.^... */
	0x2d, 0xce, 0xe2, 0xe1, 0x87, 0x8d, 0x4c, 0x4b, /* -.....LK */
	0x89, 0x24, 0x77, 0xff, 0x62, 0x95, 0xf7, 0xec, /* .$w.b... */
	0x16, 0x7c, 0x8a, 0x1e, 0x4d, 0x89, 0xcb, 0x3d, /* .|..M..= */
	0xc8, 0xc0, 0x7c, 0x12, 0x5a, 0x29, 0xf2, 0xe7, /* ..|.Z).. */
	0x68, 0xf9, 0xb9, 0x85, 0xe5, 0xc0, 0x46, 0xac, /* h.....F. */
	0x89, 0xdb, 0xd0, 0x87, 0xaa, 0xa1, 0x7a, 0x73, /* ......zs */
	0x71, 0xcc, 0x8e, 0x01, 0x80, 0xf3, 0x07, 0x70, /* q......p */
	0x00, 0x00, 0x00, 0x00, 0x80, 0x4f, 0x32, 0x41, /* .....O2A */
	0xff, 0xff, 0xff, 0xff                          /* .... */
};

START_TEST(emptyIndicationShallYieldNoResponse) {
	// given
	UA_Int32 handle = stackTestFixture_create(responseMsg, closerCallback);
	UA_ByteString message = { -1, (UA_Byte *)UA_NULL };

	// when
	indicateMsg(handle, &message);

	// then
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.length, 0);

	// finally
	stackTestFixture_delete(handle);
}
END_TEST

START_TEST(validHELIndicationShallYieldACKResponse) {
	// given
	UA_Int32 handle = stackTestFixture_create(responseMsg, closerCallback);
	UA_ByteString message_001 = { sizeof(pkt_HEL), pkt_HEL };

	// when
	indicateMsg(handle, &message_001);

	// then
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.length, 28);
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[0], 'A');
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[1], 'C');
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[2], 'K');

	// finally
	stackTestFixture_delete(handle);
}
END_TEST

START_TEST(validOpeningSequenceShallCreateChannel) {
	// given
	UA_Int32 handle = stackTestFixture_create(responseMsg,closerCallback);

	UA_ByteString message_001 = { sizeof(pkt_HEL), pkt_HEL };
	UA_ByteString message_002 = { sizeof(pkt_OPN), pkt_OPN };
	UA_Int32 connectionState;
	// when
	indicateMsg(handle, &message_001);
	indicateMsg(handle, &message_002);
	UA_TL_Connection_getState(stackTestFixture_getFixture(handle)->connection, &connectionState);
	// then
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[0], 'O');
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[1], 'P');
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[2], 'N');
	ck_assert_int_eq(connectionState, CONNECTIONSTATE_ESTABLISHED);

	// finally
	stackTestFixture_delete(handle);
}
END_TEST

START_TEST(validOpeningCloseSequence) {
	// given
	UA_Int32 handle = stackTestFixture_create(responseMsg,closerCallback);

	UA_ByteString message_001 = { sizeof(pkt_HEL), pkt_HEL };
	UA_ByteString message_002 = { sizeof(pkt_OPN), pkt_OPN };
	UA_ByteString message_003 = { sizeof(pkt_CLO), pkt_CLO };
	UA_Int32 connectionState;
	// when
	indicateMsg(handle, &message_001);
	indicateMsg(handle, &message_002);
	indicateMsg(handle, &message_003);
	UA_TL_Connection_getState(stackTestFixture_getFixture(handle)->connection, &connectionState);
	// then
	ck_assert_int_eq(connectionState, CONNECTIONSTATE_CLOSE);

	// finally
	stackTestFixture_delete(handle);
}
END_TEST

START_TEST(validCreateSessionShallCreateSession) {
	// given
	UA_Int32 handle = stackTestFixture_create(responseMsg,closerCallback);
	SL_Channel *channel;
	UA_ByteString message_001 = { sizeof(pkt_HEL), pkt_HEL };
	UA_ByteString message_002 = { sizeof(pkt_OPN), pkt_OPN };
	UA_ByteString message_003 = { sizeof(pkt_MSG_CreateSession), pkt_MSG_CreateSession };

	// when
	indicateMsg(handle, &message_001);
	indicateMsg(handle, &message_002);
	indicateMsg(handle, &message_003);

	// then
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[0], 'M');
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[1], 'S');
	ck_assert_int_eq(stackTestFixture_getFixture(handle)->respMsg.data[2], 'G');


	SL_ChannelManager_getChannel(25,&channel);
	ck_assert_ptr_ne(channel,UA_NULL);



	// finally
	SL_ChannelManager_removeChannel(25);
	stackTestFixture_delete(handle);
}
END_TEST

START_TEST(UA_TcpMessageHeader_copyShallWorkOnInputExample) {
	// given
	UA_TcpMessageHeader src;
	UA_TcpMessageHeader_init(&src);
	src.isFinal = 2;
	src.messageSize = 43;
	src.messageType = UA_MESSAGETYPE_MSG;
	const UA_TcpMessageHeader srcConst = src;

	UA_TcpMessageHeader dst;
	UA_Int32 ret;

	// when
	ret = UA_TcpMessageHeader_copy(&srcConst, &dst);
	// then
	ck_assert_int_eq(ret, UA_SUCCESS);
	ck_assert_int_eq(UA_MESSAGETYPE_MSG, dst.messageType);
	ck_assert_int_eq(43, dst.messageSize);
	ck_assert_int_eq(2, dst.isFinal);
}
END_TEST

START_TEST(UA_AsymmetricAlgorithmSecurityHeader_copyShallWorkOnInputExample) {
	// given
	UA_AsymmetricAlgorithmSecurityHeader src;
	UA_AsymmetricAlgorithmSecurityHeader_init(&src);
	src.receiverCertificateThumbprint = (UA_String){10, (UA_Byte*)"thumbprint"};
	src.securityPolicyUri = (UA_String){6, (UA_Byte*)"policy"};
	src.senderCertificate = (UA_String){8, (UA_Byte*)"tEsT123!"};

	const UA_AsymmetricAlgorithmSecurityHeader srcConst = src;

	UA_AsymmetricAlgorithmSecurityHeader dst;
	UA_Int32 ret;

	// when
	ret = UA_AsymmetricAlgorithmSecurityHeader_copy(&srcConst, &dst);
	// then
	ck_assert_int_eq(ret, UA_SUCCESS);
	ck_assert_int_eq('m', dst.receiverCertificateThumbprint.data[3]);
	ck_assert_int_eq(10, dst.receiverCertificateThumbprint.length);
	ck_assert_int_eq('o', dst.securityPolicyUri.data[1]);
	ck_assert_int_eq(6, dst.securityPolicyUri.length);
	ck_assert_int_eq('t', dst.senderCertificate.data[0]);
	ck_assert_int_eq(8, dst.senderCertificate.length);

}
END_TEST

START_TEST(UA_SecureConversationMessageHeader_copyShallWorkOnInputExample) {
	// given
	UA_SecureConversationMessageHeader src;
	UA_SecureConversationMessageHeader_init(&src);
	src.secureChannelId = 84;
	UA_TcpMessageHeader srcHeader;
	UA_TcpMessageHeader_init(&srcHeader);
	srcHeader.isFinal = 4;
	srcHeader.messageSize = 765;
	srcHeader.messageType = UA_MESSAGETYPE_CLO;
	src.messageHeader = srcHeader;

	const UA_SecureConversationMessageHeader srcConst = src;

	UA_SecureConversationMessageHeader dst;
	UA_Int32 ret;

	// when
	ret = UA_SecureConversationMessageHeader_copy(&srcConst, &dst);
	// then
	ck_assert_int_eq(ret, UA_SUCCESS);
	ck_assert_int_eq(84, dst.secureChannelId);
	ck_assert_int_eq(4, dst.messageHeader.isFinal);
	ck_assert_int_eq(765, dst.messageHeader.messageSize);
	ck_assert_int_eq(UA_MESSAGETYPE_CLO, dst.messageHeader.messageType);
}
END_TEST

START_TEST(UA_SequenceHeader_copyShallWorkOnInputExample) {
	// given
	UA_SequenceHeader src;
	UA_SequenceHeader_init(&src);
	src.requestId = 84;
	src.sequenceNumber = 1345;

	const UA_SequenceHeader srcConst = src;

	UA_SequenceHeader dst;
	UA_Int32 ret;

	// when
	ret = UA_SequenceHeader_copy(&srcConst, &dst);
	// then
	ck_assert_int_eq(ret, UA_SUCCESS);
	ck_assert_int_eq(84, dst.requestId);
	ck_assert_int_eq(1345, dst.sequenceNumber);
}
END_TEST

START_TEST(UA_SecureConversationMessageFooter_copyShallWorkOnInputExample) {
	// given
	UA_SecureConversationMessageFooter src;
	UA_SecureConversationMessageFooter_init(&src);
	UA_Byte srcByte[3] = {24, 57, 87};
	src.padding = srcByte;
	src.paddingSize = 3;
	src.signature = 5;

	const UA_SecureConversationMessageFooter srcConst = src;

	UA_SecureConversationMessageFooter dst;
	UA_Int32 ret;

	// when
	ret = UA_SecureConversationMessageFooter_copy(&srcConst, &dst);
	// then
	ck_assert_int_eq(ret, UA_SUCCESS);
	ck_assert_int_eq(5, dst.signature);
	ck_assert_int_eq(3, dst.paddingSize);
	ck_assert_int_eq(24, dst.padding[0]);
	ck_assert_int_eq(57, dst.padding[1]);
	ck_assert_int_eq(87, dst.padding[2]);
}
END_TEST

START_TEST(UA_SecureConversationMessageFooter_calcSizeBinaryShallWorkOnInputExample) {
	// given
	UA_SecureConversationMessageFooter src;
	UA_SecureConversationMessageFooter_init(&src);
	UA_Byte srcByte[3] = {24, 57, 87};
	src.padding = srcByte;
	src.paddingSize = 3;
	src.signature = 5;

	const UA_SecureConversationMessageFooter srcConst = src;

	UA_Int32 ret;

	// when
	ret = UA_SecureConversationMessageFooter_calcSizeBinary(&srcConst);
	// then
	ck_assert_int_eq(ret, 8);
}
END_TEST

START_TEST(UA_SecureConversationMessageFooter_encodeBinaryShallWorkOnInputExample) {
//	// given
//	UA_SecureConversationMessageFooter src = {3, (UA_Byte*)"447", 5};;
//
//	UA_Int32 ret;
//	UA_UInt32 offset = 0;
//	UA_ByteString dst = (UA_ByteString){15, (UA_Byte*)"123456789abcdef"};
//
//	// when
//	ret = UA_SecureConversationMessageFooter_encodeBinary(&src, &dst, &offset);
//	// then
//	ck_assert_int_eq(ret, UA_SUCCESS);
//	ck_assert_int_eq(dst.length, 8);
//	ck_assert_int_eq(dst.data[0], 0);
//	ck_assert_int_eq(dst.data[1], 0);
//	ck_assert_int_eq(dst.data[2], 0);
//	ck_assert_int_eq(dst.data[3], 3);
//	ck_assert_int_eq(dst.data[4], 24);
//	ck_assert_int_eq(dst.data[5], 57);
//	ck_assert_int_eq(dst.data[6], 87);
//	ck_assert_int_eq(dst.data[7], 6);
}
END_TEST

START_TEST(UA_SecureConversationMessageAbortBody_copyShallWorkOnInputExample) {
	// given
	UA_SecureConversationMessageAbortBody src;
	UA_SecureConversationMessageAbortBody_init(&src);
	src.error = 5478;
	src.reason = (UA_String){6, (UA_Byte*)"reAson"};

	const UA_SecureConversationMessageAbortBody srcConst = src;
	UA_SecureConversationMessageAbortBody dst;
	UA_Int32 ret;

	// when
	ret = UA_SecureConversationMessageAbortBody_copy(&srcConst, &dst);
	// then
	ck_assert_int_eq(ret, UA_SUCCESS);
	ck_assert_int_eq(5478, dst.error);
	ck_assert_int_eq('A', dst.reason.data[2]);
	ck_assert_int_eq(6, dst.reason.length);
}
END_TEST

Suite *testSuite() {
	Suite *s = suite_create("Stack Test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, emptyIndicationShallYieldNoResponse);
	tcase_add_test(tc_core, validHELIndicationShallYieldACKResponse);
	tcase_add_test(tc_core, validOpeningSequenceShallCreateChannel);
	tcase_add_test(tc_core, validOpeningCloseSequence);
	tcase_add_test(tc_core, validCreateSessionShallCreateSession);
	suite_add_tcase(s, tc_core);

	TCase *tc_transport = tcase_create("Transport");
	tcase_add_test(tc_transport, UA_TcpMessageHeader_copyShallWorkOnInputExample);
	tcase_add_test(tc_transport, UA_AsymmetricAlgorithmSecurityHeader_copyShallWorkOnInputExample);
	tcase_add_test(tc_transport, UA_SecureConversationMessageHeader_copyShallWorkOnInputExample);
	tcase_add_test(tc_transport, UA_SequenceHeader_copyShallWorkOnInputExample);
	tcase_add_test(tc_transport, UA_SecureConversationMessageFooter_copyShallWorkOnInputExample);
	tcase_add_test(tc_transport, UA_SecureConversationMessageFooter_calcSizeBinaryShallWorkOnInputExample);
	tcase_add_test(tc_transport, UA_SecureConversationMessageFooter_encodeBinaryShallWorkOnInputExample);
	tcase_add_test(tc_transport, UA_SecureConversationMessageAbortBody_copyShallWorkOnInputExample);
	suite_add_tcase(s, tc_transport);
	return s;
}

int main(void) {
	int      number_failed = 0;
	Suite   *s;
	SRunner *sr;

	s  = testSuite();
	sr = srunner_create(s);
	srunner_set_fork_status (sr, CK_NOFORK);
	srunner_run_all(sr, CK_NOFORK);
	//srunner_run_all(sr, CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
