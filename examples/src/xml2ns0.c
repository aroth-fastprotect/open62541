/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include "ua_xml.h"

// FIXME: most of the following code should be generated as a template from the ns0-defining xml-files
/** @brief server_application_memory is a flattened memory structure of the UAVariables in ns0 */
struct server_application_memory {
	UA_UtcTime startTime;
	UA_UtcTime currentTime;
	UA_ServerState state;
	UA_String productName;
	UA_String productUri;
	UA_String manufacturerName;
	UA_String softwareVersion;
	UA_String buildNumber;
	UA_String buildDate;
	UA_Int32 secondsTillShutdown;
	UA_String shutdownReason;
} server_application_memory;

char* productName = "xml2ns0";
char* productUri = "http://open62541.org/xml2ns0/";
char* manufacturerName = "open62541";
char* softwareVersion = "0.01";
char* buildNumber = "999" __DATE__ "-001" ;
char* buildDate = __DATE__;

#define SAM_ASSIGN_CSTRING(src) do { \
	server_application_memory.src.length = strlen(src)-1; \
	server_application_memory.src.data = (UA_Byte*) src; \
} while(0)

void* direct_access_strategy(void* arg) { return arg; }
UA_DateTime* setGetCurrentTime(UA_DateTime* arg) { *arg = UA_DateTime_now(); return arg; }

#define MAP_NODEID_TO_METHOD(ns,id,f,name) \
		Namespace_attachData(ns, id, (Namespace_access_strategy) f, &server_application_memory.name, UA_NULL)

#define MAP_NODEID_TO_ADDR(ns,id,name) \
		MAP_NODEID_TO_METHOD(ns,id, direct_access_strategy,name)

void server_application_memory_init(Namespace* ns) {
	// Initialize the strings
	SAM_ASSIGN_CSTRING(productName);
	SAM_ASSIGN_CSTRING(productUri);
	SAM_ASSIGN_CSTRING(manufacturerName);
	SAM_ASSIGN_CSTRING(manufacturerName);
	SAM_ASSIGN_CSTRING(softwareVersion);
	SAM_ASSIGN_CSTRING(buildNumber);
	SAM_ASSIGN_CSTRING(buildDate);

	// Attach memory
	MAP_NODEID_TO_ADDR(ns,2256,startTime); // this is the head of server status!
	MAP_NODEID_TO_ADDR(ns,2257,startTime);
	MAP_NODEID_TO_METHOD(ns,2258,setGetCurrentTime,currentTime);
	MAP_NODEID_TO_ADDR(ns,2259,state);
	MAP_NODEID_TO_ADDR(ns,2260,productName); // start of build Info
	MAP_NODEID_TO_ADDR(ns,2261,productName);
	MAP_NODEID_TO_ADDR(ns,2262,productUri);
	MAP_NODEID_TO_ADDR(ns,2263,manufacturerName);
	MAP_NODEID_TO_ADDR(ns,2264,softwareVersion);
	MAP_NODEID_TO_ADDR(ns,2265,buildNumber);
	MAP_NODEID_TO_ADDR(ns,2266,buildDate);
	MAP_NODEID_TO_ADDR(ns,2292,secondsTillShutdown);
	MAP_NODEID_TO_ADDR(ns,2293,shutdownReason);
}

UA_Int16 UA_NodeId_getNamespace(UA_NodeId* id) {
	return id->namespace;
}
// FIXME: to simple
UA_Int16 UA_NodeId_getIdentifier(UA_NodeId* id) {
	return id->identifier.numeric;
}
_Bool UA_NodeId_isBasicType(UA_NodeId* id) {
	return (UA_NodeId_getNamespace(id) == 0) && (UA_NodeId_getIdentifier(id) <= UA_DIAGNOSTICINFO_NS0);
}

UA_Int32 Namespace_getNumberOfComponents(Namespace const * ns, UA_NodeId const * id, UA_Int32* number) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Node const * node;
	if ((retval = Namespace_get(ns,id,&node,UA_NULL)) != UA_SUCCESS)
		return retval;
	if (node == UA_NULL)
		return UA_ERR_INVALID_VALUE;
	UA_Int32 i, n;
	for (i = 0, n = 0; i < node->referencesSize; i++ ) {
		if (node->references[i]->referenceTypeId.identifier.numeric == 47 && node->references[i]->isInverse != UA_TRUE) {
			n++;
		}
	}
	*number = n;
	return retval;
}

UA_Int32 Namespace_getComponent(Namespace const * ns, UA_NodeId const * id, UA_Int32 idx, UA_NodeId** result) {
	UA_Int32 retval = UA_SUCCESS;

	UA_Node const * node;
	if ((retval = Namespace_get(ns,id,&node,UA_NULL)) != UA_SUCCESS)
		return retval;

	UA_Int32 i, n;
	for (i = 0, n = 0; i < node->referencesSize; i++ ) {
		if (node->references[i]->referenceTypeId.identifier.numeric == 47 && node->references[i]->isInverse != UA_TRUE) {
			n++;
			if (n == idx) {
				*result = &(node->references[i]->targetId.nodeId);
				return retval;
			}
		}
	}
	return UA_ERR_INVALID_VALUE;
}


UA_Int32 UAX_NodeId_encodeBinary(Namespace const * ns, UA_NodeId* id, UA_Int32* pos, UA_ByteString *dst) {
	UA_Int32 i, retval = UA_SUCCESS;
	if (UA_NodeId_isBasicType(id)) {
		void* data;
		if ((retval = Namespace_getData(ns,id,&data,UA_NULL)) != UA_SUCCESS)
				return retval;
		UA_[UA_ns0ToVTableIndex(UA_NodeId_getIdentifier(id))].encodeBinary(data,pos,dst);
	} else {
		UA_Int32 nComp = 0;
		if ((retval = Namespace_getNumberOfComponents(ns,id,&nComp)) != UA_SUCCESS)
			return retval;
		for (i=0; i < nComp; i++) {
			UA_NodeId* comp;
			Namespace_getComponent(ns,id,i,&comp);
			UAX_NodeId_encodeBinary(ns,comp, pos, dst);
		}
	}
	return retval;
}

int main() {
	char buf[1024];
	int len; /* len is the number of bytes in the current bufferful of data */
	XML_Stack s;
	XML_Stack_init(&s, "ROOT");
	UA_NodeSet n;
	UA_NodeSet_init(&n);
	XML_Stack_addChildHandler(&s, "UANodeSet", strlen("UANodeSet"), (XML_decoder) UA_NodeSet_decodeXML, UA_INVALIDTYPE, &n);

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &s);
	XML_SetElementHandler(parser, XML_Stack_startElement, XML_Stack_endElement);
	XML_SetCharacterDataHandler(parser, XML_Stack_handleText);
	while ((len = read(0, buf, 1024)) > 0) {
		if (!XML_Parse(parser, buf, len, (len < 1024))) {
			return 1;
		}
	}
	XML_ParserFree(parser);

	DBG_VERBOSE(printf("aliases addr=%p, size=%d\n", (void*) &(n.aliases), n.aliases.size));
	DBG_VERBOSE(UA_NodeSetAliases_println("aliases in nodeset: ", &n.aliases));

	Namespace_iterate(n.ns, print_node);
	server_application_memory_init(n.ns);

	// Direct encoding
	UA_ByteString buffer = { 1024, (UA_Byte*) buf };

	UA_NodeId nodeid;
	nodeid.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	nodeid.namespace = 0;
	nodeid.identifier.numeric = 2256; // ServerStatus

	// Design alternative one : use generated data types
	void* data;
	UA_Int32 pos = 0;
	if (Namespace_getData(n.ns,&nodeid,&data,UA_NULL) != UA_SUCCESS)
		perror("no data");
	UA_ServerStatusDataType_encodeBinary(data,&pos,&buffer);
	buffer.length = pos;
	UA_ByteString_printx("generated encoder result=", &buffer);

	// Design alternative two : use generated data types
	pos = 0;
	buffer.length = 1024;
	UAX_NodeId_encodeBinary(n.ns,&nodeid,&pos,&buffer);
	buffer.length = pos;
	UA_ByteString_printx("namespace based encoder result=", &buffer);

	return 0;
}
