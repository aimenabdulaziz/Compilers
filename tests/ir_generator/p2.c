extern void print(int);
extern int read();
int func(int m)
{
	int a;
	int n;

	n = 5;

	if (m < n)
	{
		a = m;
	}
	else
	{
		a = n;
	}

	while (m < n)
	{
		m = m + 10;
		if (m < 30)
		{
			a = m;
		}
	}

	return (a);
}
