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
// convert string to lower case
string to_lower(string str) {
    char *cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    for (uint64_t i = 0; i < str.length(); i++) {
        cstr[i] = tolower(cstr[i]);
    }
    str = string(cstr);
    return str;
}

// Check if a string matches a topic
bool find_match(string t_subs, string t_udp) {
    // t_subs = topic from subscribed topics
    // t_udp = topic from udp message
    if (t_subs == t_udp || t_subs == "*") {
        return true;
    }
    // Split the topics into components by "/"
    char *subs = new char[t_subs.length() + 1];
    strcpy(subs, t_subs.c_str());
    char *udp = new char[t_udp.length() + 1];
    strcpy(udp, t_udp.c_str());
    vector<string> wildcard_comps; // Components from subscribed topic
    vector<string> path_comps; // Components from udp topic
    char *p = strtok(subs, "/");
    while (p) {
        wildcard_comps.push_back(p);
        p = strtok(NULL, "/");
    }
    p = strtok(udp, "/");
    while (p) {
        path_comps.push_back(p);
        p = strtok(NULL, "/");
    }
    uint64_t j = 0; // Index for path_comps
    for (uint64_t i = 0; i < wildcard_comps.size(); i++) {
        if (wildcard_comps[i] != path_comps[j]) {
            if (wildcard_comps[i] == "+") {
                // If the component is "+", continue to the next component
                j++;
            } else if (wildcard_comps[i] == "*") {
                // If the component is "*"
                // If it is the last component, return true
                if (i == wildcard_comps.size() - 1) {
                    return true;
                }
                // Find the matching component in the path_comps
                uint64_t k = j + 1;
                while (k < path_comps.size()) {
                    if (path_comps[k] == wildcard_comps[i + 1]) {
                        break;
                    }
                    k++;
                }
                // If the component is not found, return false
                if (k == path_comps.size()) {
                    return false;
                }
                // Update the index
                j = k;
            } else {
                return false;
            }
        } else {
            // If the components match, continue to the next component
            j++;
        }
    }
    // If there are components left in path_comps, return false
    if (j != path_comps.size()) {
        return false;
    }
    return true;

}

// Add unique subscribers to the result
vector<string> add_subs_uniqe(vector<string> result, vector<string> new_subs) {
    for (uint64_t i = 0; i < new_subs.size(); i++) {
        if (find(result.begin(), result.end(), new_subs[i]) == result.end()) {
            result.push_back(new_subs[i]);
        }
    }
    return result;
    
}

// Find the subscribers for a topic
vector<string> wild_card(vector<struct topic_subs> ts, string tp) {
    vector<string> result;
    for (uint64_t i = 0; i < ts.size(); i++) {
        if (find_match(ts[i].topic, tp)) {
            result = add_subs_uniqe(result, ts[i].subscribers);
        }
    }
    return result;

}

// Find a topic in the list
struct topic_subs *find_topic(vector<struct topic_subs> &ts, string tp) {
    for (uint64_t i = 0; i < ts.size(); i++) {
        if (ts[i].topic == tp) {
            return &ts[i];
        }
    }
    return NULL;
}

// Find a client by id in clients
struct client *find_client_by_id(vector<struct client> &cl, string id) {
    for (uint64_t i = 0; i < cl.size(); i++) {
        if (cl[i].id == id) {
            return &cl[i];
        }
    }
    return NULL;
}

// Find a client by socket in clients
struct client *find_client_by_socket(vector<struct client> &cl, int sock) {
    for (uint64_t i = 0; i < cl.size(); i++) {
        if (cl[i].socket == sock) {
            return &cl[i];
        }
    }
    return NULL;
}

// Check if a string is a valid number
bool check_valid_number(string str) {
    const char *s = str.c_str();
    for (size_t i = 0; i < strlen(s); i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
}
void handle_udp_message(int udp_socket,
    vector<struct topic_subs> &topic_subs_struct, vector<struct client> &clients) {
    int rc;
    // Buffer to receive udp message
    char buff[MAX_UDP_SIZE];
    memset(buff, 0, MAX_UDP_SIZE);
    // Receive udp message
    auto *udp_addr = new sockaddr_in;
    socklen_t udp_addr_len = sizeof(struct sockaddr_in);
    rc = (int) recvfrom(udp_socket, buff, sizeof(buff), 0,
                        (struct sockaddr *) udp_addr, &udp_addr_len);
    DIE(rc < 0, "topic recv failed");
    // Create a new message to send to subscribers
    auto *subs_response = new msg_to_subs;
    memset(subs_response, 0, sizeof(struct msg_to_subs));
    subs_response->type_tcp = 2; // Set type to 2
    // type_tcp = 2 is the code for udp message sent to subscribers
    // Copy information about the udp message
    memcpy(&subs_response->ip_udp, &udp_addr->sin_addr, sizeof(struct in_addr));
    memcpy(&subs_response->port_udp, &udp_addr->sin_port, sizeof(uint16_t));
    // Copy the topic
    memcpy(subs_response->topic, buff, MAX_TOPIC-1);
    // Copy the type
    memcpy(&subs_response->type, buff + MAX_TOPIC - 1, sizeof(uint8_t));
    // Copy the payload
    memcpy(subs_response->payload, buff + MAX_TOPIC, MAX_UDP_PAYLOAD);
    // Find the subscribers
    vector<string> subscribers = wild_card(topic_subs_struct, subs_response->topic);
    // If there are no subscribers, continue
    if (subscribers.begin() == subscribers.end())
        return;
    // Send the message to all subscribers
    for (auto subs : subscribers) {
        // Find the client by id
        struct client *client_found = find_client_by_id(clients, subs);
        // Get the socket
        int subs_sock = client_found->socket;
        if (subs_sock >= 0) {
            rc = send_all(subs_sock, subs_response, sizeof(struct msg_to_subs));
            DIE(rc < 0, "subs response");
        }
    }
}

void handle_tcp_connection_req(int tcp_socket, struct pollfd *poll_fds,
                                int &num_sockets, vector<struct client> &clients) {
    // Accept the connection
    auto *subscriber_addr = new sockaddr_in;
    socklen_t subscriber_addr_len = sizeof(struct sockaddr_in);
    int subscriber = accept(tcp_socket, (struct sockaddr *)subscriber_addr, &subscriber_addr_len);
    DIE(subscriber < 0, "connection failed");
    // Nagle's algorithm disabled
    int flag = 1;
    int rc = setsockopt(subscriber, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    // New tcp message to receive the client's id
    auto *recv_subs_msg = new tcp_struct;
    rc = recv_all(subscriber, recv_subs_msg, sizeof(tcp_struct));
    DIE(rc < 0, "connection request");
    if (recv_subs_msg->type != 0) {
        // Invalid connection message
        close(subscriber);
        return;
    }
    auto *subscriber_id = (char *) recv_subs_msg->payload;
    auto *client_found = find_client_by_id(clients, subscriber_id);
    // Check if the client is already connected.
    if (client_found != NULL && client_found->socket != -1) {
        // Client is already connected.
        // Close the new connection.
        close(subscriber);
        cout << "Client " << subscriber_id << " already connected.\0" << endl;
        return;
    } else {
        if (client_found != NULL) {
            // Client connected before, update the socket.
            client_found->socket = subscriber;
        } else {
            // Client never connected before.
            // Add the client to the list.
            struct client *new_client = new struct client;
            new_client->id = subscriber_id;
            new_client->socket = subscriber;
            clients.push_back(*new_client);
        }
    }
    // Add the new socket to the poll_fds.
    poll_fds[num_sockets].fd = subscriber;
    poll_fds[num_sockets].events = POLLIN;
    num_sockets++;
    // Get the ip address and port of the client.
    auto *ip_addr = inet_ntoa(subscriber_addr->sin_addr);
    auto port = ntohs(subscriber_addr->sin_port);

    cout << "New client " << subscriber_id << " connected from " << ip_addr << ":" << port << endl;
}
void handle_unsubscribe(vector<struct topic_subs> &topic_subs_struct, struct client *client_found,
                      struct pollfd *poll_fds, int i, struct tcp_struct *recv_subs_message) {
    int confirmation = 0; // unsubscribe failed
    struct topic_subs *topic_found = find_topic(topic_subs_struct, recv_subs_message->payload);
    // Check if the topic exists.
    if (topic_found != NULL) {
        // Get the subscribers list.
        auto subs_tcp = topic_found->subscribers;
        // Find the client in the subscribers list.
        auto client_id = find(subs_tcp.begin(), subs_tcp.end(), client_found->id);
        // Remove the client from the list.
        if (client_id != subs_tcp.end()) {
            subs_tcp.erase(client_id);
            // Update the subscribers list.
            topic_found->subscribers = subs_tcp;
        }
        // Unsubscribe success -> send 1 to client.
        confirmation = 1;
    }
    int rc = send_all(poll_fds[i].fd, &confirmation, 4);
    DIE(rc < 0, "unsubscribe send");
}

void handle_subscribe(vector<struct topic_subs> &topic_subs_struct, struct client *client_found,
                      struct pollfd *poll_fds, int i, struct tcp_struct *recv_subs_message) {
    // Find the topic in the list.
    struct topic_subs *topic_found = find_topic(topic_subs_struct, recv_subs_message->payload);
    if (topic_found != NULL){
        auto subs = find(topic_found->subscribers.begin(), topic_found->subscribers.end(), client_found->id);
        // Check if the client is already subscribed.
        // If not, add the client to the subscribers list.
        if (subs == topic_found->subscribers.end())
            topic_found->subscribers.push_back(client_found->id);
    } else {
        // Topic doesn't exist
        // Create a new topic
        struct topic_subs *new_topic = new struct topic_subs;
        new_topic->topic = recv_subs_message->payload;
        // Add the client to the subscribers list.
        new_topic->subscribers.push_back(client_found->id);
        // Add the new topic to the list.
        topic_subs_struct.push_back(*new_topic);
    }
    // Subscribe success -> send 1 to client.
    int confirmation = 1;
    int rc = send_all(poll_fds[i].fd, &confirmation, 4);
    DIE(rc < 0, "subscribe send");
}
int main(int argc, char *argv[]) {
    // Check number of arguments
    if (argc != 2) {
        cerr << "Incorrect number of arguments" << endl;
        cerr << "Expected ./server <port>" << endl;
        return 0;
    }
    // Get port
    if (!check_valid_number(argv[1])) {
        cerr << "Invalid port: expected number" << endl;
        return 0;
    }
    uint16_t port;
    port = stoi(argv[1]);
    DIE(port == 0, "port");
    int rc;
    // Buffering disabled
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);
    //Get udp socket to receive topics
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "udp socket error");

    // Set fields to bind
    struct sockaddr_in *udp_addr = new sockaddr_in;
    socklen_t udp_len = sizeof(struct sockaddr_in);
    memset(udp_addr, 0, udp_len);
    udp_addr->sin_family = AF_INET;
    udp_addr->sin_port = htons(port);
    udp_addr->sin_addr.s_addr = INADDR_ANY;
    // Bind
    rc = bind(udp_socket, (const struct sockaddr *) udp_addr, sizeof(struct sockaddr_in));
    DIE(rc < 0, "bind udp failed");

    // Get tcp socket to accept connections from tcp clients
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_socket < 0, "udp socket error");

    // Set fields to bind
    struct sockaddr_in tcp_addr;
    socklen_t tcp_len = sizeof(struct sockaddr_in);
    memset(&tcp_addr, 0, tcp_len);
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    // Bind
    rc = bind(tcp_socket, (const struct sockaddr *) &tcp_addr, sizeof(struct sockaddr_in));
    DIE(rc < 0, "bind tcp failed");

    rc = listen(tcp_socket, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");
    // Nagle's algorithm disabled
    int flag = 1;
    rc = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    DIE(rc < 0, "nagle");

    // Set pollfd initially with stdin, udp_socket, tcp_socket
    struct pollfd poll_fds[MAX_CONNECTIONS];
    int num_sockets = 3;
    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = udp_socket;
    poll_fds[1].events = POLLIN;
    poll_fds[2].fd = tcp_socket;
    poll_fds[2].events = POLLIN;

    // Clients data
    vector<struct client> clients;
    
    // Link between topic and subscribers
    vector<struct topic_subs> topic_subs_struct;

    bool cond = true;
    while(cond) {
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");
        // Check if there is any data on the sockets.
        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) { // Check if there is data
                if (poll_fds[i].fd == STDIN_FILENO) {
                    string comm; // Command received from stdin.
                    cin>>comm;
                    // Convert to lower case.
                    comm = to_lower(comm);
                    if (comm == "exit") {
                        // Close all sockets except stdin.
                        for (int j = 1; j < num_sockets; j++) {
                            if (j != 2)
                                close(poll_fds[j].fd);
                        }
                        // Stop the server
                        cond = false;
                        break;
                    }

                    continue;
                }
                if (poll_fds[i].fd == udp_socket) { // New udp message received
                    handle_udp_message(udp_socket, topic_subs_struct, clients);
                    continue;
                }
                if (poll_fds[i].fd == tcp_socket) { // New client wants to connect
                    handle_tcp_connection_req(tcp_socket, poll_fds, num_sockets, clients);
                    continue;
                }
                // Client disconnected or wants to subscribe/unsubscribe
                // Receive the message from the tcp client
                auto *recv_subs_message = new tcp_struct;
                memset(recv_subs_message, 0, sizeof(struct tcp_struct));
                rc = recv_all(poll_fds[i].fd, recv_subs_message, sizeof(struct tcp_struct));
                DIE(rc < 0, "subs msg");
                struct client *client_found = find_client_by_socket(clients, poll_fds[i].fd);
                // No data received, client disconnected.
                if (rc == 0) {
                    cout << "Client " << client_found->id << " disconnected." << endl;
                    client_found->socket = -1;
                    // Elimitate the socket from the poll_fds.
                    close(poll_fds[i].fd);
                    for (int j = i; j < num_sockets - 1; j++) {
                        poll_fds[j] = poll_fds[j + 1];
                    }
                    continue;
                }
                // Execute the subscribe request.
                if (recv_subs_message->type == 1) {
                    handle_subscribe(topic_subs_struct, client_found, poll_fds, i, recv_subs_message);
                }
                // Execute the unsubscribe request.
                if (recv_subs_message->type == -1) {
                    handle_unsubscribe(topic_subs_struct, client_found, poll_fds, i, recv_subs_message);
                    continue;
                }
            }
        }
    }
    return 0;
}
