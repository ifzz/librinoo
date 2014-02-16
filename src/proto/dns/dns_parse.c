/**
 * @file   dns_parse.c
 * @author Reginald Lips <reginald.l@gmail.com> - Copyright 2014
 * @date   Sun Feb 16 00:24:11 2014
 *
 * @brief  DNS parsing functions
 *
 *
 */

#include "rinoo/rinoo.h"

int rinoo_dns_getheader(t_buffer_iterator *iterator, t_rinoodns_header *header)
{
	if (buffer_iterator_gethushort(iterator, &header->id) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &header->flags) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &header->qdcount) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &header->ancount) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &header->nscount) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &header->arcount) != 0) {
		return -1;
	}
	return 0;
}

int rinoo_dns_getname(t_buffer_iterator *iterator, t_buffer *name)
{
	char *label;
	unsigned char size;
	unsigned short tmp;
	t_buffer_iterator tmp_iter;

	while (!buffer_iterator_end(iterator)) {
		size = *(unsigned char *)(buffer_iterator_ptr(iterator));
		if (size == 0) {
			/* Move iterator position */
			buffer_iterator_getchar(iterator, NULL);
			buffer_addnull(name);
			return 0;
		} else if (DNS_QUERY_NAME_IS_COMPRESSED(size)) {
			if (buffer_iterator_gethushort(iterator, &tmp) != 0) {
				return -1;
			}
			buffer_iterator_set(&tmp_iter, iterator->buffer);
			if (buffer_iterator_position_set(&tmp_iter, DNS_QUERY_NAME_GET_OFFSET(tmp)) != 0) {
				return -1;
			}
			if (rinoo_dns_getname(&tmp_iter, name) != 0) {
				return -1;
			}
			/* Only end of domain can be compressed */
			return 0;
		} else {
			buffer_iterator_getchar(iterator, NULL);
			label = buffer_iterator_ptr(iterator);
			if (buffer_iterator_position_inc(iterator, size) != 0) {
				return -1;
			}
			if (buffer_size(name) > 0) {
				buffer_addstr(name, ".");
			}
			buffer_add(name, label, size);
		}
	}
	buffer_addnull(name);
	return 0;
}

int rinoo_dns_getrdata(t_buffer_iterator *iterator, size_t rdlength, t_rinoodns_type type, t_rinoodns_rdata *rdata)
{
	int ip;
	size_t position;

	position = buffer_iterator_position_get(iterator);
	switch (type) {
		case DNS_QUERY_A:
			if (buffer_iterator_getint(iterator, &ip) != 0) {
				return -1;
			}
			rdata->a.address = *(t_ip *)(&ip);
			break;
		case DNS_QUERY_NS:
			if (rinoo_dns_getname(iterator, &rdata->ns.nsname.buffer) != 0) {
				return -1;
			}
			break;
		case DNS_QUERY_CNAME:
			if (rinoo_dns_getname(iterator, &rdata->cname.cname.buffer) != 0) {
				return -1;
			}
			break;
		case DNS_QUERY_SOA:
			if (rinoo_dns_getname(iterator, &rdata->soa.mname.buffer) != 0) {
				return -1;
			}
			if (rinoo_dns_getname(iterator, &rdata->soa.rname.buffer) != 0) {
				return -1;
			}
			if (buffer_iterator_gethuint(iterator, &rdata->soa.serial) != 0) {
				return -1;
			}
			if (buffer_iterator_gethint(iterator, &rdata->soa.refresh) != 0) {
				return -1;
			}
			if (buffer_iterator_gethint(iterator, &rdata->soa.retry) != 0) {
				return -1;
			}
			if (buffer_iterator_gethint(iterator, &rdata->soa.expire) != 0) {
				return -1;
			}
			if (buffer_iterator_gethuint(iterator, &rdata->soa.minimum) != 0) {
				return -1;
			}
			break;
		case DNS_QUERY_PTR:
			if (rinoo_dns_getname(iterator, &rdata->ptr.ptrname.buffer) != 0) {
				return -1;
			}
			break;
		case DNS_QUERY_HINFO:
			if (rinoo_dns_getname(iterator, &rdata->hinfo.cpu.buffer) != 0) {
				return -1;
			}
			if (rinoo_dns_getname(iterator, &rdata->hinfo.os.buffer) != 0) {
				return -1;
			}
			break;
		case DNS_QUERY_MX:
			if (buffer_iterator_gethshort(iterator, &rdata->mx.preference) != 0) {
				return -1;
			}
			if (rinoo_dns_getname(iterator, &rdata->mx.exchange.buffer) != 0) {
				return -1;
			}
			break;
		case DNS_QUERY_TXT:
			if (rinoo_dns_getname(iterator, &rdata->txt.txtdata.buffer) != 0) {
				return -1;
			}
			break;
		default:
			return -1;
	}
	if (buffer_iterator_position_get(iterator) - position != rdlength) {
		return -1;
	}
	return 0;
}

int rinoo_dns_getquery(t_buffer_iterator *iterator, t_rinoodns_query *query)
{
	if (rinoo_dns_getname(iterator, &query->name.buffer) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &query->type) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &query->qclass) != 0) {
		return -1;
	}
	return 0;
}

int rinoo_dns_getanswer(t_buffer_iterator *iterator, t_rinoodns_type type, t_rinoodns_answer *answer)
{
	if (rinoo_dns_getname(iterator, &answer->name.buffer) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &answer->type) != 0) {
		return -1;
	}
	if (answer->type != type) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &answer->aclass) != 0) {
		return -1;
	}
	if (buffer_iterator_gethint(iterator, &answer->ttl) != 0) {
		return -1;
	}
	if (buffer_iterator_gethushort(iterator, &answer->rdlength) != 0) {
		return -1;
	}
	if (rinoo_dns_getrdata(iterator, answer->rdlength, answer->type, &answer->rdata) != 0) {
		return -1;
	}
	return 0;
}
