//
// Created by marijn on 12/17/24.
//
#include <linux/netlink.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"



/*!<
 * function declarations
 * */
int main(void);



/*!<
 * variables
 * */
struct sockaddr_nl		src_addr, dest_addr =	{};
struct nlmsghdr*		netlink_hdr =			NULL;
struct iovec			io_vector =				{};
struct msghdr			msg_hdr =				{};
int						sock_fd =				0;
void*					netlink_buffer =		NULL;



/*!<
 * functions
 * */
void send_message(void* msg, uint32_t size) {
	memset(netlink_hdr, 0, MAX_NETLINK_PACKET_SIZE);
	netlink_hdr->nlmsg_len = NLMSG_SPACE(size);
	netlink_hdr->nlmsg_pid = getpid();
	netlink_hdr->nlmsg_flags = 0;

	memcpy(NLMSG_DATA(netlink_hdr), msg, size);

	io_vector.iov_base = netlink_hdr;
	io_vector.iov_len = netlink_hdr->nlmsg_len;

	msg_hdr.msg_name = (void*)&dest_addr;
	msg_hdr.msg_namelen = sizeof(dest_addr);
	msg_hdr.msg_iov = &io_vector;
	msg_hdr.msg_iovlen = 1;

	if (sendmsg(sock_fd, &msg_hdr, 0) < 0) {
		perror("sendmsg");
		printf("fsplit_daemon: failed to send message\n");
	}
}

void daemon_loop() {
	// initialize the connection with the kernel module
	send_message("init", 5);

	io_vector.iov_base = netlink_hdr;
	io_vector.iov_len = MAX_NETLINK_PACKET_SIZE;

	for(;;) {
		memset(netlink_hdr, 0, MAX_NETLINK_PACKET_SIZE);

		if (recvmsg(sock_fd, &msg_hdr, 0) < 0) {
			printf("fsplit_daemon: failed to receive message");
			continue;
		}

		// TODO file writes!!
		uint32_t payload_len = netlink_hdr->nlmsg_len - NLMSG_HDRLEN;
		printf("received [%d]: %.*s\n", payload_len, payload_len, (char*)NLMSG_DATA(netlink_hdr));
	}
}



/*!<
 * init functions
 * */
int main() {
	// open the socket
	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_PORT);
	if (sock_fd < 0) {
		printf("fsplit_daemon: error can't open socket\n");
		return -1;
	}

	// setup own address
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family =	AF_NETLINK;
	src_addr.nl_pid =		getpid();

	// bind with own address
	if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
		printf("fsplit_daemon: error could not bind address\n");
		close(sock_fd); return -1;
	}

	// setup destination address
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family =	AF_NETLINK;
	dest_addr.nl_pid =		0;	// Linux kernel PID
	dest_addr.nl_groups =	0;	// Unicast

	// setup netlink header
	netlink_hdr = (struct nlmsghdr*)malloc(MAX_NETLINK_PACKET_SIZE);
	if (!netlink_hdr) {
		printf("fsplit_daemon: error allocating netlink header\n");
		close(sock_fd); return -1;
	}

	// start loop function
	daemon_loop();

	// clean up
	free(netlink_hdr);
	close(sock_fd);
	return 0;
}