/*
 C ECHO client example using sockets

 This is an example client for internal benchmarks. It works, but is not ready
 for serious use. We do not really check any of the returns from the server.
 */
#include <stdio.h> //printf
#include <string.h> //strlen
#ifndef _WIN32
#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h> // for close
#else
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#endif
#include <stdlib.h> // pulls in declaration of malloc, free
#include "ua_transport_generated.h"
#include "ua_namespace_0.h"
#include "ua_util.h"

enum UA_AttributeId {
    UA_ATTRIBUTEID_NODEID                  = 1,
    UA_ATTRIBUTEID_NODECLASS               = 2,
    UA_ATTRIBUTEID_BROWSENAME              = 3,
    UA_ATTRIBUTEID_DISPLAYNAME             = 4,
    UA_ATTRIBUTEID_DESCRIPTION             = 5,
    UA_ATTRIBUTEID_WRITEMASK               = 6,
    UA_ATTRIBUTEID_USERWRITEMASK           = 7,
    UA_ATTRIBUTEID_ISABSTRACT              = 8,
    UA_ATTRIBUTEID_SYMMETRIC               = 9,
    UA_ATTRIBUTEID_INVERSENAME             = 10,
    UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
    UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
    UA_ATTRIBUTEID_VALUE                   = 13,
    UA_ATTRIBUTEID_DATATYPE                = 14,
    UA_ATTRIBUTEID_VALUERANK               = 15,
    UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
    UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
    UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
    UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
    UA_ATTRIBUTEID_HISTORIZING             = 20,
    UA_ATTRIBUTEID_EXECUTABLE              = 21,
    UA_ATTRIBUTEID_USEREXECUTABLE          = 22
};

UA_Int32 sendHello(UA_Int32 sock, UA_String *endpointURL) {

	UA_TcpMessageHeader messageHeader;
	messageHeader.isFinal = 'F';
	messageHeader.messageType = UA_MESSAGETYPE_HEL;

	UA_TcpHelloMessage hello;
	UA_String_copy(endpointURL, &hello.endpointUrl);
	hello.maxChunkCount = 1;
	hello.maxMessageSize = 16777216;
	hello.protocolVersion = 0;
	hello.receiveBufferSize = 65536;
	hello.sendBufferSize = 65536;

	messageHeader.messageSize = UA_TcpHelloMessage_calcSizeBinary((UA_TcpHelloMessage const*) &hello) +
                                UA_TcpMessageHeader_calcSizeBinary((UA_TcpMessageHeader const*) &messageHeader);
	UA_ByteString message;
	UA_ByteString_newMembers(&message, messageHeader.messageSize);

	UA_UInt32 offset = 0;
	UA_TcpMessageHeader_encodeBinary((UA_TcpMessageHeader const*) &messageHeader, &message, &offset);
	UA_TcpHelloMessage_encodeBinary((UA_TcpHelloMessage const*) &hello, &message, &offset);

	UA_Int32 sendret = send(sock, message.data, offset, 0);

	UA_ByteString_deleteMembers(&message);
	free(hello.endpointUrl.data);
	if (sendret < 0)
		return 1;
	return 0;
}

int sendOpenSecureChannel(UA_Int32 sock) {
	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_OPN;

	UA_UInt32 secureChannelId = 0;
	UA_String securityPolicy;
	UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None", &securityPolicy);

	UA_String senderCert;
	senderCert.data = UA_NULL;
	senderCert.length = -1;

	UA_String receiverCertThumb;
	receiverCertThumb.data = UA_NULL;
	receiverCertThumb.length = -1;

	UA_UInt32 sequenceNumber = 51;

	UA_UInt32 requestId = 1;

	UA_NodeId type;
	type.identifier.numeric = 446; // id of opensecurechannelrequest
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	UA_OpenSecureChannelRequest opnSecRq;
	UA_OpenSecureChannelRequest_init(&opnSecRq);
	opnSecRq.requestHeader.timestamp = UA_DateTime_now();
	UA_ByteString_newMembers(&opnSecRq.clientNonce, 1);
	opnSecRq.clientNonce.data[0] = 0;
	opnSecRq.clientProtocolVersion = 0;
	opnSecRq.requestedLifetime = 30000;
	opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;
	opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
	opnSecRq.requestHeader.authenticationToken.identifier.numeric = 10;
	opnSecRq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	opnSecRq.requestHeader.authenticationToken.namespaceIndex = 10;

	msghdr.messageSize = 135; // todo: compute the message size from the actual content

	UA_ByteString message;
	UA_ByteString_newMembers(&message, 1000);
	UA_UInt32 offset = 0;
	UA_TcpMessageHeader_encodeBinary(&msghdr, &message, &offset);
	UA_UInt32_encodeBinary(&secureChannelId, &message, &offset);
	UA_String_encodeBinary(&securityPolicy, &message, &offset);
	UA_String_encodeBinary(&senderCert, &message, &offset);
	UA_String_encodeBinary(&receiverCertThumb, &message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, &message, &offset);
	UA_UInt32_encodeBinary(&requestId, &message, &offset);
	UA_NodeId_encodeBinary(&type, &message, &offset);
	UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &message, &offset);

    UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);
	UA_String_deleteMembers(&securityPolicy);

	UA_Int32 sendret = send(sock, message.data, offset, 0);
	UA_ByteString_deleteMembers(&message);
	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;
}

UA_Int32 sendCreateSession(UA_Int32 sock, UA_UInt32 channelId, UA_UInt32 tokenId, UA_UInt32 sequenceNumber,
                           UA_UInt32 requestId, UA_String *endpointUrl) {
    UA_ByteString message;
	UA_ByteString_newMembers(&message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	UA_UInt32 offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_MSG;

	UA_NodeId type;
	type.identifier.numeric = 461;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	UA_CreateSessionRequest rq;
    UA_CreateSessionRequest_init(&rq);
	rq.requestHeader.requestHandle = 1;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.authenticationToken.identifier.numeric = 10;
	rq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	rq.requestHeader.authenticationToken.namespaceIndex = 10;
	UA_String_copy(endpointUrl, &rq.endpointUrl);
	UA_String_copycstring("mysession", &rq.sessionName);
	UA_String_copycstring("abcd", &rq.clientCertificate);
	UA_ByteString_newMembers(&rq.clientNonce, 1);
	rq.clientNonce.data[0] = 0;
	rq.requestedSessionTimeout = 1200000;
	rq.maxResponseMessageSize = UA_INT32_MAX;

	msghdr.messageSize = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) +
                         UA_CreateSessionRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr, &message, &offset);
	UA_UInt32_encodeBinary(&tmpChannelId, &message, &offset);
	UA_UInt32_encodeBinary(&tokenId, &message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, &message, &offset);
	UA_UInt32_encodeBinary(&requestId, &message, &offset);
	UA_NodeId_encodeBinary(&type, &message, &offset);
	UA_CreateSessionRequest_encodeBinary(&rq, &message, &offset);

	UA_Int32 sendret = send(sock, message.data, offset, 0);
	UA_ByteString_deleteMembers(&message);
	UA_CreateSessionRequest_deleteMembers(&rq);
	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;
}
UA_Int32 sendActivateSession(UA_Int32 sock, UA_UInt32 channelId, UA_UInt32 tokenId, UA_UInt32 sequenceNumber,
                             UA_UInt32 requestId, UA_NodeId authenticationToken) {
	UA_ByteString *message = UA_ByteString_new();
	UA_ByteString_newMembers(message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	UA_UInt32 offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_MSG;
	msghdr.messageSize = 86;

	UA_NodeId type;
	type.identifier.numeric = 467;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	UA_ActivateSessionRequest rq;
	UA_ActivateSessionRequest_init(&rq);
	rq.requestHeader.requestHandle = 2;
	rq.requestHeader.authenticationToken = authenticationToken;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.timeoutHint = 10000;
    
	msghdr.messageSize  = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) +
                          UA_ActivateSessionRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr, message, &offset);
	UA_UInt32_encodeBinary(&tmpChannelId, message, &offset);
	UA_UInt32_encodeBinary(&tokenId, message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, message, &offset);
	UA_UInt32_encodeBinary(&requestId, message, &offset);
	UA_NodeId_encodeBinary(&type, message, &offset);
	UA_ActivateSessionRequest_encodeBinary(&rq, message, &offset);

	UA_Int32 sendret = send(sock, message->data, offset, 0);
	UA_ByteString_delete(message);

	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;

}

UA_Int64 sendReadRequest(UA_Int32 sock, UA_UInt32 channelId, UA_UInt32 tokenId, UA_UInt32 sequenceNumber, UA_UInt32 requestId,
                         UA_NodeId authenticationToken, UA_Int32 nodeIds_size,UA_NodeId* nodeIds) {
	UA_ByteString *message = UA_ByteString_new();
	UA_ByteString_newMembers(message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	UA_UInt32 offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_MSG;

	UA_NodeId type;
	type.identifier.numeric = 631;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	UA_ReadRequest rq;
	UA_ReadRequest_init(&rq);
	rq.maxAge = 0;
	UA_Array_new((void **)&rq.nodesToRead, nodeIds_size, &UA_TYPES[UA_READVALUEID]);
	rq.nodesToReadSize = nodeIds_size;
	for(UA_Int32 i=0;i<nodeIds_size;i++) {
		UA_ReadValueId_init(&(rq.nodesToRead[i]));
		rq.nodesToRead[i].attributeId = UA_ATTRIBUTEID_VALUE;
		UA_NodeId_init(&(rq.nodesToRead[i].nodeId));
		rq.nodesToRead[i].nodeId = nodeIds[i];
		UA_QualifiedName_init(&(rq.nodesToRead[0].dataEncoding));
	}
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.authenticationToken = authenticationToken;
	rq.timestampsToReturn = 0x03;
	rq.requestHeader.requestHandle = 1 + requestId;

	msghdr.messageSize = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) +
                         UA_ReadRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr,message,&offset);
	UA_UInt32_encodeBinary(&tmpChannelId, message, &offset);
	UA_UInt32_encodeBinary(&tokenId, message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, message, &offset);
	UA_UInt32_encodeBinary(&requestId, message, &offset);
	UA_NodeId_encodeBinary(&type,message,&offset);
	UA_ReadRequest_encodeBinary(&rq, message, &offset);

	UA_DateTime tic = UA_DateTime_now();
	UA_Int32 sendret = send(sock, message->data, offset, 0);
	UA_Array_delete(rq.nodesToRead,nodeIds_size,&UA_TYPES[UA_READVALUEID]);
	UA_ByteString_delete(message);

	if (sendret < 0) {
		printf("send readrequest failed");
		return 1;
	}
	return tic;
}

#define CHECK_STATUS(__op) \
if(__op != UA_STATUSCODE_GOOD) { printf("failed at " #__op "\n"); return; }

static void processMessage(const UA_ByteString *msg, UA_UInt32 *pos) {

    UA_String_printx_hex("msg=", msg);

    // 1) Read in the securechannel
    UA_UInt32 secureChannelId;
    CHECK_STATUS(UA_UInt32_decodeBinary(msg, pos, &secureChannelId));
/*
    UA_SecureChannel *channel;
    UA_SecureChannelManager_get(&server->secureChannelManager, secureChannelId, &channel);
    */

    // 2) Read the security header
    UA_UInt32 tokenId;
    CHECK_STATUS(UA_UInt32_decodeBinary(msg, pos, &tokenId));
    UA_SequenceHeader sequenceHeader;
    CHECK_STATUS(UA_SequenceHeader_decodeBinary(msg, pos, &sequenceHeader));
    fprintf(stdout, "pos=%i seqHdr=", *pos);
    UA_SequenceHeader_print(&sequenceHeader, stdout);
    fprintf(stdout, "\n");

    /*
    channel->sequenceNumber = sequenceHeader.sequenceNumber;
    channel->requestId = sequenceHeader.requestId;
    */
    // todo
    //UA_SecureChannel_checkSequenceNumber(channel,sequenceHeader.sequenceNumber);
    //UA_SecureChannel_checkRequestId(channel,sequenceHeader.requestId);

    // 3) Read the nodeid of the request
    UA_ExpandedNodeId requestType;
    UA_ExpandedNodeId_init(&requestType);
    CHECK_STATUS(UA_ExpandedNodeId_decodeBinary(msg, pos, &requestType));
    if (requestType.nodeId.identifierType != UA_NODEIDTYPE_NUMERIC) {
        UA_ExpandedNodeId_deleteMembers(&requestType); // if the nodeidtype is numeric, we do not have to free anything
        return;
    }

    fprintf(stdout, "pos=%i requestType=", *pos);
    UA_ExpandedNodeId_print(&requestType, stdout);
    fprintf(stdout, "\n");

    UA_ReadResponse rr;
    UA_ReadResponse_init(&rr);
    CHECK_STATUS(UA_ReadResponse_decodeBinary(msg, pos, &rr));

    fprintf(stdout, "pos=%i respHdr=", *pos);
    UA_ResponseHeader_print(&rr.responseHeader, stdout);
    fprintf(stdout, "\n");

    fprintf(stdout, "results=");
    UA_Array_print(rr.results, rr.resultsSize, &UA_TYPES[UA_DATAVALUE], stdout);
    fprintf(stdout, "\n");
    for(int n = 0; n < rr.resultsSize; n++)
    {
        UA_Variant * v = &rr.results[n].value;

        UA_UInt32 * d = v->storage.data.dataPtr;
        fprintf(stdout, "result_data[%i]=%i:%p, %x\n", n, v->storage.data.arrayLength, v->storage.data.dataPtr, (d != 0)?*d:0);


        fprintf(stdout, "result[%i]=", n);
        //UA_DataValue_print(&rr.results[n], stdout);
        UA_Variant_print(v, stdout);
        fprintf(stdout, "\n");
        /*
        printf("result[%i]=%i (status %i)\n", n, 
            rr.results[n].value.storageType, 
            rr.results[n].value.storageType,
            rr.results[n].status);
            */
    }
    for(int n = 0; n < rr.diagnosticInfosSize; n++)
    {
        fprintf(stdout, "diag[%i]=", n);
        UA_DiagnosticInfo_print(&rr.diagnosticInfos[n], stdout);
        fprintf(stdout, "\n");

    }
}


void processBinaryMessage(const UA_ByteString *msg)
{
    UA_Int32  retval = UA_STATUSCODE_GOOD;
    UA_UInt32 pos = 0;
    UA_TcpMessageHeader tcpMessageHeader;

    // todo: test how far pos advanced must be equal to what is said in the messageheader
    do {
        retval = UA_TcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader);
        UA_UInt32 targetpos = pos - 8 + tcpMessageHeader.messageSize;
        if (retval == UA_STATUSCODE_GOOD) {
            // none of the process-functions returns an error its all contained inside.
            switch (tcpMessageHeader.messageType) {
            case UA_MESSAGETYPE_HEL:
                //processHello(connection, msg, &pos);
                break;

            case UA_MESSAGETYPE_OPN:
                //processOpen(connection, server, msg, &pos);
                break;

            case UA_MESSAGETYPE_MSG:
                //no break
                // if this fails, the connection is closed (no break on the case)
                printf("got msg\n");
                processMessage(msg, &pos);
#if 0
                if (connection->state == UA_CONNECTION_ESTABLISHED &&
                    connection->channel != UA_NULL) {
                    processMessage(connection, server, msg, &pos);
                    break;
                }
#endif // 0
                break;

            case UA_MESSAGETYPE_CLO:
                /*
                connection->state = UA_CONNECTION_CLOSING;
                processClose(connection, server, msg, &pos);
                connection->close(connection->callbackHandle);
                */
                return;
            }
            UA_TcpMessageHeader_deleteMembers(&tcpMessageHeader);
        }
        else {
            printf("TL_Process - ERROR: decoding of header failed \n");
            //connection->state = UA_CONNECTION_CLOSING;
            //processClose(connection, server, msg, &pos);
            //connection->close(connection->callbackHandle);
        }
        // todo: more than one message at once..
        if (pos != targetpos) {
            printf("The message size was not as announced or an error occurred while processing, skipping to the end of the message.\n");
            pos = targetpos;
        }
    } while (msg->length > (UA_Int32)pos);
}

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in server;

	UA_ByteString reply;
	UA_ByteString_newMembers(&reply, 65536);

#ifdef _WIN32
    int iResult;
    WSADATA wsaData;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#endif

	//start parameters
	if(argc < 7) {
		printf("1st parameter: number of nodes to read \n");
		printf("2nd parameter: number of read-tries \n");
		printf("3rd parameter: name of the file to save measurement data \n");
		printf("4th parameter: 1 = read same node, 0 = read different nodes \n");
		printf("5th parameter: ip adress \n");
		printf("6th parameter: port \n");
		return 0;
	}

	UA_UInt32 nodesToReadSize;
	UA_UInt32 tries;
	UA_Boolean alwaysSameNode;
	if(argv[1] == UA_NULL)
		nodesToReadSize = 20;
	else
		nodesToReadSize = atoi(argv[1]);

	if(argv[2] == UA_NULL)
		tries= 2;
	else
		tries = (UA_UInt32) atoi(argv[2]);

	if(atoi(argv[4]) != 0)
		alwaysSameNode = UA_TRUE;
	else
		alwaysSameNode = UA_FALSE;

	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		printf("Could not create socket");
        return 1;
    }
	server.sin_addr.s_addr = inet_addr(argv[5]);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[6]));

    //Connect to remote server
	if(connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}

	UA_String *endpointUrl = UA_String_new();
	UA_String_copycstring("opc.tcp://blabla.com:1234", endpointUrl);
	sendHello(sock, endpointUrl);
	int received = recv(sock, reply.data, reply.length, 0);
	sendOpenSecureChannel(sock);
	received = recv(sock, reply.data, reply.length, 0);

	UA_UInt32 recvOffset = 0;
	UA_TcpMessageHeader msghdr;
	UA_TcpMessageHeader_decodeBinary(&reply, &recvOffset, &msghdr);
	UA_UInt32 secureChannelId;
	UA_AsymmetricAlgorithmSecurityHeader asymHeader;
	UA_SequenceHeader seqHeader;
	UA_NodeId rspType;
	UA_OpenSecureChannelResponse openSecChannelRsp;
	UA_UInt32_decodeBinary(&reply, &recvOffset, &secureChannelId);
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply,&recvOffset,&asymHeader);
	UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
	UA_SequenceHeader_decodeBinary(&reply,&recvOffset,&seqHeader);
	UA_NodeId_decodeBinary(&reply,&recvOffset,&rspType);
	UA_OpenSecureChannelResponse_decodeBinary(&reply,&recvOffset,&openSecChannelRsp);

	sendCreateSession(sock, secureChannelId, openSecChannelRsp.securityToken.tokenId, 52, 2, endpointUrl);
	received = recv(sock, reply.data, reply.length, 0);

	UA_NodeId messageType;
	recvOffset = 24;
	UA_NodeId_decodeBinary(&reply,&recvOffset,&messageType);
	UA_CreateSessionResponse createSessionResponse;
	UA_CreateSessionResponse_decodeBinary(&reply,&recvOffset,&createSessionResponse);

	sendActivateSession(sock, secureChannelId, openSecChannelRsp.securityToken.tokenId, 53, 3,
                        createSessionResponse.authenticationToken);
	received = recv(sock, reply.data, reply.length, 0);

    UA_NodeId *nodesToRead;
    UA_Array_new((void**)&nodesToRead,nodesToReadSize,&UA_TYPES[UA_NODEID]);

	for(UA_UInt32 i = 0; i<nodesToReadSize; i++) {
        if(alwaysSameNode)
            nodesToRead[i].identifier.numeric = 2250; //ask always the same node
        else
            nodesToRead[i].identifier.numeric = 2267 + i;
		nodesToRead[i].identifierType = UA_NODEIDTYPE_NUMERIC;
		nodesToRead[i].namespaceIndex = 0;
	}

	{
        UA_DataValue dv;
        UA_DataValue_init(&dv);
        fprintf(stdout, "dv=");
        UA_DataValue_print(&dv, stdout);
        fprintf(stdout, "\n");

        UA_Int32 dummy = 42;
        UA_Variant_copySetValue(&dv.value, &UA_TYPES[UA_INT32], &dummy);

        UA_Int32 dummy2 = *(UA_Int32 *)dv.value.storage.data.dataPtr;

        fprintf(stdout, "dv=%i\n", dummy2);

        fprintf(stdout, "dv=");
        UA_DataValue_print(&dv, stdout);
        fprintf(stdout, "\n");
    }

	UA_DateTime tic, toc;
	UA_Double *timeDiffs;

	UA_Array_new((void**)&timeDiffs,tries,&UA_TYPES[UA_DOUBLE]);
	UA_Double sum = 0;

	for(UA_UInt32 i = 0; i < tries; i++) {
		tic = sendReadRequest(sock, secureChannelId, openSecChannelRsp.securityToken.tokenId, 54+i, 4+i,
                              createSessionResponse.authenticationToken,nodesToReadSize,nodesToRead);
		received = recv(sock, reply.data, 2000, 0);
        reply.length = received;

        processBinaryMessage(&reply);

		toc = UA_DateTime_now() - tic;
		timeDiffs[i] = (UA_Double)toc/(UA_Double)1e4;
		sum = sum + timeDiffs[i];
	}

	UA_Double mean = sum / tries;
	printf("mean time for handling request: %16.10f ms \n",mean);

	if(received>0)
		printf("%i",received); // dummy

	//save to file
	char data[100];
	const char * flag = "a";
	FILE* fHandle =  fopen(argv[3], flag);
    if(fHandle)
    {
	    //header

	    UA_Int32 bytesToWrite = sprintf(data, "measurement %s in ms, nodesToRead %d \n", argv[3], nodesToReadSize);
	    fwrite(data,1,bytesToWrite,fHandle);
	    for(UA_UInt32 i=0;i<tries;i++) {
		    bytesToWrite = sprintf(data,"%16.10f \n",timeDiffs[i]);
		    fwrite(data,1,bytesToWrite,fHandle);
	    }
	    fclose(fHandle);
    }

    UA_OpenSecureChannelResponse_deleteMembers(&openSecChannelRsp);
	UA_String_delete(endpointUrl);
	UA_String_deleteMembers(&reply);
	UA_Array_delete(nodesToRead,nodesToReadSize,&UA_TYPES[UA_NODEID]);
    //UA_free(timeDiffs);
	UA_CreateSessionResponse_deleteMembers(&createSessionResponse);

#ifdef _WIN32
    closesocket(sock);
#else
	close(sock);
#endif

	return 0;
}
