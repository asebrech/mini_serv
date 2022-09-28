#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	send_msg(int fd, int max_fd, char *msg, fd_set *writefds)
{
	for(int i = 0; i <= max_fd ; i++)
	{
		if (i != fd && FD_ISSET(i, writefds))
			send(i, msg, strlen(msg), 0);
	}
}

int main(int ac, char **av) {

	char *arg = "Wrong number of arguments\n";
	char *err = "Fatal error\n"; 
	if (ac != 2)
	{
		write(2, arg, strlen(arg));
		exit(1);
	}

	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		write(2, err, strlen(err));
		exit(1); 
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		write(2, err, strlen(err));
		exit(1); 
	} 
	if (listen(sockfd, 10) != 0) {
		write(2, err, strlen(err));
		exit(1); 
	}
	len = sizeof(cli);

	fd_set	masterfds;
	fd_set	readfds;
	fd_set	writefds;
	int	max_fd = sockfd;	
	int id = 0;
	int	ret;
	int	user_id[4096];
	char	*user_buff[4096];
	char	recv_buff[2048];
	char	msg[2048];
	char	*extracted_msg;

	FD_SET(sockfd, &masterfds);

	while(1)
	{
		writefds = readfds = masterfds;
		select(max_fd + 1, &readfds, &writefds, NULL, NULL);
		for (int i = 0; i <= max_fd; i++)
		{
			if (!FD_ISSET(i, &readfds))
				continue;
			if (i == sockfd)
			{
				connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
				FD_SET(connfd, &masterfds);
				if (connfd > max_fd)
					max_fd = connfd;
				user_id[connfd] = id;
				sprintf(msg, "server: client %d just arrived\n", id);
				send_msg(i, max_fd, msg, &writefds); 
				id++;
			}
			else
			{
				if ((ret = recv(i, recv_buff, 2048, 0)) <= 0)
				{
					sprintf(msg, "server: client %d just left\n", user_id[i]);
					send_msg(i, max_fd, msg, &writefds); 
					FD_CLR(i, &masterfds);
					close(i);
				}
				else
				{
					recv_buff[ret] = 0;
					user_buff[i] = str_join(user_buff[i], recv_buff);
					while (extract_message(&user_buff[i], &extracted_msg))
					{
						sprintf(msg, "client %d: %s", user_id[i], extracted_msg);
						send_msg(i, max_fd, msg, &writefds); 
						free(extracted_msg);
					}
				}
			}
		}
	}
}
