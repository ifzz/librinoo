/**
 * @file   ssl.c
 * @author Reginald Lips <reginald.l@gmail.com> - Copyright 2013
 * @date   Wed Feb  1 18:56:27 2017
 *
 * @brief  Secure connection management
 *
 *
 */

#include "rinoo/net/module.h"

extern const rn_socket_class_t socket_class_ssl;
extern const rn_socket_class_t socket_class_ssl6;

/**
 * Creates a simple SSL context.
 *
 *
 * @return SSL context pointer
 */
rn_ssl_ctx_t *rn_ssl_context(void)
{
	RSA *rsa;
	X509 *x509;
	SSL_CTX *ctx;
	EVP_PKEY *pkey;
	X509_NAME *name;
	rn_ssl_ctx_t *ssl;

	/* SSL_library_init is not reentrant! */
	SSL_library_init();
	pkey = EVP_PKEY_new();
	if (pkey == NULL) {
		return NULL;
	}
	x509 = X509_new();
	if (x509 == NULL) {
		EVP_PKEY_free(pkey);
		return NULL;
	}
	rsa = RSA_generate_key(512, RSA_F4, NULL,NULL);
	if (rsa == NULL || EVP_PKEY_assign_RSA(pkey, rsa) == 0) {
		X509_free(x509);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	X509_set_version(x509, 3);
	ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);
	X509_gmtime_adj(X509_get_notBefore(x509), 0);
	X509_gmtime_adj(X509_get_notAfter(x509), 60 * 60 * 24 * 360);
	X509_set_pubkey(x509, pkey);

	name = X509_get_subject_name(x509);
	if (X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char *) "US", -1, -1, 0) == 0) {
		X509_free(x509);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	if (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char *) "RiNOO", -1, -1, 0) == 0) {
		X509_free(x509);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	X509_set_issuer_name(x509, name);
	if (X509_sign(x509, pkey, EVP_md5()) == 0) {
		X509_free(x509);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	ssl = malloc(sizeof(*ssl));
	if (ssl == NULL) {
		X509_free(x509);
		EVP_PKEY_free(pkey);
		return NULL;
	}
	ctx = SSL_CTX_new(SSLv23_method());
	if (ctx == NULL) {
		X509_free(x509);
		EVP_PKEY_free(pkey);
		free(ssl);
		return NULL;
	}
	ssl->x509 = x509;
	ssl->pkey = pkey;
	ssl->ctx = ctx;
	if (SSL_CTX_use_certificate(ctx, x509) == 0) {
		rn_ssl_context_destroy(ssl);
		return NULL;
	}
	if (SSL_CTX_use_PrivateKey(ctx, pkey) == 0) {
		rn_ssl_context_destroy(ssl);
		return NULL;
	}
	return ssl;
}

/**
 * Destroys a SSL context.
 *
 * @param ctx SSL contextt pointer
 */
void rn_ssl_context_destroy(rn_ssl_ctx_t *ctx)
{
	if (ctx != NULL) {
		X509_free(ctx->x509);
		EVP_PKEY_free(ctx->pkey);
		SSL_CTX_free(ctx->ctx);
		free(ctx);
	}
}

/**
 * Gets a SSL socket from a rinoosocket.
 *
 * @param socket Socket pointer
 *
 * @return SSL socket pointer
 */
rn_ssl_t *rn_ssl_get(rn_socket_t *socket)
{
	return container_of(socket, rn_ssl_t, socket);
}

/**
 * Creates a SSL client and tries to connect to the specified IP and port.
 *
 * @param sched Scheduler pointer
 * @param ctx SSL context
 * @param ip Destination IP
 * @param port Destination port
 * @param timeout Socket timeout
 *
 * @return Socket pointer on success or NULL if an error occurs
 */
rn_socket_t *rn_ssl_client(rn_sched_t *sched, rn_ssl_ctx_t *ctx, rn_ip_t *ip, uint32_t port, uint32_t timeout)
{
	rn_ip_t loopback;
	rn_ssl_t *ssl;
	rn_socket_t *socket;
	socklen_t addr_len;
	struct sockaddr *addr;

	if (ip == NULL) {
		memset(&loopback, 0, sizeof(loopback));
		loopback.v4.sin_family = AF_INET;
		loopback.v4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ip = &loopback;
	}
	socket = rn_socket(sched, (IS_IPV6(ip) ? &socket_class_ssl6 : &socket_class_ssl));
	if (unlikely(socket == NULL)) {
		return NULL;
	}
	ssl = rn_ssl_get(socket);
	ssl->ctx = ctx;
	if (timeout != 0 && rn_socket_timeout(socket, timeout) != 0) {
		rn_socket_destroy(socket);
		return NULL;
	}
	if (ip->v4.sin_family == AF_INET) {
		ip->v4.sin_port = htons(port);
		addr = (struct sockaddr *) &ip->v4;
		addr_len = sizeof(ip->v4);
	} else {
		ip->v6.sin6_port = htons(port);
		addr = (struct sockaddr *) &ip->v6;
		addr_len = sizeof(ip->v6);
	}
	if (rn_socket_connect(socket, addr, addr_len) != 0) {
		rn_socket_destroy(socket);
		return NULL;
	}
	return socket;
}

/**
 * Creates a SSL server listening to a specific port, on specific IP.
 *
 * @param sched Scheduler pointer
 * @param ctx SSL context
 * @param ip IP to bind
 * @param port Port to bind
 *
 * @return Socket pointer on success or NULL if an error occurs
 */
rn_socket_t *rn_ssl_server(rn_sched_t *sched, rn_ssl_ctx_t *ctx, rn_ip_t *ip, uint32_t port)
{
	rn_ip_t any;
	rn_ssl_t *ssl;
	rn_socket_t *socket;
	socklen_t addr_len;
	struct sockaddr *addr;

	if (ip == NULL) {
		memset(&any, 0, sizeof(any));
		any.v4.sin_family = AF_INET;
		any.v4.sin_addr.s_addr = INADDR_ANY;
		ip = &any;
	}
	socket = rn_socket(sched, (IS_IPV6(ip) ? &socket_class_ssl6 : &socket_class_ssl));
	if (unlikely(socket == NULL)) {
		return NULL;
	}
	ssl = rn_ssl_get(socket);
	ssl->ctx = ctx;
	if (ip->v4.sin_family == AF_INET) {
		ip->v4.sin_port = htons(port);
		addr = (struct sockaddr *) &ip->v4;
		addr_len = sizeof(ip->v4);
	} else {
		ip->v6.sin6_port = htons(port);
		addr = (struct sockaddr *) &ip->v6;
		addr_len = sizeof(ip->v6);
	}
	if (rn_socket_bind(socket, addr, addr_len, RN_TCP_BACKLOG) != 0) {
		rn_socket_destroy(socket);
		return NULL;
	}
	return socket;
}

/**
 * Accepts a new connection from a listening socket.
 *
 * @param socket Pointer to the socket which is listening to
 * @param fromip Pointer to a rn_ip_t where to store the from_ip
 * @param fromport Pointer to a uint32_t where to store the from_port
 *
 * @return A pointer to the new socket on success or NULL if an error occurs
 */
rn_socket_t *rn_ssl_accept(rn_socket_t *socket, rn_ip_t *fromip, uint32_t *fromport)
{
	rn_ip_t addr;
	socklen_t addr_len;
	rn_socket_t *new;

	addr_len = sizeof(addr);
	new = rn_socket_accept(socket, (struct sockaddr *) &addr, &addr_len);
	if (fromip != NULL) {
		*fromip = addr;
	}
	if (fromport != NULL) {
		if (addr.v4.sin_family == AF_INET) {
			*fromport = ntohs(addr.v4.sin_port);
		} else {
			*fromport = ntohs(addr.v6.sin6_port);
		}
	}
	return new;
}

