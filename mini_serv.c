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

static void	fatal_exit(void)
{
	char *str = "Fatal error\n";
	write(2, str, strlen(str));
	exit(1);
}

static int	create_server_listener(unsigned short int port)
{
	//TODO: Crear socket listener en 127.0.0.1
	//bzero, sockaddr
	//socket, bind, listen
}

static void	add_client(fd_set *reads, fd_set *writes, int *max,
		t_list *clients)
{
	//TODO: accept(), actualizar max, añadir a lista y FD_SET
}

static void	remove_client(fd_set *reads, fd_set *writes, int *max,
		t_list *clients)
{
	//TODO: close(), actualizar max, quitar de la lista y FD_CLR
}

static void	send_messages(char *buff, fd_set *writes, t_list *clients)
{
	//TODO: sprintf(%d) a cada línea de buff
	t_list *temp = client->next;
	while (temp != NULL)
	{
		if (FD_ISSET(temp->fd, &write_fds) &&
			send(temp->fd, buff, sizeof buff, 0) == -1)
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
	unsigned short int port = atoi(argv[1]);
	int listener = create_server_listener(port);
	int max = listener;

	fd_set read_fds;
	fd_set read_master;
	FD_ZERO(&read_fds);
	FD_ZERO(&read_master);
	FD_SET(listener, &read_master);

	fd_set write_fds;
	fd_set write_master;
	FD_ZERO(&write_fds);
	FD_ZERO(&write_master);
	FD_SET(listener, &write_master);

	t_list clients;
	clients.fd = listener;
	while(1)
	{
		FD_COPY(&read_master, &read_fds);
		FD_COPY(&write_master, &write_fds);
		if (select(max + 1, &read_fds, &write_fds, NULL, NULL) == -1)
			fatal_exit();
		//cambiar for por iterar sobre lista? Ya que se empieza en listener
		for (int i = 0; i <= max; ++i)
		{
			if (!FD_ISSET(i, &read_fds))
				continue ;
			if (i == listener)
				add_client(&read_master, &write_master, &max, &clients);
			else
			{
				char buff[4096];
				memset(buff, 0, sizeof buff);
				int nbytes;
				if ((nbytes = recv(i, buff, sizeof buff, 0) <= 0))
				{
					if (nbytes != 0)
						fatal_exit();
					remove_client(&read_master, &write_master, &max, &clients);
				}
				else
					send_messages(buff, &write_fds, &clients);
			}
		}
	}
}
