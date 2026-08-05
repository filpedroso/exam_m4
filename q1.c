// Assignment name  : ft_popen
// Expected files   : ft_popen.c
// Allowed functions: pipe, fork, dup2, execvp, close, exit
// --------------------------------------------------------------------------------------

// Write the following function:

// int ft_popen(const char *file, char *const argv[], char type);

// The function must launch the executable file with the arguments argv (using execvp).
// If type is 'r' the function must return a file descriptor connected to the output of the command.
// If type is 'w' the function must return a file descriptor connected to the input of the command.
// In case of error or invalid parameter the function must return -1.

// Hints:
// Do not leak file descriptors!
// This exercise is inspired by the libc's popen().

// For example, the function could be used like that:

#include "/Users/filpedroso/EstudosCS/42/42_projects/common_core/libft/libft.h"

int	ft_popen(const char *file, char *const argv[], char type)
{
	pid_t	pid;
	int		pip[2];

	if (check_args(file, argv, type) == -1)
		return (-1);
	
	pipe(pip);
	if (type == 'r')
	{
		pid = fork();
		if (pid == 0)
		{
			dup2(pip[1], STDOUT_FILENO);
			close(pip[0]);
			close(pip[1]);
			execvp(file, argv);
			exit(1);
		}
		close (pip[1]);
		return (pip[0]);
	}
	if (type == 'w')
	{
		pid = fork();
		if (pid == 0)
		{
			dup2(pip[0], STDIN_FILENO);
			close(pip[1]);
			close(pip[0]);
			execvp(file, argv);
			exit(1);
		}
		close (pip[0]);
		return (pip[1]);
	}
	return (-1);
}


int main()
{
    int  fd;
    char *line;

    fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
    while ((line = get_next_line(fd)))
        ft_putstr(line);
    return (0);
}





/* int	main() {
	int	fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
	dup2(fd, 0);
	fd = ft_popen("grep", (char *const []){"grep", "c", NULL}, 'r');
	char	*line;
	while ((line = get_next_line(fd)))
		printf("%s", line);
} */