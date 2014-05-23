/*
 ============================================================================
 Name        : check_stack.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "opcua.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "check.h"

#define MAXMSG 512
#define BUFFER_SIZE 8192

typedef struct stackTestFixture {
	UA_Byte respMsgBuffer[BUFFER_SIZE];
	UA_ByteString respMsg;
	TL_Connection connection;
} stackTestFixture;

/** write message provided in the gather buffers to a tcp transport layer connection */
UA_Int32 responseMsg(struct TL_Connection const * c, UA_ByteString const * const * gather_buf, UA_UInt32 gather_len) {
	UA_Int32 retval = UA_SUCCESS;
	UA_UInt32 total_len = 0;

	stackTestFixture* fixture = (stackTestFixture*) c->connectionHandle;

	for (UA_UInt32 i=0; i<gather_len && retval == UA_SUCCESS; ++i) {
		if (total_len + gather_buf[i]->length < BUFFER_SIZE) {
			memcpy(&(fixture->respMsg.data[total_len]),gather_buf[i]->data,gather_buf[i]->length);
			total_len += gather_buf[i]->length;
		} else {
			retval = UA_ERR_NO_MEMORY;
		}
	}
	fixture->respMsg.length = total_len;
	return UA_SUCCESS;
}

stackTestFixture* createFixture() {
	stackTestFixture* fixture;
	UA_alloc((void**)&fixture, sizeof(stackTestFixture));
	fixture->respMsg.data = fixture->respMsgBuffer;
	fixture->respMsg.length = 0;
	fixture->connection.connectionState = CONNECTIONSTATE_CLOSED;
	fixture->connection.writerCallback = (TL_Writer) responseMsg;
	fixture->connection.localConf.maxChunkCount = 1;
	fixture->connection.localConf.maxMessageSize = BUFFER_SIZE;
	fixture->connection.localConf.protocolVersion = 0;
	fixture->connection.localConf.recvBufferSize = BUFFER_SIZE;
	fixture->connection.localConf.recvBufferSize = BUFFER_SIZE;
	// FIXME: this works only for architectures where sizeof(UA_Int32) == sizeof(void*)
	fixture->connection.connectionHandle = (UA_Int32) fixture;

	return fixture;
}



void indicateMsg(stackTestFixture* f, UA_ByteString *slMessage) {
	TL_Process(&(f->connection), slMessage);
}

START_TEST(emptyIndicationShallYieldNoResponse)
{
	// given
	stackTestFixture* fixture = createFixture();
	UA_ByteString message = { -1, (UA_Byte*) UA_NULL };
	// when
	indicateMsg(fixture, &message);
	// then
	ck_assert_int_eq(fixture->respMsg.length,0);
	// finally
	UA_free(fixture);
}
END_TEST


//START_TEST(validHELIndicationShallYieldACKResponse)
//{
//	// given
//	stackTestFixture* fixture = createFixture();
//	// FIXME: should be a valid HEL-Msg
//	UA_ByteString message = { -1, (UA_Byte*) UA_NULL };
//	// when
//	indicateMsg(fixture, &message);
//	// then
//	// FIXME: length of ACK-Message
//	ck_assert_int_eq(fixture->respMsg.length,22);
//	// finally
//	UA_free(fixture);
//}
//END_TEST

Suite* testSuite()
{
	Suite *s = suite_create("Stack Test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, emptyIndicationShallYieldNoResponse);
//	tcase_add_test(tc_core, validHELIndicationShallYieldACKResponse);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed = 0;

	Suite *s;
	SRunner *sr;

	s = testSuite();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


