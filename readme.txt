STRUCTURI DE COMUNICARE
----------------------
struct topic_udp
{
    char topic[MAX_TOPIC];
    uint8_t type;
    char payload[MAX_UDP_PAYLOAD];
};

Structura modeleaza un mesaj primit de la un client UDP si contine:
    -topic - sir de caractere de maxim 51 de caractere (50 + '\0')
    -type - un unsigned int pe 1 octet care descrie tipul payload-ului
    -payload - sir de caractere de maxim 1500 de caractere

----------------------
struct tcp_struct
{
    int type;
    char payload[MAX_TCP_PAYLOAD];
};

Structura modeleaza un mesaj tcp utilizat pentru actiunile de conectare,
subscribe, unsubscribe ale clientilor TCP. Campul type poate avea urmatoarele
valori:
    0 pentru mesaj de conectare iar in payload se afla ID-ul clietului
    1 pentru mesaj de subscribe iar in payload se afla numele topicului
    -1 pentru mesaj de unsubscribe iar in payload se afla numele topicului

----------------------
struct msg_to_subs
{
    int type_tcp;
    uint16_t port_udp;
    uint8_t type;
    struct in_addr ip_udp;
    char topic[MAX_TOPIC];
    char payload[MAX_UDP_PAYLOAD];
};

Structura modeleaza un mesaj trimis de catre server catre un client TCP, ce
contine topic-ul la care este abonat utilizatorul. Contine:
    - type_tcp - camp care descrie tipul de mesaj (adaugat pentru siguranta,
                    are valoarea 2)
    - port_udp - portul clientului UDP care a trimis mesajul
    - type - tipul payload-ului trimis de clientul UDP
    - ip_udp - adresa ip a clientului UDP
    - topic - numele topicului
    - payload - payload-ul trimis de clientul UDP

----------------------

ALTE STRUCTURI

----------------------
struct topic_subs
{
    std::string topic;
    std::vector<std::string> subscribers;
};

Structura contine legatura dintre un topic si abonatii la topic-ul respectiv.

----------------------
struct client
{
    std::string id;
    int socket;
};

Structura contine legatura dintre un client TCP si socket-ul pe care comunica
cu serverul (pentru clientii abonati anterior socket va fi setat la -1).

----------------------

SERVERUL

Primul pas este verificarea corectitudinii parametrilor de rulare. Apoi este
dezactivat bufferingul. Sunt initializati socketii pentru UDP si TCP. De 
asemenea este dezactivat algoritmul Nagle de pe socket-ul TCP. Este
initializat un sir de struct pollfd care ajuta la multiplexare si vom pune
pe primele 3 pozitii in ordine: STDIN_FILENO, socket-ul UDP si cel TCP;

Avem un vector de clienti in care vom memora clientii TCP si socketii aferenti
lor, dar si un vector in care mapam topic-urile si abonatii lor.

Apoi se deschide bucla infinita in care vom verifica descriptorii de fisiere pe
care avem date.

Daca avem date la stdin, vom citi comanda. Daca aceasta este "exit" atunci vom
inchide toate conexiunile si vom opri serverul.

Daca avem date pe socket-ul de UDP atunci vom primi intr-un buffer datele de la
clientul UDP. Apoi vom identifica datele transmise si le vom pune intr-o 
structura de tipul msg_to_subs. Cautam in lista de topic-uri toate care se 
potrivesc cu topicul furnizat de clientul UDP si furnizam un vector cu clientii
abonati la topic-ul respectiv.

Identificarea topic-urilor se face in functia wild_card() care parcurge lista
cu topic-uri si va face comparatie intre cel furnizat si cele din lista cu
ajutorul functiei find_match(). Aceasta separa cele 2 cai dupa caracrterul
'/' si parcurge calea din lista de topic-uri comparand cu elementele din calea data de 
clientul UDP. In cazul in care este gasit caracterul '+' se trece mai departe
in calea clientului UDP. In cazul in care gasim '*' vom verifica daca este
ultima, atunci se va returna true iar daca nu este ultima, se va cauta 
elementul de pe pozitia urmatoare din topic-ul din lista de topic-uri in resutul listei de
componente a topic-ului UDP. Clientii TCP sunt adaugati o singura data in
lista finala de abonati la topic.

La final este trimis mesajul cu toate datele catre toti abonatii la topic.

Daca avem date pe un socket-ul TCP de asculatre (poll_fds[2].fd) inseamna ca un
client a trimis un mesaj de conectare. Vom genera un nou socket si vom accepta
conexiunea si vom dezactiva algoritmul Nagle. Apoi vom primi mesajul cu ID-ul
clientului TCP. Determinam ID-ul si vom cauta clientul. Daca ID-ul este utilizat
de un alt client atunci se va afisa mesajul de eroare. Daca acesta nu a fost
conectat anterior, va fi adaugat in lista de clienti, iar daca a mai fost
conectat in trecut, i se va actualiza socket-ul la valoarea returnata de accept.

Daca nu avem date pe niciunul din socketii de mai sus atunci inseamna ca avem
situatiile de client deconectat sau careri de subscribe/unsubscribe. In situatia
in care clientul s-a deconectat va fi semnalat de apelul de recv_all care va
intoarce 0. Atunci clientul va avea socket-ul setat pe -1 si va fi eliminat din
poll_fds[]. 

In cazul in care recv_all nu intoarce 0, atunci vom verifica ce tip de mesaj
avem. Daca avem o cerere de subscribe vom cauta topicul in lista de topic-uri.
Daca il gasim, verificam ca subscriber-ul sa nu fie deja abonat la topic.
Daca nu gasim topic-ul, vom genera o noua intrare in lista de topic-uri. La
final se va trimite 1 catre client.

Daca avem o cere de unsubscribe, vom cauta topic-ul si vom elimina din lista de
abonati pe solicitant. Daca topicul nu a fost gasit, se va trimite 0 catre
client. Daca dezabonarea s-a facut cu succes se va trimite 1.

----------------------

SUBSCRIBER-UL

Primul pas este verificarea parametrilor de rulare ai clientului TCP. Apoi
este dezactivat bufferingul. Este generat un socket TCP cu care se face
conectarea la server si se trimite ID-ul clientului prin intermediul
unei structuri de tipul tcp_struct. De asemenea este dezactivat algoritmul
Nagle de pe socket-ul TCP. Multiplexarea se face tot cu ajutorul poll_fds[],
in care avem stdin si socket-ul TCP generat.

Apoi se porneste bucla infinita si se verifica socketii pe care avem date.

Daca avem o comanda la stdin o vom parsa dupa ' ', si vom salva fiecare
componenta a ei intr-un vector de string. In cazul in care avem "exit"
vom termina bucla si vom inchide socketul.

Daca avem comanda de subscribe vom verifica daca avem destule argumente. Apoi
se creaza un mesaj tcp de tipul tcp_struct ce contine type = 1 si in payload
topic-ul la care se doreste abonarea. Apoi este trimis mesajul catre server
si se asteapta raspus. Daca avem valoarea 1 in variabila confirmation vom
afisa mesajul corespunzator de subscribe.

Daca avem comanda de unsubscribe vom verifica de asemenea daca avem argumentele
corespunzatoare.  Apoi se creaza un mesaj tcp de tipul tcp_struct ce contine
type = -1 si in payload topic-ul de la care se doreste dezabonarea. Apoi este 
trimis mesajul catre server si se asteapta raspus. Daca avem valoarea 1 in 
variabila confirmation vom afisa mesajul corespunzator de unsubscribe altfel
se va afisa un mesaj de eroare("Topic not found.").

Daca avem date pe socketul TCP vom face primirea datelor intr-o structura de tipul
msg_to_subs. Daca apelul de recv() intoarce 0 inseamna ca serverul s-a oprit
si vom opri si clientul. Daca recv() intoarce valoare diferita de 0 vom 
verifica daca tipul de mesaj este cel corespunzator (type_tcp = 2, utilizat
pentru a acoperi situatia in care vine din greseala alt tip de mesaj iar
parsarea acestuia se face in mod necorespunzator cauzand erori). Vom prelua
apoi datele primite si vom afisa mai intai ip-ul si portul clientului udp, apoi
vom apela functia corespunzatoare de parsare a payload-ului trimis de clientul
UDP in care se va face afisarea payload-ului.

----------------------

Primirea datelor se face cu ajutorul functiei recv_all() care face primirea
a len octeti in buffer.

Trimiterea datelor se face cu ajutorul functiei send_all() care face trimiterea
a len octeti din buffer.

----------------------

Obs.: Atat clientul cat si serverul nu sunt case-senisitive din punct de vedere
    al comenzilor pe care le primesc, ficare comada este convertita la lowercase
    prin functia to_lower().

