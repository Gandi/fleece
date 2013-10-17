extern int gethostname(char *name, size_t len);
extern int hostname_to_ip(char *hostname, char *ip);
extern const char *gai_strerror(int errcode);

int hostname_to_ip(char *hostname, char *ip);
