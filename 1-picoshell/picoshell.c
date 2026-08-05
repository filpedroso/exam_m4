#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

typedef enum e_state
{
	FIRST,
	MID,
	LAST
} t_state;

// close, fork, wait, exit, execvp, dup2, pipe
// int pipe(int fildes[2]);
// Data written to fildes[1] appears on (can be read from) fildes[0]
void	ft_child(t_state state, char **argv, int pip[2], int last_read);
t_state	next_state(char **cmds[], int i);

int	picoshell(char **cmds[])
{
	int		i		= -1;
	t_state	state	= FIRST;
	int		last_read = 0;

	int		pip[2];
	int		res;
	int		stat_loc;
	pid_t	pid;

	while (cmds[++i])
	{
		if (pipe(pip) == -1)
			return (1);
		pid = fork();
		if (pid == -1)
		{
			close(pip[0]);
			close(pip[1]);
			return (1);
		}
		else if (pid == 0)
			ft_child(state, cmds[i], pip, last_read);
		else
		{
			state = next_state(cmds, i);
			if (dup2(pip[0], last_read) == -1)
				return (1);
			close(pip[1]);
		}
	}
	close(last_read);
	while(1)
	{
		res = wait(&stat_loc);
		if (WEXITSTATUS(stat_loc) == 1)
			return (1);
		if (res == -1)
			break;
	}
	return (0);
}

void	ft_child(t_state state, char **argv, int pip[2], int last_read)
{
	close(pip[0]);
	if (state == FIRST)
	{
		dup2(pip[1], STDOUT_FILENO);
		execvp(argv[0], argv);
		close(pip[1]);
		exit(1);
	}
	else if (state == MID)
	{
		dup2(last_read, STDIN_FILENO);
		dup2(pip[1], STDOUT_FILENO);
		execvp(argv[0], argv);
		close(pip[1]);
		exit(1);
	}
	else if (state == LAST)
	{
		close(pip[1]);
		dup2(last_read, STDIN_FILENO);
		execvp(argv[0], argv);
		exit(1);
	}
}

t_state	next_state(char **cmds[], int i)
{
	if (cmds[i + 1] == NULL)
		return (LAST);
	else
		return (MID);
}







#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int picoshell(char **cmds[]);

/*
** ---------------------------------------------------------------------
** FD leak checker
**
** Portable (no /proc dependency, works on macOS + Linux): probes a
** range of fd numbers with fcntl(fd, F_GETFD). If it succeeds, the fd
** is open; if it fails with EBADF, it's closed.
**
** We snapshot before calling picoshell() and after, then diff.
** Anything open after that wasn't open before is a leak.
** Anything that was open before (0/1/2, or fds from the test harness
** itself) and is still open after is fine and expected.
** ---------------------------------------------------------------------
*/

#define FD_PROBE_LIMIT 256

static int is_fd_open(int fd)
{
    return (fcntl(fd, F_GETFD) != -1 || errno != EBADF);
}

static void snapshot_fds(int snapshot[FD_PROBE_LIMIT])
{
    int fd;

    fd = 0;
    while (fd < FD_PROBE_LIMIT)
    {
        snapshot[fd] = is_fd_open(fd);
        fd++;
    }
}

static int report_fd_diff(const int before[FD_PROBE_LIMIT],
    const int after[FD_PROBE_LIMIT])
{
    int fd;
    int leaked;

    leaked = 0;
    fd = 0;
    while (fd < FD_PROBE_LIMIT)
    {
        if (!before[fd] && after[fd])
        {
            printf("  [FD LEAK] fd %d was closed before, still open after "
                "picoshell() returned\n", fd);
            leaked++;
        }
        if (before[fd] && !after[fd] && fd > 2)
        {
            /* Not a bug in picoshell, just informational: something the
               test harness itself had open got closed. Rare, but worth
               knowing about if it ever shows up. */
            printf("  [FD INFO] fd %d was open before, closed after "
                "(likely harmless)\n", fd);
        }
        fd++;
    }
    return (leaked);
}

/*
** ---------------------------------------------------------------------
** Log helpers
** ---------------------------------------------------------------------
*/

static void print_cmds(char **cmds[])
{
    int i;
    int j;

    i = 0;
    while (cmds[i])
    {
        printf("  [%d] -> ", i);
        j = 0;
        while (cmds[i][j])
        {
            printf("\"%s\" ", cmds[i][j]);
            j++;
        }
        printf("\n");
        i++;
    }
}

static int run_case(const char *title, char **cmds[], int expected)
{
    int ret;
    int fds_before[FD_PROBE_LIMIT];
    int fds_after[FD_PROBE_LIMIT];
    int leaked;
    int fail;

    printf("========================================================\n");
    printf("TEST: %s\n", title);
    printf("cmds:\n");
    print_cmds(cmds);
    printf("--- running picoshell() ---\n");
    fflush(stdout);

    snapshot_fds(fds_before);
    ret = picoshell(cmds);
    snapshot_fds(fds_after);

    printf("--- picoshell() returned: %d (expected: %d) -> %s\n",
        ret, expected, ret == expected ? "OK" : "FAIL");

    leaked = report_fd_diff(fds_before, fds_after);
    if (leaked == 0)
        printf("--- fd check: no leaked file descriptors\n");
    else
        printf("--- fd check: %d leaked file descriptor(s)\n", leaked);

    fail = (ret != expected) || (leaked != 0);
    return (fail ? 1 : 0);
}

/*
** ---------------------------------------------------------------------
** Test cases
** each "cmds" is a NULL-terminated argv array,
** and the array of arrays is itself NULL-terminated.
** ---------------------------------------------------------------------
*/

int main(int argc, char **argv, char **envp)
{
    int fails;

    (void)argc;
    (void)argv;
    (void)envp;
    fails = 0;

    /* 1. Single command, no pipe */
    {
        char *c1[] = {"/bin/ls", NULL};
        char **cmds[] = {c1, NULL};

        fails += run_case("single command (ls)", cmds, 0);
    }

    /* 2. Simple pipeline: ls | grep picoshell */
    {
        char *c1[] = {"/bin/ls", NULL};
        char *c2[] = {"/usr/bin/grep", "picoshell", NULL};
        char **cmds[] = {c1, c2, NULL};

        fails += run_case("ls | grep picoshell", cmds, 0);
    }

    /* 3. 3-stage pipeline: echo | cat | sed (example from the assignment)
          NOTE: using bare "sed" (PATH lookup via execvp) instead of a
          hardcoded path, since /bin/sed doesn't exist on every system
          (e.g. macOS keeps it elsewhere). */
    {
        char *c1[] = {"/bin/echo", "squalala", NULL};
        char *c2[] = {"/bin/cat", NULL};
        char *c3[] = {"sed", "s/a/b/g", NULL};
        char **cmds[] = {c1, c2, c3, NULL};

        fails += run_case("echo squalala | cat | sed s/a/b/g", cmds, 0);
    }

    /* 4. Command that fails to execvp (bad path/name) mid-pipeline
          -> valid argv shape, but the binary doesn't exist: this is
          exactly the "if any error occur, return 1" case. */
    {
        char *c1[] = {"/bin/echo", "hello", NULL};
        char *c2[] = {"/bin/this_binary_does_not_exist", NULL};
        char *c3[] = {"/bin/cat", NULL};
        char **cmds[] = {c1, c2, c3, NULL};

        fails += run_case("echo hello | <execvp fails> | cat", cmds, 1);
    }

    /* 5. First command's exit code is non-zero (grep, no match) */
    {
        char *c1[] = {"/bin/echo", "abc", NULL};
        char *c2[] = {"/usr/bin/grep", "no_such_pattern", NULL};
        char **cmds[] = {c1, c2, NULL};

        fails += run_case("echo abc | grep no_such_pattern (no match)", cmds, 1);
    }

    /* 6. Longer pipeline (5 stages), stresses the dup2/close/prev_fd loop */
    {
        char *c1[] = {"/bin/echo", "aaaaa", NULL};
        char *c2[] = {"/bin/cat", NULL};
        char *c3[] = {"/bin/cat", NULL};
        char *c4[] = {"/bin/cat", NULL};
        char *c5[] = {"/usr/bin/wc", "-c", NULL};
        char **cmds[] = {c1, c2, c3, c4, c5, NULL};

        fails += run_case("echo aaaaa | cat | cat | cat | wc -c", cmds, 0);
    }

    /* 7. Middle stage produces no match / empty output, downstream
          stage should still run cleanly on empty input, not hang. */
    {
        char *c1[] = {"/bin/echo", "abc", NULL};
        char *c2[] = {"/usr/bin/grep", "no_such_pattern", NULL};
        char *c3[] = {"/bin/cat", NULL};
        char **cmds[] = {c1, c2, c3, NULL};

        fails += run_case("echo abc | grep no_such_pattern | cat "
            "(empty input downstream)", cmds, 1);
    }

    /* 8. Pipeline that would hang forever if the parent leaks a pipe
          write-end fd (reader never sees EOF). If your picoshell hangs
          here, it's very likely a missing close() in the parent after
          fork(), not a problem with this test. */
    {
        char *c1[] = {"/bin/echo", "line1", NULL};
        char *c2[] = {"/bin/cat", NULL};
        char *c3[] = {"/bin/cat", NULL};
        char *c4[] = {"/usr/bin/wc", "-l", NULL};
        char **cmds[] = {c1, c2, c3, c4, NULL};

        printf("========================================================\n");
        printf("TEST: hang guard (echo | cat | cat | wc -l)\n");
        printf("If this test never prints its result, picoshell() is\n");
        printf("blocking forever -- almost certainly an unclosed pipe fd\n");
        printf("in the parent process after fork().\n");
        fails += run_case("echo line1 | cat | cat | wc -l (hang guard)",
            cmds, 0);
    }

    printf("========================================================\n");
    if (fails == 0)
        printf("ALL TESTS PASSED\n");
    else
        printf("%d TEST(S) FAILED\n", fails);

    return (fails != 0);
}
