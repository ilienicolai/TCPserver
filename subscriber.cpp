#include "utils.h"

using namespace std;

// Receive len bytes from a socket
int recv_all(int sockfd, void *buffer, size_t len) {

    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    size_t bytes_received_now = 0;
    char *buff = (char *)buffer;
    while (bytes_remaining) {
        bytes_received_now  = recv(sockfd, buff + len - bytes_remaining, bytes_remaining, 0);
        if(bytes_received_now == 0) {
            return 0;
        }
        if (bytes_received_now < 0) {
            return -1;
        }
        bytes_received += bytes_received_now;
        bytes_remaining -= bytes_received;
    }

    
    return bytes_received;
}

// Send len bytes from buffer
int send_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    size_t bytes_sent_now = 0;
    char *buff = (char *)buffer;
    while (bytes_remaining) {
        bytes_sent_now = send(sockfd,  buff + len - bytes_remaining, bytes_remaining, 0);
        if(bytes_sent_now == 0) {
            return 0;
        }
        if (bytes_sent_now < 0) {
            return -1;
        }
        bytes_sent += bytes_sent_now;
        bytes_remaining -= bytes_sent;
    }
  return bytes_sent;
}

void parse_int(char *payload) {
    uint8_t sgn; // sign
    int number;
    // Copy the sign and the number.
    memcpy(&sgn, payload, sizeof(uint8_t));
    memcpy(&number, payload + sizeof(uint8_t), sizeof(int));
    number = ntohl(number);
    if (sgn)
        number *= -1;
    cout << " - INT - " << (int) number << endl;
}

void parse_short_real(char *payload) {
    uint16_t number;
    // Copy the number.
    memcpy(&number, payload, sizeof(uint16_t));
    number = ntohs(number);
    double short_real = number / 100.00;
    cout << " - SHORT_REAL - " << fixed << setprecision(2) << short_real << endl;
}

void parse_float(char *payload) {
    uint8_t sgn; // sign
    uint32_t raw_number; // number without the power
    uint8_t power;
    memcpy(&sgn, payload, sizeof(uint8_t));
    memcpy(&raw_number, payload + sizeof(uint8_t), sizeof(uint32_t));
    memcpy(&power, payload + sizeof(uint8_t) + sizeof(uint32_t), sizeof(uint8_t));
    double number = ntohl(raw_number);
    // Divide the number
    for (int i = 0; i < power; i++)
        number /= 10;
    if (sgn)
        number *= -1;
    cout << " - FLOAT - " << fixed << setprecision(power) << number << endl;
}

// Convert a string to lower case.
string to_lower(string str) {
    char *cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    for (uint64_t i = 0; i < str.length(); i++) {
        cstr[i] = tolower(cstr[i]);
    }
    str = string(cstr);
    return str;
}
// Parse a command by space.
vector<string> parse_command_by_space(string comm) {
    vector<string> result;
    char *cstr = new char[comm.length() + 1];
    strcpy(cstr, comm.c_str());
    char *token = strtok(cstr, " ");
    while (token != nullptr) {
        result.push_back(token);
        token = strtok(NULL, " ");
    }
    return result;
}

// Check if a string is a valid number.
bool check_valid_number(string str) {
    const char *s = str.c_str();
    for (size_t i = 0; i < strlen(s); i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
}
void subscribe (vector<string> splited_comm, int sockfd) {
    if (splited_comm.size() != 2) {
        cerr << "Incorrect number of arguments." << endl;
        cerr << "Expected subscribe <topic>" << endl;
        return;
    }
    // Create tcp message to request subscribe.
    auto *tcp_client_subscribe_msg = new tcp_struct;
    memset(tcp_client_subscribe_msg, 0, sizeof(struct tcp_struct));
    // type = 1 means subscribe request.
    tcp_client_subscribe_msg->type = 1;
    char *topic = (char *) splited_comm[1].c_str();
    // Put the topic in the payload.
    memcpy(tcp_client_subscribe_msg->payload, topic, strlen(topic));
    // Send subscribe request to the server.
    int rc = send_all(sockfd, tcp_client_subscribe_msg, sizeof(tcp_struct));
    DIE(rc < 0, "subscribe");
    // Check confirmation from server.
    int confirmation;
    rc = recv_all(sockfd, &confirmation, 4);
    DIE(rc < 0, "receive");
    if (confirmation == 1) {  // confirmation = 1 means success.
        cout << "Subscribed to topic " << splited_comm[1].c_str() << endl;
    }
}
void unsubscribe(vector<string> splited_comm, int sockfd) {
    if (splited_comm.size() != 2) {
        cerr << "Incorrect number of arguments." << endl;
        cerr << "Expected unsubscribe <topic>" << endl;
        return;
    }
    // Create tcp message to request unsubscribe.
    auto *tcp_client_unsubscribe_msg = new tcp_struct;
    memset(tcp_client_unsubscribe_msg, 0, sizeof(struct tcp_struct));
    // type = -1 means unsubscribe request.
    tcp_client_unsubscribe_msg->type = -1;
    // Put the topic in the payload.
    char *topic = (char *) splited_comm[1].c_str();
    memcpy(tcp_client_unsubscribe_msg->payload, topic, strlen(topic));
    // Send unsubscribe request to the server.
    int rc = send_all(sockfd, tcp_client_unsubscribe_msg, sizeof(tcp_struct));
    DIE(rc < 0, "unsubscribe");
    // Check confirmation from server.
    int confirmation;
    rc = recv_all(sockfd, &confirmation, 1);
    DIE(rc < 0, "receive");
    if (confirmation == 1) {
        cout << "Unsubscribed from topic " << splited_comm[1].c_str() << endl;
    } else {
        cout << "Topic not found." << endl;
    }
}

void receive_msg_from_server(int sockfd, bool &cond) {
     // Receive message from the server.
    auto  *recv_msg = new msg_to_subs;
    memset(recv_msg, 0, sizeof(struct msg_to_subs));
    int rc = recv(sockfd, recv_msg, sizeof(struct msg_to_subs), 0);
    DIE(rc < 0, "receive topic");
    if (rc == 0) { // server closed connection
        cond = false;
        return;
    } else { // Check if the message type is correct.
        if (recv_msg->type_tcp != 2) {
            return;
        }
    }
    // Parse the message.
    // Get the udp sender port and address.
    uint16_t udp_port = ntohs(recv_msg->port_udp);
    auto *udp_addr = inet_ntoa(recv_msg->ip_udp);
    cout << udp_addr << ":" << udp_port << " - " << recv_msg->topic;
    // Parse integer.
    if (recv_msg->type == 0) {
        parse_int(recv_msg->payload);
        return;
    }
    // Parse short real.
    if (recv_msg->type == 1) {
        parse_short_real(recv_msg->payload);
        return;
    }
    // Parse float.
    if (recv_msg->type == 2) {
        parse_float(recv_msg->payload);
        return;
    }
    // Parse string.
    if (recv_msg->type == 3) {
        cout << " - STRING - " << recv_msg->payload << endl;
        return;
    }
}
int main(int argc, char *argv[]) {
    // Check the number of arguments.
    if (argc != 4) {
        cerr << "Invalid number of arguments." << endl;
        cerr << "Expected: ./subscriber id server_address server_port" << endl;
        return 0;
    }

    // Get server port.
    if (!check_valid_number(argv[3])) {
        cerr << "Invalid port: expected number." << endl;
        return 0;
    }
    uint16_t port;
    port = stoi(argv[3]);
    DIE(port < 0, "port");

    // Buffering disabled.
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    int rc;
    // Create the socket.
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, socket_len);
    // Set the server address.
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr);
    DIE(rc < 0, "inet_aton");

    // Connect to the server.
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

    // Nagle's algorithm disabled.
    int flag = 1;
    rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(rc < 0, "nagle");

    // Get the client ID.
    auto *client_id = argv[1];

    // Create connection tcp message.
    auto *tcp_client_connect_msg = new tcp_struct;
    memset(tcp_client_connect_msg, 0, sizeof(struct tcp_struct));
    tcp_client_connect_msg->type = 0;

    // Set the client ID.
    memcpy(tcp_client_connect_msg->payload, client_id, MAX_ID_LEN + 1);

    // Connect to server with the client ID.
    rc = send_all(sockfd, tcp_client_connect_msg, sizeof(tcp_struct));
    DIE(rc < 0, "connect id");

    // Create poll.
    struct pollfd poll_fds[2];
    poll_fds[0].fd = sockfd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;
    int num_sockets = 2;
    
    // Poll loop.
    bool cond = true;
    while (cond) {
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");
        // Check if there is any data on the sockets.
        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == STDIN_FILENO) {
                    string comm;
                    getline(cin, comm);
                    auto splited_comm = parse_command_by_space(comm);
                    // Check if the command is exit.
                    if (to_lower(splited_comm[0]) == "exit") {
                        cond = false; // exit the loop
                        break;
                    }
                    // Check if the command is subscribe.
                    if (to_lower(splited_comm[0]) == "subscribe") {
                        subscribe(splited_comm, sockfd);
                        continue;
                    }
                    // Check if the command is unsubscribe.
                    if (to_lower(splited_comm[0]) == "unsubscribe") {
                        unsubscribe(splited_comm, sockfd);
                        continue;
                    }
                } else {
                    // Receive message from the server containing the topic.
                    receive_msg_from_server(sockfd, cond);
                }
            }
        }
    }
    // Close the socket.
    close(sockfd);
    return 0;
}