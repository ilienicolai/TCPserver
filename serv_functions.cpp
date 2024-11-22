#include <utils.h>
#include "serv_functions.h"
using namespace std;
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
    DIE(subscriber < 0, "subscribe");
    // Nagle's algorithm disabled
    int flag = 1;
    int rc = setsockopt(subscriber, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    // New tcp message to receive the client's id
    auto *recv_subs_msg = new tcp_struct;
    rc = recv_all(subscriber, recv_subs_msg, sizeof(tcp_struct));
    DIE(rc < 0, "tcp receive");
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
        if (client_found == NULL) {
            // Client never connected before.
            // Add the client to the list.
            struct client *new_client = new struct client;
            new_client->id = subscriber_id;
            new_client->socket = subscriber;
            clients.push_back(*new_client);
        } else {
            // Client connected before, update the socket.
            client_found->socket = subscriber;
        }
    }
    // Add the new socket to the poll_fds.
    poll_fds[num_sockets].fd = subscriber;
    poll_fds[num_sockets].events = POLLIN;
    num_sockets++;

    // Print the "new client connected" message.
    auto *ip_addr = inet_ntoa(subscriber_addr->sin_addr);
    auto port = ntohs(subscriber_addr->sin_port);

    cout << "New client " << subscriber_id << 
        " connected from " << ip_addr << ":" << port << endl;
}
void handle_unsubscribe(vector<struct topic_subs> &topic_subs_struct, struct client *client_found,
                      struct pollfd *poll_fds, int i, struct tcp_struct *recv_subs_message) {
    int confirmation = 0;
    struct topic_subs *topic_found = find_topic(topic_subs_struct, recv_subs_message->payload);
    // Check if the topic exists.
    if (topic_found != NULL) {
        // Get the subscribers list.
        auto subs_tcp = topic_found->subscribers;
        // Find the client in the subscribers list.
        auto client_id = find(subs_tcp.begin(), subs_tcp.end(), client_found->id);
        // If the client is found, remove it from the list.
        if (client_id != subs_tcp.end()) {
            subs_tcp.erase(client_id);
            // Update the subscribers list.
            topic_found->subscribers = subs_tcp;
        }
        // Unsubscribe success -> send 0 to client.
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
        // If the client is not already subscribed, add it to the list.
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