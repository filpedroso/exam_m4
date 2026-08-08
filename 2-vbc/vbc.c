#include <stdio.h>
#include <ctype.h>

typedef struct s_parser
{
	char	*input;
	int		i;
	int		error;
} t_parser;

int	parse_expr(t_parser *pars);
int	parse_term(t_parser *pars);
int	parse_factor(t_parser *pars);
char	ch(t_parser *pars);
void	print_error(t_parser *pars);


void	advance(t_parser *pars)
{
	pars->i += 1;
}



int main (int argc, char **argv)
{
	t_parser	pars;
	int			result;

	if (argc != 2)
		return (1);
	
	pars.input	= argv[1];
	pars.i		= 0;
	pars.error	= 0;

	result = parse_expr(&pars);
	if (pars.error)
	{
		print_error(&pars);
		return (1);
	}
	printf("%i\n", result);
	return (0);
}

int	parse_expr(t_parser *pars)
{
	int	value;

	value = parse_term(pars);
	if (pars->error)
		return (0);

	while(ch(pars) == '+')
	{
		value = value + parse_term(pars);
		if (pars->error)
			return (0);
	}
	return (value);
}

int	parse_term(t_parser *pars)
{
	int	value;

	value = parse_factor(pars);
	if (pars->error)
		return (0);

	while(ch(pars) == '*')
	{
		value = value * parse_factor(pars);
		if (pars->error)
			return (0);
	}

	return (value);
}

int	parse_factor(t_parser *pars)
{

	if (ch(pars) ==  '\0')
	{
		pars->error = 1;
		return (0);
	}

	if (isdigit(ch(pars)))
	{
		advance(pars);
		return (ch(pars) - '0');
	}

	if (ch(pars) == '(')
	{
		int value;

		advance(pars);
		if (ch(pars) == '\0')
		{
			pars->error = 1;
			return (0);
		}
		value = parse_expr(pars);
		if (pars->error || ch(pars) != ')')
		{
			pars->error = 1;
			return (0);
		}
		return (value);
	}
	return (1);
}

char	ch(t_parser *pars)
{
	return (pars->input[pars->i]);
}

void	print_error(t_parser *pars)
{
	char	c = ch(pars);

	if (c == '\0')
		printf("Unexpected end of input\n");
	else
		printf("Unexpected token '%c'\n", c);
}


// int ft_factor()
// {
//     int n = 0;
//     if(isdigit(s))
//         return(s++ - '0');
//     while(s == '(')
//     {
//         s++;
//         n = ft_sum();
//         s++;
//     }
//     return(n);
// }