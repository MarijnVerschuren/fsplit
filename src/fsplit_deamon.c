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



/*!<
 * init functions
 * */
int main() {
	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_PORT);
	if(sock_fd < 0) {
		printf("fsplit_deamon: error cant open socket");
		return -1;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family =		AF_NETLINK;
	src_addr.nl_pid =			getpid();

	bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family =		AF_NETLINK;
	dest_addr.nl_pid =			0;	// linux kernel PID
	dest_addr.nl_groups =		0;	// unicast

	netlink_hdr = (struct nlmsghdr*)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(netlink_hdr, 0, NLMSG_SPACE(MAX_PAYLOAD));
	netlink_hdr->nlmsg_len =	NLMSG_SPACE(MAX_PAYLOAD);
	netlink_hdr->nlmsg_pid =	getpid();
	netlink_hdr->nlmsg_flags =	0;

	strcpy(NLMSG_DATA(netlink_hdr), "Hello");

	io_vector.iov_base =	(void*)netlink_hdr;
	io_vector.iov_len =		netlink_hdr->nlmsg_len;
	msg_hdr.msg_name =		(void*)&dest_addr;
	msg_hdr.msg_namelen =	sizeof(dest_addr);
	msg_hdr.msg_iov =		&io_vector;
	msg_hdr.msg_iovlen =	1;

	printf("sending message to kernel\n");
	sendmsg(sock_fd, &msg_hdr, 0);
	printf("waiting for message from kernel\n");

	recvmsg(sock_fd, &msg_hdr, 0);
	printf("\nreceived message payload: %s\n", (char*)NLMSG_DATA(netlink_hdr));
	close(sock_fd);
}