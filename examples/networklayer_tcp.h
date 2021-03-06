/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef NETWORKLAYERTCP_H_
#define NETWORKLAYERTCP_H_

#ifdef WIN32
  #include "winsock2.h"
#else
  #include <sys/mman.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #include <sys/time.h>
#endif

#include "ua_server.h"

struct NetworklayerTCP;
typedef struct NetworklayerTCP NetworklayerTCP;

NetworklayerTCP * NetworklayerTCP_new(UA_ConnectionConfig localConf, UA_UInt32 port);
void NetworklayerTCP_delete(NetworklayerTCP *layer);
UA_StatusCode NetworkLayerTCP_run(NetworklayerTCP *layer, UA_Server *server, struct timeval tv,
                                  void(*worker)(UA_Server*), UA_Boolean *running);

#endif /* NETWORKLAYERTCP_H_ */
