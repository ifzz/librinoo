/**
 * @file   httpresponse.h
 * @author Reginald LIPS <reginald.l@gmail.com> - Copyright 2011
 * @date   Mon Oct 25 18:05:40 2010
 *
 * @brief  Header file which describes http response structures
 *
 *
 */

#ifndef		RINOOHTTP_GLOBAL_HTTPRESPONSE_H_
# define	RINOOHTTP_GLOBAL_HTTPRESPONSE_H_

typedef struct	s_httpresponse
{
  t_httpversion	version;
  u32		code;
  t_buffer	msg;
  t_hashtable	*headers;
  u64		length;
  u64		received;
  u64		contentlength;
}		t_httpresponse;

struct s_httpsocket;

int		httpresponse_read(struct s_httpsocket *httpsock);
int		httpresponse_readbody(struct s_httpsocket *httpsock);
void		httpresponse_reset(struct s_httpsocket *httpsock);
void		httpresponse_setmsg(struct s_httpsocket *httpsock, const char *msg);
void		httpresponse_setdefaultmsg(struct s_httpsocket *httpsock);
void		httpresponse_setdefaultheaders(struct s_httpsocket *httpsock);

#endif		/* !RINOOHTTP_GLOBAL_HTTPRESPONSE_H_ */