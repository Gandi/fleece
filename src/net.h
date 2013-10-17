struct addrinfo { 
    int              ai_flags; 
    int              ai_family; 
    int              ai_socktype; 
    int              ai_protocol; 
    socklen_t        ai_addrlen; 
    struct sockaddr *ai_addr; 
    char            *ai_canonname; 
    struct addrinfo *ai_next;
};

extern int getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints,
                 struct addrinfo **res);

extern void freeaddrinfo(struct addrinfo *res);

