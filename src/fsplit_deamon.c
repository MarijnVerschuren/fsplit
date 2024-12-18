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
	io_vector.iov_len =	netlink_hdr->nlmsg_len = NLMSG_SPACE(size);
	memcpy(netlink_buffer, msg, size);
	sendmsg(sock_fd, &msg_hdr, 0);
}

void deamon_loop() {
	netlink_buffer =		NLMSG_DATA(netlink_hdr);

	io_vector.iov_base =	(void*)netlink_hdr;
	msg_hdr.msg_name =		(void*)&dest_addr;
	msg_hdr.msg_namelen =	sizeof(dest_addr);
	msg_hdr.msg_iov =		&io_vector;
	msg_hdr.msg_iovlen =	1;

	send_message("init", 5);

	for (;;) {
		recvmsg(sock_fd, &msg_hdr, 0);
		printf("received [%d]: %.*s\n", netlink_hdr->nlmsg_len - NLMSG_HDRLEN, netlink_hdr->nlmsg_len - NLMSG_HDRLEN, NLMSG_DATA(netlink_hdr));
		printf("\n");
	}
}



/*!<
 * init functions
 * */
int main() {
	// open the socket
	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_PORT);
	if(sock_fd < 0) {
		printf("fsplit_deamon: error cant open socket");
		return -1;
	}

	// setup own address
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family =		AF_NETLINK;
	src_addr.nl_pid =			getpid();

	// bind with own address
	bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

	// setup destination address
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family =		AF_NETLINK;
	dest_addr.nl_pid =			0;	// linux kernel PID
	dest_addr.nl_groups =		0;	// unicast

	// setup netlink header
	netlink_hdr = (struct nlmsghdr*)malloc(MAX_NETLINK_PACKET_SIZE);
	memset(netlink_hdr, 0, MAX_NETLINK_PACKET_SIZE);
	netlink_hdr->nlmsg_len =	MAX_NETLINK_PACKET_SIZE;
	netlink_hdr->nlmsg_pid =	getpid();
	netlink_hdr->nlmsg_flags =	0;

	// start loop function
	deamon_loop();
	close(sock_fd);
}