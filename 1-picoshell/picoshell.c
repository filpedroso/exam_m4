#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

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
t_state	get_state(char **cmds[], int i);

int	picoshell(char **cmds[])
{
	int		i			= -1;
	int		last_read	= -1;
	int		ret			= 0;

	t_state	state;
	int		pip[2];
	int		res;
	int		stat_loc;
	pid_t	pid;

	while (cmds[++i])
	{
		state = get_state(cmds, i);
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
			if (last_read != -1)
				close(last_read);
			last_read = pip[0];
			close(pip[1]);
		}
	}
	close(pip[0]);

	while(1)
	{
		res = wait(&stat_loc);
		if (WEXITSTATUS(stat_loc) == 1)
			ret = 1;
		if (res == -1)
			break;
	}
	return (ret);
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
		close(last_read);
		exit(1);
	}
	else if (state == LAST)
	{
		close(pip[1]);
		dup2(last_read, STDIN_FILENO);
		execvp(argv[0], argv);
		close(last_read);
		exit(1);
	}
}

t_state	get_state(char **cmds[], int i)
{
	if (cmds[i + 1] == NULL)
		return (LAST);
	else if (i == 0)
		return (FIRST);
	else
		return (MID);
}
#include <stdio.h>

int picoshell(char **cmds[]);

static int run_case(const char *title, char **cmds[], int expected)
{
    int ret;

    printf("TEST: %s\n", title);
    ret = picoshell(cmds);
    printf("  returned: %d (expected: %d) -> %s\n",
        ret, expected, ret == expected ? "OK" : "FAIL");
    return (ret == expected ? 0 : 1);
}

int main(void)
{
    int fails;

    fails = 0;

    /* 1. Single command, no pipe */
    {
        char *c1[] = {"ls", NULL};
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

    /* 3. 3-stage pipeline: echo | cat | sed */
    {
        char *c1[] = {"/bin/echo", "squalala", NULL};
        char *c2[] = {"/bin/cat", NULL};
        char *c3[] = {"sed", "s/a/b/g", NULL};
        char **cmds[] = {c1, c2, c3, NULL};

        fails += run_case("echo squalala | cat | sed s/a/b/g", cmds, 0);
    }

    /* 4. Command that fails to execvp mid-pipeline */
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

    /* 6. Longer pipeline (5 stages) */
    {
        char *c1[] = {"/bin/echo", "aaaaa", NULL};
        char *c2[] = {"/bin/cat", NULL};
        char *c3[] = {"/bin/cat", NULL};
        char *c4[] = {"/bin/cat", NULL};
        char *c5[] = {"/usr/bin/wc", "-c", NULL};
        char **cmds[] = {c1, c2, c3, c4, c5, NULL};

        fails += run_case("echo aaaaa | cat | cat | cat | wc -c", cmds, 0);
    }

    /* 7. Middle stage produces no match / empty output */
    {
        char *c1[] = {"/bin/echo", "abc", NULL};
        char *c2[] = {"/usr/bin/grep", "no_such_pattern", NULL};
        char *c3[] = {"/bin/cat", NULL};
        char **cmds[] = {c1, c2, c3, NULL};

        fails += run_case("echo abc | grep no_such_pattern | cat", cmds, 1);
    }

    /* 8. Pipeline that would hang if a pipe fd leaks in the parent */
    {
        char *c1[] = {"/bin/echo", "line1", NULL};
        char *c2[] = {"/bin/cat", NULL};
        char *c3[] = {"/bin/cat", NULL};
        char *c4[] = {"/usr/bin/wc", "-l", NULL};
        char **cmds[] = {c1, c2, c3, c4, NULL};

        fails += run_case("echo line1 | cat | cat | wc -l", cmds, 0);
    }

    if (fails == 0)
        printf("ALL TESTS PASSED\n");
    else
        printf("%d TEST(S) FAILED\n", fails);

    return (fails != 0);
}






// #include <stdio.h>
// #include <string.h>
// #include <sys/wait.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <stdlib.h>

// int picoshell(char **cmds[]);

// /*
// ** ---------------------------------------------------------------------
// ** Log helpers
// ** ---------------------------------------------------------------------
// */

// static void print_cmds(char **cmds[])
// {
//     int i;
//     int j;

//     i = 0;
//     while (cmds[i])
//     {
//         printf("  [%d] -> ", i);
//         j = 0;
//         while (cmds[i][j])
//         {
//             printf("\"%s\" ", cmds[i][j]);
//             j++;
//         }
//         printf("\n");
//         i++;
//     }
// }

// /*
// ** Prints the captured pipeline output, quoting it so trailing
// ** newlines / empty output are visible and unambiguous in the log.
// */
// static void print_captured_output(const char *buf, ssize_t len)
// {
//     printf("--- pipeline output (%zd bytes): ---\n", len);
//     if (len == 0)
//     {
//         printf("(empty)\n");
//         return ;
//     }
//     printf(">>>\n");
//     fwrite(buf, 1, (size_t)len, stdout);
//     if (len > 0 && buf[len - 1] != '\n')
//         printf("\n(no trailing newline)\n");
//     printf("<<<\n");
// }

// /*
// ** ---------------------------------------------------------------------
// ** Runs picoshell() while capturing whatever it writes to stdout
// ** (i.e. whatever the last command in the pipeline produces).
// **
// ** Works by redirecting fd 1 to a temp file for the duration of the
// ** call, then restoring the original fd 1 and reading the temp file
// ** back. This does not touch picoshell()'s internals at all -- from
// ** its point of view STDOUT_FILENO just happens to point at a file
// ** instead of the terminal, exactly like real shell redirection.
// ** ---------------------------------------------------------------------
// */
// #define CAPTURE_BUF_SIZE 65536

// static int run_case(const char *title, char **cmds[], int expected)
// {
//     int ret;
//     int saved_stdout;
//     int tmp_fd;
//     char tmp_path[] = "/tmp/picoshell_test_XXXXXX";
//     static char capture_buf[CAPTURE_BUF_SIZE];
//     ssize_t captured_len;

//     printf("========================================================\n");
//     printf("TEST: %s\n", title);
//     printf("cmds:\n");
//     print_cmds(cmds);
//     printf("--- running picoshell() ---\n");
//     fflush(stdout);

//     /* Redirect our own stdout to a temp file so we can capture
//        whatever picoshell()'s last pipeline stage writes. */
//     tmp_fd = mkstemp(tmp_path);
//     if (tmp_fd == -1)
//     {
//         printf("--- (could not create temp file, skipping capture) ---\n");
//         ret = picoshell(cmds);
//     }
//     else
//     {
//         saved_stdout = dup(STDOUT_FILENO);
//         dup2(tmp_fd, STDOUT_FILENO);
//         close(tmp_fd);

//         ret = picoshell(cmds);

//         fflush(stdout);
//         dup2(saved_stdout, STDOUT_FILENO);
//         close(saved_stdout);

//         /* Read the temp file back into memory and print it through
//            our (now restored) real stdout. */
//         tmp_fd = open(tmp_path, O_RDONLY);
//         captured_len = 0;
//         if (tmp_fd != -1)
//         {
//             captured_len = read(tmp_fd, capture_buf, CAPTURE_BUF_SIZE - 1);
//             if (captured_len < 0)
//                 captured_len = 0;
//             close(tmp_fd);
//         }
//         unlink(tmp_path);
//         print_captured_output(capture_buf, captured_len);
//     }

//     printf("--- picoshell() returned: %d (expected: %d) -> %s\n",
//         ret, expected, ret == expected ? "OK" : "FAIL");
//     return (ret == expected ? 0 : 1);
// }

// /*
// ** ---------------------------------------------------------------------
// ** Test cases
// ** each "cmds" is a NULL-terminated argv array,
// ** and the array of arrays is itself NULL-terminated.
// ** ---------------------------------------------------------------------
// */

// int main(int argc, char **argv, char **envp)
// {
//     int fails;

//     (void)argc;
//     (void)argv;
//     (void)envp;
//     fails = 0;

//     /* 1. Single command, no pipe */
//     {
//         char *c1[] = {"/bin/ls", NULL};
//         char **cmds[] = {c1, NULL};

//         fails += run_case("single command (ls)", cmds, 0);
//     }

//     /* 2. Simple pipeline: ls | grep picoshell */
//     {
//         char *c1[] = {"/bin/ls", NULL};
//         char *c2[] = {"/usr/bin/grep", "picoshell", NULL};
//         char **cmds[] = {c1, c2, NULL};

//         fails += run_case("ls | grep picoshell", cmds, 0);
//     }

//     /* 3. 3-stage pipeline: echo | cat | sed (example from the assignment)
//           NOTE: using bare "sed" (PATH lookup via execvp) instead of a
//           hardcoded path, since /bin/sed doesn't exist on every system
//           (e.g. macOS keeps it elsewhere). */
//     {
//         char *c1[] = {"/bin/echo", "squalala", NULL};
//         char *c2[] = {"/bin/cat", NULL};
//         char *c3[] = {"sed", "s/a/b/g", NULL};
//         char **cmds[] = {c1, c2, c3, NULL};

//         fails += run_case("echo squalala | cat | sed s/a/b/g", cmds, 0);
//     }

//     /* 4. Command that fails to execvp (bad path/name) mid-pipeline
//           -> valid argv shape, but the binary doesn't exist: this is
//           exactly the "if any error occur, return 1" case. */
//     {
//         char *c1[] = {"/bin/echo", "hello", NULL};
//         char *c2[] = {"/bin/this_binary_does_not_exist", NULL};
//         char *c3[] = {"/bin/cat", NULL};
//         char **cmds[] = {c1, c2, c3, NULL};

//         fails += run_case("echo hello | <execvp fails> | cat", cmds, 1);
//     }

//     /* 5. First command's exit code is non-zero (grep, no match) */
//     {
//         char *c1[] = {"/bin/echo", "abc", NULL};
//         char *c2[] = {"/usr/bin/grep", "no_such_pattern", NULL};
//         char **cmds[] = {c1, c2, NULL};

//         fails += run_case("echo abc | grep no_such_pattern (no match)", cmds, 1);
//     }

//     /* 6. Longer pipeline (5 stages), stresses the dup2/close/prev_fd loop */
//     {
//         char *c1[] = {"/bin/echo", "aaaaa", NULL};
//         char *c2[] = {"/bin/cat", NULL};
//         char *c3[] = {"/bin/cat", NULL};
//         char *c4[] = {"/bin/cat", NULL};
//         char *c5[] = {"/usr/bin/wc", "-c", NULL};
//         char **cmds[] = {c1, c2, c3, c4, c5, NULL};

//         fails += run_case("echo aaaaa | cat | cat | cat | wc -c", cmds, 0);
//     }

//     /* 7. Middle stage produces no match / empty output, downstream
//           stage should still run cleanly on empty input, not hang. */
//     {
//         char *c1[] = {"/bin/echo", "abc", NULL};
//         char *c2[] = {"/usr/bin/grep", "no_such_pattern", NULL};
//         char *c3[] = {"/bin/cat", NULL};
//         char **cmds[] = {c1, c2, c3, NULL};

//         fails += run_case("echo abc | grep no_such_pattern | cat "
//             "(empty input downstream)", cmds, 1);
//     }

//     /* 8. Pipeline that would hang forever if the parent leaks a pipe
//           write-end fd (reader never sees EOF). If your picoshell hangs
//           here, it's very likely a missing close() in the parent after
//           fork(). Run this whole binary under `timeout` to avoid a
//           frozen terminal, e.g.: timeout 5 ./picoshell_test */
//     {
//         char *c1[] = {"/bin/echo", "line1", NULL};
//         char *c2[] = {"/bin/cat", NULL};
//         char *c3[] = {"/bin/cat", NULL};
//         char *c4[] = {"/usr/bin/wc", "-l", NULL};
//         char **cmds[] = {c1, c2, c3, c4, NULL};

//         fails += run_case("echo line1 | cat | cat | wc -l (hang guard)",
//             cmds, 0);
//     }

//     printf("========================================================\n");
//     if (fails == 0)
//         printf("ALL TESTS PASSED\n");
//     else
//         printf("%d TEST(S) FAILED\n", fails);

//     return (fails != 0);
// }
