#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

enum Method { GET, HEAD, POST, PATCH, PUT, DELETE };

struct Slice {
	unsigned char *ptr;
	size_t len;
};

struct Request {
	int fd;
	struct Slice path;
	enum Method method;
};

struct Route {
	void (*handler)(struct Request *);
	struct Slice path;
} routes[512]; // 512 should be enough for anybody :D
size_t routes_cnt;

void http_write(struct Request *r, const char *string) {
	write(r->fd, string, strlen(string));
}

void http_writebuf(struct Request *r, void *buf, size_t len) {
	write(r->fd, buf, len);
}

void http_status(struct Request *r, int code) {
	char buf[32];
	sprintf(buf, "HTTP/1.1 %d\n\n", code);
	printf("code=%d\n", code);
	http_write(r, buf);
}

// TODO
// struct Slice http_get_header(struct Request *r, char *key) {
// }

void http_route(const char *path, void (*handler)(struct Request*)) {
	printf("Registering %s\n", path);
	routes[routes_cnt].path.ptr = path;
	routes[routes_cnt].path.len = strlen(path);
	routes[routes_cnt].handler = handler;
	routes_cnt ++;
}

void handle(int fd) {
	unsigned char buf[1024];
	int n = read(fd, buf, 1024);

	if (n < 8) {
		printf("Failed to parse request - req line too short\n");
		return;
	}
	// write(1, buf, n); // debug

	struct Request r;
	r.fd = fd;
	int p = 0; // Parser location.

	// TODO
	if (strncmp("GET ", buf, 4) == 0) {
		r.method = GET;
		p = 4;
	} else if(strncmp("POST ", buf, 5) == 0) {
		r.method = POST;
		p = 5;
	} else {
		printf("Failed to parse request method\n");
		return;
	}

	r.path.ptr = buf+p;
	for (;p < n && buf[p] != ' ';p ++) {}
	if (buf[p] != ' ') {
		printf("Failed to parse request path\n");
		return;
	}
	r.path.len = p - (r.path.ptr-buf);

	printf("method=%d path=%.*s ", r.method, (int)r.path.len, r.path.ptr);

	for (size_t i = 0;i < routes_cnt;i ++) {
		if (routes[i].path.len != r.path.len) continue; // Can't possibly match
		size_t match = 0;
		for (;match < routes[i].path.len && routes[i].path.ptr[match] == r.path.ptr[match];match ++){}
		if (match == routes[i].path.len) {
			routes[i].handler(&r);
			return;
		}
	}

	http_status(&r, 404);
}

void http_listen(int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("cannot create socket");
		return;
	}

	/* allow immediate reuse of the port */
	int sockoptval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(int));

	struct sockaddr_in myaddr;
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return;
	}

	if (listen(sockfd, 5) < 0) {
		perror("listen failed");
		exit(1);
	}

	for (;;) {
		// printf("Waiting for connection...\n");
		struct sockaddr_in client_addr;
		socklen_t alen;
		int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &alen);
		if (connfd < 0) {
			perror("accept failed");
			exit(1);
		}

		char ip_str[INET_ADDRSTRLEN];
		struct in_addr ipAddr = client_addr.sin_addr;
		inet_ntop(AF_INET, &ipAddr, ip_str, INET_ADDRSTRLEN);
		printf("Connected from %s - ", ip_str);

		handle(connfd);
		close(connfd);
	}
}
