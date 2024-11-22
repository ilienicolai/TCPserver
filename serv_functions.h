bool find_match(string t_subs, string t_udp);
vector<string> add_subs_uniqe(vector<string> result, vector<string> new_subs);
vector<string> wild_card(vector<struct topic_subs> ts, string tp);
struct topic_subs *find_topic(vector<struct topic_subs> &ts, string tp);
struct client *find_client_by_id(vector<struct client> &cl, string id);
struct client *find_client_by_socket(vector<struct client> &cl, int sock);
void handle_udp_message(int udp_socket,
    vector<struct topic_subs> &topic_subs_struct, vector<struct client> &clients);
void handle_tcp_connection_req(int tcp_socket, struct pollfd *poll_fds,
                                int &num_sockets, vector<struct client> &clients);
void handle_unsubscribe(vector<struct topic_subs> &topic_subs_struct, struct client *client_found,
                      struct pollfd *poll_fds, int i, struct tcp_struct *recv_subs_message);
void handle_subscribe(vector<struct topic_subs> &topic_subs_struct, struct client *client_found,
                      struct pollfd *poll_fds, int i, struct tcp_struct *recv_subs_message);