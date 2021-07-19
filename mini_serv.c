#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

typedef struct		s_list
{
	int				fd;
	unsigned int	id;
	char			*cache;
	unsigned int	cache_size;
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

static void			fatal_exit(void)
{
	char *str = "Fatal error\n";
	write(2, str, strlen(str));
	exit(1);
}

static int			ft_digits(int num)
{
	char	dummy[16];
	return (sprintf(dummy, "%d", num));
}

static t_serv_conf	load_server_conf(unsigned short int port)
{
	int listener;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0
		|| bind(listener, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0
		|| listen(listener, 0) < 0)
		fatal_exit();
	t_serv_conf serv;
	serv.listener = listener;
	FD_ZERO(&serv.read_fds);
	FD_ZERO(&serv.read_master);
	FD_SET(listener, &serv.read_master);
	FD_ZERO(&serv.write_fds);
	FD_ZERO(&serv.write_master);
	FD_SET(listener, &serv.write_master);
	serv.max = listener;
	return (serv);
}

static void			add_user(t_serv_conf *serv, t_list **clients)
{
	//Accept new socket
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);
	bzero(&remoteaddr, addrlen);
	int newfd;
	if ((newfd = accept(serv->listener, (struct sockaddr *)&remoteaddr,
		&addrlen)) < 0)
		fatal_exit();
	//Set socket in the server master sets
	FD_SET(newfd, &serv->read_master);
	FD_SET(newfd, &serv->write_master);
	//Add user in the clients list
	t_list *init = *clients;
	t_list *temp = *clients;
	while (temp->next != NULL)
		temp = temp->next;
	t_list *newclient;
	if (!(newclient = calloc(1, sizeof(t_list))))
		fatal_exit();
	newclient->fd = newfd;
	newclient->id = init->id++;
	newclient->next = NULL;
	if (!(newclient->cache = calloc(1, sizeof(char))))
		fatal_exit();
	temp->next = newclient;
	//Send new client message to all other clients
	char *message;
	if (!(message = calloc(ft_digits(newclient->id) + 31, sizeof(char)))
		|| sprintf(message, "server: client %d just arrived\n",
			newclient->id) < 0)
		fatal_exit();
	temp = init->next;
	while (temp != NULL)
	{
		if (FD_ISSET(temp->fd, &serv->write_fds)
			&& temp->fd != newfd
			&& send(temp->fd, message, strlen(message), 0) == -1)
			fatal_exit();
		temp = temp->next;
	}
	//Update server max fd if needed
	if (newfd > serv->max)
		serv->max = newfd;
	*clients = init;
	free(message);
}

static t_list		*remove_user(t_list *removed, t_serv_conf *serv,
	t_list **clients)
{
	//Close socket
	if (close(removed->fd) < 0)
		fatal_exit();
	//Remove socket from server master sets
	FD_CLR(removed->fd, &serv->read_master);
	FD_CLR(removed->fd, &serv->write_master);
	//Remove client from clients list
	// AND send client left message to all other clients
	char *message;
	if (!(message = calloc(ft_digits(removed->id) + 28, sizeof(char)))
		|| sprintf(message, "server: client %d just left\n", removed->id) < 0)
		fatal_exit();
	t_list *init = *clients;
	t_list *ret;
	t_list *temp = *clients;
	while (temp != NULL)
	{
		if (temp->next == removed)
		{
			ret = temp;
			temp->next = removed->next;
		}
		if (FD_ISSET(temp->fd, &serv->write_fds) 
			&& temp->fd != serv->listener
			&& send(temp->fd, message, strlen(message), 0) == -1)
			fatal_exit();
		temp = temp->next;
	}
	//Update server max fd if needed
	if (serv->max == removed->fd)
	{
		serv->max = 0;
		temp = init;
		while (temp != NULL)
		{
			if (temp->fd > serv->max)
				serv->max = temp->fd;
			temp = temp->next;
		}
	}
	*clients = init;
	free(message);
	free(removed->cache);
	free(removed);
	return (ret);
}

static void			send_messages(t_list *sender, char *buff, t_serv_conf *serv,
	t_list *clients)
{
	char	*message;
	char	*nlpos;
	t_list	*init = clients;

	//Send what's stored in the cache if no bytes were read
	// OR send whatever lines have been read
	while ((sender->cache_size > 0 && *buff == '\0')
		|| (nlpos = strstr(buff, "\n")) != NULL)
	{
		if (!(message = calloc(ft_digits(sender->id) + 10, sizeof(char)))
			|| sprintf(message, "client %d: ", sender->id) < 0)
			fatal_exit();
		if (sender->cache_size > 0)
		{
			if (!(message = realloc(message, strlen(message)
				+ sender->cache_size + 1 + 1 * (*buff == '\0')))
				|| !strcat(message, sender->cache))
				fatal_exit();
			bzero(sender->cache, sender->cache_size + 1);
			free(sender->cache);
			if (!(sender->cache = calloc(1, sizeof(char))))
				fatal_exit();
			sender->cache_size = 0;
		}
		if (*buff)
		{
			*nlpos = '\0';
			if (!(message = realloc(message,  strlen(message)
				+ (nlpos - buff) + 2)) || !strcat(message, buff))
				fatal_exit();
			buff = nlpos + 1;
		}
		if (!strcat(message, "\n"))
			fatal_exit();
		clients = init->next;
		while (clients != NULL)
		{
			if (FD_ISSET(clients->fd, &serv->write_fds)
				&& clients->fd != sender->fd
				&& send(clients->fd, message, strlen(message), 0) == -1)
				fatal_exit();
			clients = clients->next;
		}
		bzero(message, strlen(message) + 1);
		free(message);
	}
	//Store the read, incomplete line in the cache. This cannot contain '\n'
	if (*buff != '\0')
	{
		sender->cache_size += strlen(buff);
		if (!(sender->cache = realloc(sender->cache, sender->cache_size + 1))
			|| !strcat(sender->cache, buff))
			fatal_exit();
	}
}

int					main(int argc, char **argv)
{
	if (argc != 2)
	{
		char *str = "Wrong number of arguments\n";
		write(2, str, strlen(str));
		exit(1);
	}
	t_serv_conf serv = load_server_conf(atoi(argv[1]));
	t_list *clients;
	if (!(clients = calloc(1, sizeof(t_list))))
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
				add_user(&serv, &clients);
			else
			{
				char buff[BUFFER_SIZE + 1];
				bzero(buff, sizeof(buff));
				int rbytes = recv(temp->fd, buff, BUFFER_SIZE, 0);
				if (rbytes < 0)
					fatal_exit();
				else if (rbytes == 0)
				{
					if (temp->cache_size != 0)
						send_messages(temp, buff, &serv, clients);
					temp = remove_user(temp, &serv, &clients);
				}
				else
					send_messages(temp, buff, &serv, clients);
			}
		}
	}
}
