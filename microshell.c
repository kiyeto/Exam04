#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>

typedef struct	s_args
{
	char	**args;
	int		in;
	int		out;
	struct s_args *next;
}				t_args;

int	len(char *str)
{
	int i = -1;
		while (str[++i]);
	return (i);
}

void	errors(char *err, int x, char *var)
{
    write(2, err, len(err));
	if (var)
		write(2, var, len(var));
    write(2, "\n", 1);
    if (x)
        exit(-1);
}

void process(t_args *t, int *fd, char **env)
{
	int pid;
	pid = fork();
	if (pid < 0)
		errors("error: fatal", 1, NULL);
	if (!pid)
	{
		if (t->in)
		{
			dup2 (t->in, 0);
			close (t->in);
		}
		if (t->out != 1)
		{
			dup2 (t->out, 1);
			close (t->out);
		}
		close(fd[0]);
		execve(t->args[0], t->args, env);
		errors("error: cannot execute ", 1, *t->args);
	}
}

void	exec_cd(t_args *t)
{
    if (!t->args[1] || t->args[2])
		errors("error: cd: bad arguments", 0, NULL);
    if (chdir(t->args[1]))
		errors("error: cd: cannot change directory to ", 0, t->args[1]);
}

void	exec(t_args *t, char **env)
{
	int fd[2];
	int in = t->in;
	while (t->next)
	{
		pipe (fd);
		t->in = in;
		t->out = fd[1];
		process(t, fd, env);
		close(fd[1]);
		if (in)
			close(in);
		in = fd[0];
		t = t->next;
	}
	t->in = in;
	if (!strcmp(t->args[0], "cd"))
	{
		exec_cd(t);
		return ;
	}
	process(t, fd, env);
	if (t->in)
		close (t->in);
	while (waitpid(0, NULL, 0) != -1);
}

t_args	*init_args()
{
	t_args *t;
	if (!( t = malloc(sizeof(t_args))) || !(t->args = malloc(sizeof(char *) * 100)))
		errors("error: fatal\n", 1, NULL);
	t->in = 0;
	t->out = 1;
	t->next = NULL;
	return (t);
}

void	fill_cmd(t_args **t, char **av, int *i)
{
	int j = 0;
	while (av[*i] && strcmp(av[*i], ";"))
	{
		if (!strcmp(av[*i], "|"))
		{
			(*t)->args[j] = NULL;
			(*i)++;
			(*t)->next = init_args();
			*t = (*t)->next;
			j = 0;
		}
		(*t)->args[j] = av[*i];
		(*i)++;
		j++;
	}
	(*t)->args[j] = NULL;
}

void	free_tok(t_args **t)
{
	while (*t)
	{
	    if ((*t)->args)
	        free((*t)->args);
	    free((*t));
	    (*t) = (*t)->next;
	}
	(*t) = NULL;
}

int main(int ac, char **av, char **env)
{
	t_args	*tok;
	t_args  *bu;
	int		i = 1;
	while (i < ac)
	{
		tok = init_args();
		bu = tok;
		if (!strcmp(av[i], ";"))
		{
			i++;
			continue;
		}
		fill_cmd(&tok, av, &i);
		exec(bu, env);
		free_tok(&bu);
	}
}