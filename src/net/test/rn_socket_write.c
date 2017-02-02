/**
 * @file rn_socket_write.c
 * @author Reginald Lips <reginald.l@gmail.com> - Copyright 2013
 * @date   Wed Feb  1 18:56:27 2017
 *
 * @brief Test file for read/write functions.
 *
 *
 */
#include "rinoo/rinoo.h"

extern const rn_socket_class_t socket_class_tcp;

void process_client(void *arg)
{
	char b;
	rn_socket_t *socket = arg;

	rn_log("server - client accepted");
	rn_log("server - sending 'abcdef'");
	XTEST(rn_socket_write(socket, "abcdef", 6) == 6);
	rn_log("server - receiving 'b'");
	XTEST(rn_socket_read(socket, &b, 1) == 1);
	XTEST(b == 'b');
	rn_log("server - receiving nothing");
	XTEST(rn_socket_read(socket, &b, 1) == -1);
	rn_socket_destroy(socket);
}

void server_func(void *arg)
{
	rn_socket_t *server;
	rn_socket_t *client;
	struct sockaddr_in addr;
	rn_sched_t *sched = arg;

	server = rn_socket(sched, &socket_class_tcp);
	XTEST(server != NULL);
	addr.sin_port = htons(4242);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0;
	XTEST(rn_socket_bind(server, (struct sockaddr *) &addr, sizeof(addr), 42) == 0);
	rn_log("server listening...");
	client = rn_socket_accept(server, (struct sockaddr *) &addr, (socklen_t *)(int[]){(sizeof(struct sockaddr))});
	XTEST(client != NULL);
	rn_log("server - accepting client (%s:%d)", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	rn_task_start(sched, process_client, client);
	rn_socket_destroy(server);
}

void client_func(void *arg)
{
	char a;
	char cur;
	struct sockaddr_in addr;
	rn_socket_t *socket;
	rn_sched_t *sched = arg;

	socket = rn_socket(sched, &socket_class_tcp);
	XTEST(socket != NULL);
	addr.sin_port = htons(4242);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0;
	XTEST(rn_socket_connect(socket, (struct sockaddr *) &addr, sizeof(addr)) == 0);
	rn_log("client - connected");
	for (cur = 'a'; cur <= 'f'; cur++) {
		rn_log("client - receiving '%c'", cur);
		XTEST(rn_socket_read(socket, &a, 1) == 1);
		XTEST(a == cur);
	}
	rn_log("client - sending 'b'");
	XTEST(rn_socket_write(socket, "b", 1) == 1);
	rn_socket_destroy(socket);
}

/**
 * Main function for this unit test.
 *
 * @return 0 if test passed
 */
int main()
{
	rn_sched_t *sched;

	sched = rn_scheduler();
	XTEST(sched != NULL);
	XTEST(rn_task_start(sched, server_func, sched) == 0);
	XTEST(rn_task_start(sched, client_func, sched) == 0);
	rn_scheduler_loop(sched);
	rn_scheduler_destroy(sched);
	XPASS();
}
