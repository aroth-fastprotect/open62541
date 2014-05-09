/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include "ua_xml.h"

// FIXME: most of the following code should be generated as a template from the ns0-defining xml-files
/** @brief server_application_memory is a flattened memory structure of the UAVariables in ns0 */
struct sam {
	UA_ServerStatusDataType serverStatus;
} sam;

char* productName = "xml2ns0";
char* productUri = "http://open62541.org/xml2ns0/";
char* manufacturerName = "open62541";
char* softwareVersion = "0.01";
char* buildNumber = "999" __DATE__ "-001" ;

#define SAM_ASSIGN_CSTRING(src,dst) do { \
	dst.length = strlen(src)-1; \
	dst.data = (UA_Byte*) src; \
} while(0)

void* direct_access_strategy(void* arg) { return arg; }
UA_DateTime* setGetCurrentTime(UA_DateTime* arg) { *arg = UA_DateTime_now(); return arg; }

void sam_attach(Namespace *ns,UA_Int32 id,UA_Int32 type, void* p) {
	Namespace_Lock* lock;
	UA_NodeId nodeid;
	nodeid.namespace = ns->namespaceId;
	nodeid.identifier.numeric = id;
	nodeid.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	UA_Node* result;
	Namespace_getWritable(ns,&nodeid,&result,&lock);
	if (result->nodeClass == UA_NODECLASS_VARIABLE) {
		UA_VariableNode* variable = (UA_VariableNode*) result;
		variable->value.arrayLength = 1;
		// FIXME: maybe we want to make a difference for an array and a scalar value
		if (variable->value.data == UA_NULL) {
			UA_alloc((void**)&(variable->value.data), sizeof(void*));
		}
		variable->value.data[0] = p;
		variable->value.encodingMask = UA_[type].ns0Id;
		variable->value.vt = &UA_[type];
	}
	Namespace_Lock_release(lock);
}

void sam_init(Namespace* ns) {
	// Initialize the strings
	SAM_ASSIGN_CSTRING(productName,sam.serverStatus.buildInfo.productName);
	SAM_ASSIGN_CSTRING(productUri,sam.serverStatus.buildInfo.productUri);
	SAM_ASSIGN_CSTRING(manufacturerName,sam.serverStatus.buildInfo.manufacturerName);
	SAM_ASSIGN_CSTRING(softwareVersion,sam.serverStatus.buildInfo.softwareVersion);
	SAM_ASSIGN_CSTRING(buildNumber,sam.serverStatus.buildInfo.buildNumber);

	// Attach memory
	sam_attach(ns,2256,UA_SERVERSTATUSDATATYPE,&sam.serverStatus); // this is the head of server status!
	sam_attach(ns,2257,UA_DATETIME, &sam.serverStatus.startTime);
	sam_attach(ns,2258,UA_DATETIME, &sam.serverStatus.currentTime);
	sam_attach(ns,2259,UA_SERVERSTATE, &sam.serverStatus.state);
	sam_attach(ns,2260,UA_BUILDINFO, &sam.serverStatus.buildInfo); // start of build Info
	sam_attach(ns,2261,UA_STRING, &sam.serverStatus.buildInfo.productName);
	sam_attach(ns,2262,UA_STRING, &sam.serverStatus.buildInfo.productUri);
	sam_attach(ns,2263,UA_STRING, &sam.serverStatus.buildInfo.manufacturerName);
	sam_attach(ns,2264,UA_STRING, &sam.serverStatus.buildInfo.softwareVersion);
	sam_attach(ns,2265,UA_STRING, &sam.serverStatus.buildInfo.buildNumber);
	sam_attach(ns,2266,UA_DATETIME, &sam.serverStatus.buildInfo.buildDate);
	sam_attach(ns,2992,UA_UINT32, &sam.serverStatus.secondsTillShutdown);
	sam_attach(ns,2993,UA_STRING,&sam.serverStatus.shutdownReason);
}

UA_Int16 UA_NodeId_getNamespace(UA_NodeId const * id) {
	return id->namespace;
}
// FIXME: to simple
UA_Int16 UA_NodeId_getIdentifier(UA_NodeId const * id) {
	return id->identifier.numeric;
}

_Bool UA_NodeId_isBasicType(UA_NodeId const * id) {
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


UA_Int32 UAX_NodeId_encodeBinary(Namespace const * ns, UA_NodeId const * id, UA_Int32* pos, UA_ByteString *dst) {
	UA_Int32 i, retval = UA_SUCCESS;
	if (UA_NodeId_isBasicType(id)) {
		UA_Node const * result;
		Namespace_Lock* lock;
		if ((retval = Namespace_get(ns,id,&result,&lock)) != UA_SUCCESS)
				return retval;
		UA_Variant_encodeBinary(&((UA_VariableNode *) result)->value,pos,dst);
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

	// Namespace_iterate(n.ns, print_node);
	sam_init(n.ns);
	Namespace_iterate(n.ns, print_node);

	// Direct encoding
	UA_ByteString buffer = { 1024, (UA_Byte*) buf };

	UA_NodeId nodeid;
	nodeid.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	nodeid.namespace = 0;
	nodeid.identifier.numeric = 2256; // ServerStatus

	// Use generated data types if you are sure that the memory layout fits
	UA_Node const * node;
	Namespace_Lock* lock;
	UA_Int32 pos = 0;
	if (Namespace_get(n.ns,&nodeid,&node,&lock) != UA_SUCCESS) {
		perror("no data");
	} else {
		if (node->nodeClass == UA_NODECLASS_VARIABLE) {
			// I forgot how cool this is - the Variant simply knows how to handle itself
			UA_Variant_encodeBinary(&((UA_VariableNode*) node)->value,&pos,&buffer);
			buffer.length = pos;
			UA_ByteString_printx("generated encoder result=", &buffer);
		}
		Namespace_Lock_release(lock);
	}

	// Design alternative two : use meta data
	pos = 0;
	buffer.length = 1024;
	// UAX_NodeId_encodeBinary(n.ns,&nodeid,&pos,&buffer);
	buffer.length = pos;
	UA_ByteString_printx("namespace based encoder result=", &buffer);

	return 0;
}
