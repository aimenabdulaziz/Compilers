extern void print(int);
extern int read();

int func(int i)
{
    int a;
    int b;
    int result;

    a = i * 10;
    b = i * 2;

    return (a + b);
}
// Expected to fail (miniC doesn't allow comments)