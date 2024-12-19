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

char	 				buffer[MAX_PAYLOAD];



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
		printf("fsplit_daemon: failed to send message\n");
	}
}

static inline uint8_t hex_to_int(char c) {
	static const uint8_t table[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,	// 0-9
		0, 0, 0, 0, 0, 0, 0,			// :;<=>?@
		10, 11, 12, 13, 14, 15,			// A-F
	}; return table[c - '0'];
}

static inline void read_event(char* buf) {
	uint64_t read_size = 0, read_offset = 0;
	uint8_t i;
	for (i = 0; i < 0x10; i++) {
		read_size |= (hex_to_int(*buf++) << (60 - (i << 2)));
	}
	for (i = 0; i < 0x10; i++) {
		read_offset |= (hex_to_int(*buf++) << (60 - (i << 2)));
	}
	return;
	printf("read %lu bytes at %lu", read_size, read_offset);
}

static inline void write_event(char* buf) {
	uint16_t bytes = strlen(buf) >> 1;
	printf("wrote: ");
	for (uint16_t i = 0; i < bytes; i++) {
		printf("%c", (
			(hex_to_int(*buf++) << 4)	|
			(hex_to_int(*buf++) << 0)
		));
	} printf("\n");
}

static inline void ioctl_event(char* buf) {
	static const char* dir_types[] = {"VOID", "W", "R", "WR"};
	uint32_t cmd = 0; uint64_t arg = 0;
	uint8_t i;
	for (i = 0; i < 0x08; i++) {
		cmd |= (hex_to_int(*buf++) << (28 - (i << 2)));
	}
	uint8_t num =	(cmd >> 0) & 0xFFU;
	uint8_t type =	(cmd >> 8) & 0xFFU;
	uint16_t size =	(cmd >> 16) & 0x3FFFU;
	uint8_t dir =	(cmd >> 30) & 0x3U;
	// TODO args
	//for (i = 0; i < 0x10; i++) {
	//	arg |= (hex_to_int(*buf++) << (60 - (i << 2)));
	//}
	printf("ioctl(t=%c, n=%d, d=%s, s=%d)\n", type, num, dir_types[dir], size);
}

void handle_message(char* buf) {
	switch (*buf++) {
	case 'R':	return read_event(buf);
	case 'W':	return write_event(buf);
	case 'C':	return ioctl_event(buf);
	default:	return (void)printf("fsplit_daemon: unknown message: %c", *(buf - 1));
	}
}

void daemon_loop() {
	// initialize the connection with the kernel module
	send_message("init", 5);
	io_vector.iov_base = netlink_hdr;
	io_vector.iov_len = MAX_NETLINK_PACKET_SIZE;

	for(;;) {
		// get message
		memset(netlink_hdr, 0, MAX_NETLINK_PACKET_SIZE);
		if (recvmsg(sock_fd, &msg_hdr, 0) < 0) {
			printf("fsplit_daemon: failed to receive message");
			continue;
		}
		// handle message
		strcpy(buffer, (char*)NLMSG_DATA(netlink_hdr));
		//printf("raw: %s\n", buffer);
		handle_message(buffer);
	}
}


// avrdude -c arduino -p m328p -P /dev/fsplit -b115200 -D -U flash:w:firmware.hex:i -v -v -v -v
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