extern int read();
extern void print(int);

int func(int i)
{
	int a;
	int b;

	a = 10;

	if (a + i < 100)
	{
		b = a + 100;
		print(b);
	}
	else
	{
		b = a + 20;
	}

	return (b);
}