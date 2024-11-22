#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <iomanip>
#define MAX_CONNECTIONS 200
#define MAX_TOPIC 51
#define MAX_ID_LEN 10
#define MAX_UDP_SIZE 1552
#define MAX_UDP_PAYLOAD 1500
#define MAX_TCP_PAYLOAD 60

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                 \
  do                                                     \
  {                                                      \
    if (assertion)                                       \
    {                                                    \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
      perror(call_description);                          \
      exit(EXIT_FAILURE);                                \
    }                                                    \
  } while (0)

struct topic_udp
{
    char topic[MAX_TOPIC];
    uint8_t type;
    char payload[MAX_UDP_PAYLOAD];
};

struct tcp_struct
{
    int type;
    char payload[MAX_TCP_PAYLOAD];
};

struct msg_to_subs
{
    int type_tcp;
    uint16_t port_udp;
    uint8_t type;
    struct in_addr ip_udp;
    char topic[MAX_TOPIC];
    char payload[MAX_UDP_PAYLOAD];
};

struct topic_subs
{
    std::string topic;
    std::vector<std::string> subscribers;
};

struct client
{
    std::string id;
    int socket;
};
