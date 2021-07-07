#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//TODO: enters sueltos de mensajes, error al desconectar primer usuario

typedef struct		s_list
{
	int				fd;
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
	if (num < 0)
		num *= -1;
	int digits = 1;
	while ((num /= 10) > 0)
		digits++;
	return (digits);
}

t_serv_conf	load_server_conf(unsigned short int port)
{
	int listener;
	struct sockaddr_in servaddr; //struct sockaddr, sockaddr_in?
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		fatal_exit();
	if (bind(listener, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		fatal_exit();
	if (listen(listener, 10) < 0)
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
printf("Server configuration finished.\n");
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
printf("\tAccepted new connection.\n");
	FD_SET(newfd, &serv->read_master);
	FD_SET(newfd, &serv->write_master);
	if (newfd > serv->max)
		serv->max = newfd;
	char *message;
	if (!(message = malloc(sizeof(char) * (ft_digit_num(newfd) + 31))))
		fatal_exit();
	if (sprintf(message, "server: client %d just arrived\n", newfd) < 0)
		fatal_exit();
	t_list *init = *clients;
	t_list *temp = *clients;
	while (temp->next != NULL)
		temp = temp->next;
	t_list *newclient;
	if (!(newclient = malloc(sizeof(t_list))))
		fatal_exit();
	newclient->fd = newfd;
	newclient->next = NULL;
	temp->next = newclient;
	*clients = init;
	temp = (*clients)->next;
	while (temp != NULL)
	{
		if (FD_ISSET(temp->fd, &serv->write_fds) && temp->fd != newfd
			&& send(temp->fd, message, strlen(message), 0) == -1)
			fatal_exit();
printf("\tSent new user message to %d.\n", temp->fd);
		temp = temp->next;
	}
	free(message);
	*clients = init;
}

static void	remove_client(int removed, t_serv_conf *serv, t_list **clients)
{
	if (close(removed) == -1)
		fatal_exit();
printf("\tClosed the removed user connection.\n");
	FD_CLR(removed, &serv->read_master);
	FD_CLR(removed, &serv->write_master);
	char *message;
	if (!(message = malloc(sizeof(char) * (ft_digit_num(removed) + 28))))
		fatal_exit();
	if (sprintf(message, "server: client %d just left\n", removed) < 0)
		fatal_exit();
printf("\tCreated user left message\n");
	t_list *init = *clients;
	t_list *temp = (*clients)->next;
	while (temp != NULL)
	{
printf("\t\tChecking %d\n", temp->fd);
		if (temp->next != NULL && temp->next->fd == removed)
		{
			t_list *temp2 = temp->next;
			temp->next = temp2->next;
			free(temp2);
		}
		if (FD_ISSET(temp->fd, &serv->write_fds) && temp->fd != serv->listener
			&& send(temp->fd, message, strlen(message), 0) == -1)
			fatal_exit();
printf("\tSent user left message to %d\n", temp->fd);
		temp = temp->next;
	}
	if (serv->max == removed)
	{
		serv->max = 0;
		*clients = init;
		temp = (*clients)->next;
		while (temp != NULL)
		{
			if (temp->fd > serv->max)
				serv->max = temp->fd;
			temp = temp->next;
		}
	}
	*clients = init;
}

static void	send_messages(int sender, char *buff, t_serv_conf *serv,
	t_list *clients)
{
	char *message;
	if (!(message = malloc(sizeof(char))))
		fatal_exit();
	*message = '\0';
	char *nlpos;
	//acaba siempre en \n\0 el buff??
	while ((nlpos = strstr(buff, "\n")) != NULL)
	{
		*nlpos = '\0';
		if (!(message = realloc(message, strlen(message) + nlpos - buff
			+ ft_digit_num(sender) + 11)))
			fatal_exit();
		if (sprintf(message, "client %d: %s\n", sender, buff) < 0)
			fatal_exit();
		buff = nlpos + 1;
	}
	while (clients != NULL)
	{
		if (FD_ISSET(clients->fd, &serv->write_fds)
			&& clients->fd != serv->listener
			&& clients->fd != sender
			&& send(clients->fd, message, strlen(message), 0) == -1)
			fatal_exit();
printf("\tSent a message to user %d\n", clients->fd);
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
			{
printf("Adding a new client...\n");
				add_client(&serv, &clients);
			}
			else
			{
				char buff[4096];
				memset(buff, 0, sizeof(buff));
				if (recv(temp->fd, buff, sizeof(buff), 0) <= 0)
				{
printf("Removing a disconnected client...\n");
					remove_client(temp->fd, &serv, &clients);
				}
				else
				{
printf("Sending a message...\n");
					send_messages(temp->fd, buff, &serv, clients);
				}
			}
		}
	}
}
