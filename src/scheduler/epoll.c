/**
 * @file   epoll.c
 * @author Reginald Lips <reginald.l@gmail.com> - Copyright 2013
 * @date   Wed Feb  1 18:56:27 2017
 *
 * @brief  This file manages the poll API working with epoll.
 *
 *
 */

#include "rinoo/scheduler/module.h"

/**
 * Epoll initialization. It calls epoll_create and
 * initializes internal structures.
 *
 * @param sched Pointer to the scheduler to use.
 *
 * @return 0 if succeeds, else -1.
 */
int rn_epoll_init(rn_sched_t *sched)
{
	XASSERT(sched != NULL, -1);

	sched->epoll.fd = epoll_create(42); /* Size does not matter any more ;) */
	XASSERT(sched->epoll.fd != -1, -1);
	sched->epoll.curevent = -1;
	if (sigaction(SIGPIPE, &(struct sigaction){ .sa_handler = SIG_IGN }, NULL) != 0) {
		close(sched->epoll.fd);
		return -1;
	}
	return 0;
}

/**
 * Destroys the poller in a scheduler. Closes the epoll fd and free's memory.
 *
 * @param sched Pointer to the scheduler to use.
 */
void rn_epoll_destroy(rn_sched_t *sched)
{
	XASSERTN(sched != NULL);

	if (sched->epoll.fd != -1) {
		close(sched->epoll.fd);
	}
}

/**
 * Insert a socket into epoll. It calls epoll_ctl.
 *
 * @param node Scheduler node to add.
 * @param mode Polling mode to use to add.
 *
 * @return 0 if succeeds, else -1.
 */
int rn_epoll_insert(rn_sched_node_t *node, rn_sched_mode_t mode)
{
	struct epoll_event ev = { 0, { 0 } };

	if ((mode & RN_MODE_IN) == RN_MODE_IN) {
		ev.events |= EPOLLIN;
	}
	if ((mode & RN_MODE_OUT) == RN_MODE_OUT) {
		ev.events |= EPOLLOUT;
	}
	ev.events |= EPOLLET | EPOLLRDHUP;
	ev.data.ptr = node;
	if (unlikely(epoll_ctl(node->sched->epoll.fd, EPOLL_CTL_ADD, node->fd, &ev) != 0)) {
		return -1;
	}
	return 0;
}

/**
 * Adds a polling mode for a socket to epoll. It calls epoll_ctl.
 *
 * @param node Scheduler node to add.
 * @param mode Polling mode to use to add.
 *
 * @return 0 if succeeds, else -1.
 */
int rn_epoll_addmode(rn_sched_node_t *node, rn_sched_mode_t mode)
{
	struct epoll_event ev = { 0, { 0 } };

	if ((mode & RN_MODE_IN) == RN_MODE_IN) {
		ev.events |= EPOLLIN;
	}
	if ((mode & RN_MODE_OUT) == RN_MODE_OUT) {
		ev.events |= EPOLLOUT;
	}
	ev.events |= EPOLLET | EPOLLRDHUP;
	ev.data.ptr = node;
	if (unlikely(epoll_ctl(node->sched->epoll.fd, EPOLL_CTL_MOD, node->fd, &ev) != 0)) {
		return -1;
	}
	return 0;
}

/**
 * Removes a socket from epoll. It calls epoll_ctl.
 *
 * @param node Scheduler node to remove from epoll.
 *
 * @return 0 if succeeds, else -1.
 */
int rn_epoll_remove(rn_sched_node_t *node)
{
	if (unlikely(epoll_ctl(node->sched->epoll.fd, EPOLL_CTL_DEL, node->fd, NULL) != 0)) {
		return -1;
	}
	if (node->sched->epoll.curevent != -1 && node->sched->epoll.events[node->sched->epoll.curevent].data.ptr == node) {
		node->sched->epoll.events[node->sched->epoll.curevent].data.ptr = NULL;
	}
	return 0;
}

/**
 * Start polling. It calls epoll_wait.
 *
 * @param sched Pointer to the scheduler to use.
 * @param timeout Maximum time to wait in milliseconds (-1 for no timeout)
 *
 * @return 0 if succeeds, else -1.
 */
int rn_epoll_poll(rn_sched_t *sched, int timeout)
{
	int nbevents;
	struct epoll_event *event;

	XASSERT(sched != NULL, -1);

	nbevents = epoll_wait(sched->epoll.fd, sched->epoll.events, RN_EPOLL_MAX_EVENTS, timeout);
	if (unlikely(nbevents == -1)) {
		/* We don't want to raise an error in this case */
		return 0;
	}
	for (sched->epoll.curevent = 0; sched->epoll.curevent < nbevents; sched->epoll.curevent++) {
		event = &sched->epoll.events[sched->epoll.curevent];
		/* Check event->data.ptr for every event as one event could call rn_epoll_remove and destroy ptr */
		if (event->data.ptr != NULL && (event->events & EPOLLIN) == EPOLLIN) {
			rn_scheduler_wakeup(event->data.ptr, RN_MODE_IN, 0);
		}
		if (event->data.ptr != NULL && (event->events & EPOLLOUT) == EPOLLOUT) {
			rn_scheduler_wakeup(event->data.ptr, RN_MODE_OUT, 0);
		}
		if (event->data.ptr != NULL && (((event->events & EPOLLERR) == EPOLLERR || (event->events & EPOLLHUP) == EPOLLHUP))) {
			rn_scheduler_wakeup(event->data.ptr, RN_MODE_NONE, ECONNRESET);
		}
	}
	sched->epoll.curevent = -1;
	return 0;
}
