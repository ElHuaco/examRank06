#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct		s_list
{
	int				fd;
	int				id;
	struct s_list	*next;
}					t_list;

typedef struct		s_serv_conf
{
	int				listener;
	fd_set			read_fds;
	fd_set			read_master;
	fd_set			write_fds;
	fd_set			write_master;
	int				max;
}					t_serv_conf;

static void	fatal_exit(void)
{
	char *str = "Fatal error\n";
	write(2, str, strlen(str));
	exit(1);
}

static int	ft_digit_num(int num)
{
	int digits = 1;
	while ((num /= 10) > 0)
		digits++;
	return (digits);
}

t_serv_conf	load_server_conf(unsigned short int port)
{
	int listener;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		fatal_exit();
	if (bind(listener, (const struct sockaddr *)&servaddr,
		sizeof(servaddr)) < 0)
		fatal_exit();
	if (listen(listener, 0) < 0)
		fatal_exit();
	t_serv_conf serv;
	serv.listener = listener;
	serv.max = listener;
	FD_ZERO(&serv.read_fds);
	FD_ZERO(&serv.read_master);
	FD_SET(listener, &serv.read_master);
	FD_ZERO(&serv.write_fds);
	FD_ZERO(&serv.write_master);
	FD_SET(listener, &serv.write_master);
	return (serv);
}

static void	add_client(t_serv_conf *serv, t_list **clients)
{
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);
	int newfd;
	if ((newfd = accept(serv->listener, (struct sockaddr *)&remoteaddr,
		&addrlen)) == -1)
		fatal_exit();
	FD_SET(newfd, &serv->read_master);
	FD_SET(newfd, &serv->write_master);
	if (newfd > serv->max)
		serv->max = newfd;
	t_list *init = *clients;
	t_list *temp = *clients;
	while (temp->next != NULL)
		temp = temp->next;
	t_list *newclient;
	if (!(newclient = malloc(sizeof(t_list))))
		fatal_exit();
	newclient->fd = newfd;
	newclient->id = init->id++ + 1;
	newclient->next = NULL;
	temp->next = newclient;
	char *message;
	if (!(message = calloc(ft_digit_num(newfd) + 31, sizeof(char))))
		fatal_exit();
	if (sprintf(message, "server: client %d just arrived\n",
		newclient->id) < 0)
		fatal_exit();
	*clients = init;
	temp = (*clients)->next;
	while (temp != NULL)
	{
		if (FD_ISSET(temp->fd, &serv->write_fds) && temp->fd != newfd
			&& send(temp->fd, message, strlen(message), 0) == -1)
			fatal_exit();
		temp = temp->next;
	}
	*clients = init;
	free(message);
}

static void	remove_client(t_list *removed, t_serv_conf *serv, t_list **clients)
{
	if (close(removed->fd) == -1)
		fatal_exit();
	FD_CLR(removed->fd, &serv->read_master);
	FD_CLR(removed->fd, &serv->write_master);
	char *message;
	if (!(message = calloc(ft_digit_num(removed->id) + 28, sizeof(char))))
		fatal_exit();
	if (sprintf(message, "server: client %d just left\n",
		removed->id) < 0)
		fatal_exit();
	t_list *init = *clients;
	t_list *temp = *clients;
	int removedfd = removed->fd;
	while (temp != NULL)
	{
		if (temp->next != NULL && temp->next == removed)
		{
			temp->next = removed->next;
			free(removed);
		}
		if (FD_ISSET(temp->fd, &serv->write_fds) && temp->fd != serv->listener
			&& send(temp->fd, message, strlen(message), 0) == -1)
			fatal_exit();
		temp = temp->next;
	}
	if (serv->max == removedfd)
	{
		serv->max = 0;
		*clients = init;
		temp = *clients;
		while (temp != NULL)
		{
			if (temp->fd > serv->max)
				serv->max = temp->fd;
			temp = temp->next;
		}
	}
	*clients = init;
	free(message);
}

static void	send_messages(t_list *sender, char *buff, t_serv_conf *serv,
	t_list *clients)
{
	char *message;
	if (!(message = calloc(1, sizeof(char))))
		fatal_exit();
	char *nlpos;
	while ((nlpos = strstr(buff, "\n")) != NULL)
	{
		*nlpos = '\0';
		if (!(message = realloc(message, strlen(message) + nlpos - buff
			+ ft_digit_num(sender->id) + 12)))
			fatal_exit();
		if (sprintf(message, "client %d: %s\n", sender->id, buff) < 0)
			fatal_exit();
		buff = nlpos + 1;
	}
	while (clients != NULL)
	{
		if (FD_ISSET(clients->fd, &serv->write_fds)
			&& clients->fd != serv->listener
			&& clients->fd != sender->fd
			&& send(clients->fd, message, strlen(message), 0) == -1)
			fatal_exit();
		clients = clients->next;
	}
	free(message);
}

int	main(int argc, char **argv)
{
	if (argc != 2)
	{
		char *str = "Wrong number of arguments\n";
		write(2, str, strlen(str));
		exit(1);
	}
	t_serv_conf serv = load_server_conf(atoi(argv[1]));
	t_list *clients;
	if (!(clients = malloc(sizeof(t_list))))
		fatal_exit();
	clients->fd = serv.listener;
	clients->id = -1;
	clients->next = NULL;
	t_list *temp;
	while(1)
	{
		FD_COPY(&serv.read_master, &serv.read_fds);
		FD_COPY(&serv.write_master, &serv.write_fds);
		if (select(serv.max + 1, &serv.read_fds, &serv.write_fds, 0, 0) == -1)
			fatal_exit();
		for (temp = clients; temp != NULL; temp = temp->next)
		{
			if (!FD_ISSET(temp->fd, &serv.read_fds))
				continue ;
			if (temp->fd == serv.listener)
				add_client(&serv, &clients);
			else
			{
				char buff[4096];
				memset(buff, 0, sizeof(buff));
				if (recv(temp->fd, buff, sizeof(buff), 0) <= 0)
					remove_client(temp, &serv, &clients);
				else
					send_messages(temp, buff, &serv, clients);
			}
		}
	}
}
