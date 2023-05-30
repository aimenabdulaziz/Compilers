extern int read();
extern void print(int);

int func(int n)
{
	int a;
	int b;
	int c;
	int d;

	a = n * 5;
	b = n + 5;
	c = a + b;
	d = c + n;
	n = b + a;
	return d;
}
