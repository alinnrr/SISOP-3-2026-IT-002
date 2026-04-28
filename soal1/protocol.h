#ifndef PROTOCOL_H
#define PROTOCOL_H

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define NAME_SIZE 64
#define ADMIN_NAME "The Knights"
#define ADMIN_PASSWORD "protocol124"
void  timestamp( char *buf, int size);
void write_log( const char *type, const char *message);

#endif
