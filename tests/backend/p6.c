extern int read();
extern void print(int);

int func(int i)
{
	int a;
	int b;

	a = 10 + i;

	if (a < 100)
	{
		b = a + 100;
		a = a + i;
	}
	else
	{
		b = a + 20;
	}

	return (b);
}