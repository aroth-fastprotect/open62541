/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include "ua_xml.h"


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
	Namespace_iterate(n.ns, print_node);
	// printf("aliases addr=%p, size=%d\n", (void*) &(n.aliases), n.aliases.size);
	// UA_NodeSetAliases_println("aliases in nodeset: ", &n.aliases);
	return 0;
}
