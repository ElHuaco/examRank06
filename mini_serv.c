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
	struct s_list	*next = NULL;
}					t_list;

typedef struct		s_serv_conf
{
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

t_serv_conf	load_server_conf(unsigned short int port)
{
	//TODO: Crear socket listener en 127.0.0.1
	//bzero, sockaddr
	//socket, bind, listen
	t_serv_conf serv;
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
	//TODO: accept(), actualizar max, añadir a lista y FD_SET
}

static void	remove_client(t_serv_conf *serv, t_list **clients)
{
	//TODO: close(), actualizar max, quitar de la lista y FD_CLR
}

static void	send_messages(int sender, char *buff, fd_set *write_fds,
	t_list **clients)
{
	//TODO: sprintf(client %d: ) a cada línea de buff
	t_list *temp = (*client)->next;
	while (temp != NULL)
	{
		if (FD_ISSET(temp->fd, &write_fds) && temp->fd != sender
			&& send(temp->fd, buff, sizeof buff, 0) == -1)
			fatal_exit();
		temp = temp->next;
	}
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
	if (!(clients = malloc(sizeof t_list)))
		fatal_exit();
	clients->fd = listener;
	t_list *temp;
	while(1)
	{
		FD_COPY(&serv.read_master, &serv.read_fds);
		FD_COPY(&serv.write_master, &serv.write_fds);
		if (select(serv.max + 1, &serv.read_fds, &serv.write_fds, 0, 0) == -1)
			fatal_exit();
		temp = clients;
		while (temp != NULL)
		{
			if (!FD_ISSET(temp->fd, &serv.read_fds))
				continue ;
			if (temp->fd == listener)
				add_client(&serv, &clients);
			else
			{
				char buff[4096];
				memset(buff, 0, sizeof buff);
				int nbytes;
				if ((nbytes = recv(temp->fd, buff, sizeof buff, 0) <= 0))
				{
					if (nbytes != 0)
						fatal_exit();
					remove_client(&serv, &clients);
				}
				else
					send_messages(temp->fd, buff, &serv->write_fds, &clients);
			}
			temp = temp->next;
		}
	}
}
