/*
 * ua_xml.h
 *
 *  Created on: 03.05.2014
 *      Author: mrt
 */

#ifndef __UA_XML_H__
#define __UA_XML_H__

#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <ctype.h> // isspace
#include <unistd.h> // read

#include "opcua.h"
#include "ua_namespace.h"

UA_Int32 UA_Boolean_copycstring(cstring src, UA_Boolean* dst);
UA_Int32 UA_Int16_copycstring(cstring src, UA_Int16* dst);
UA_Int32 UA_UInt16_copycstring(cstring src, UA_UInt16* dst) ;

void print_node(UA_Node const * node);

typedef struct UA_TypedArray {
	UA_Int32 size;
	UA_VTable* vt;
	void** elements;
} UA_TypedArray;

UA_Int32 UA_TypedArray_init(UA_TypedArray* p);
UA_Int32 UA_TypedArray_new(UA_TypedArray** p);
UA_Int32 UA_TypedArray_setType(UA_TypedArray* p, UA_Int32 type);
UA_Int32 UA_TypedArray_decodeXML(XML_Stack* s, XML_Attr* attr, UA_TypedArray* dst, _Bool isStart);

UA_Int32 UA_NodeSetAlias_init(UA_NodeSetAlias* p);
UA_Int32 UA_NodeSetAlias_new(UA_NodeSetAlias** p);
UA_Int32 UA_NodeSetAlias_decodeXML(XML_Stack* s, XML_Attr* attr, UA_NodeSetAlias* dst, _Bool isStart);

UA_Int32 UA_NodeSetAliases_init(UA_NodeSetAliases* p);
UA_Int32 UA_NodeSetAliases_new(UA_NodeSetAliases** p);
UA_Int32 UA_NodeSetAliases_println(cstring label, UA_NodeSetAliases *p);
UA_Int32 UA_NodeSetAliases_decodeXML(XML_Stack* s, XML_Attr* attr, UA_NodeSetAliases* dst, _Bool isStart);

typedef struct UA_NodeSet {
	Namespace* ns;
	UA_NodeSetAliases aliases;
} UA_NodeSet;
UA_Int32 UA_NodeSet_init(UA_NodeSet* p);
UA_Int32 UA_NodeSet_new(UA_NodeSet** p);
UA_Int32 UA_NodeId_copycstring(cstring src, UA_NodeId* dst, UA_NodeSetAliases* aliases);
UA_Int32 UA_NodeSet_decodeXML(XML_Stack* s, XML_Attr* attr, UA_NodeSet* dst, _Bool isStart);

typedef struct UA_NodeSetReferences {
	UA_Int32 size;
	UA_ReferenceNode** references;
} UA_NodeSetReferences;
UA_Int32 UA_NodeSetReferences_init(UA_NodeSetReferences* p);
UA_Int32 UA_NodeSetReferences_new(UA_NodeSetReferences** p);
UA_Int32 UA_NodeSetReferences_println(cstring label, UA_NodeSetReferences *p);

UA_Int32 UA_ReferenceNode_println(cstring label, UA_ReferenceNode *a);

UA_Int32 UA_ExpandedNodeId_copycstring(cstring src, UA_ExpandedNodeId* dst, UA_NodeSetAliases* aliases);

void XML_Stack_init(XML_Stack* p, cstring name);
void XML_Stack_print(XML_Stack* s);
void XML_Stack_handleTextAsElementOf(XML_Stack* p, cstring textAttrib, unsigned int textAttribIdx);
void XML_Stack_addChildHandler(XML_Stack* p, cstring name, UA_Int32 length, XML_decoder handler, UA_Int32 type, void* dst);

void XML_Stack_startElement(void * data, const char *el, const char **attr);
UA_Int32 XML_isSpace(cstring s, int len);
void XML_Stack_handleText(void * data, const char *txt, int len);
void XML_Stack_endElement(void *data, const char *el);

#endif // __UA_XML_H__
