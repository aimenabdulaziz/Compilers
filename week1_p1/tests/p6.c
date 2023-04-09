extern void print(int); 
extern int read();

int func (int i) {
    int a;
    int b;
    int result;

    a = i * 10;
    b = i * (2 * 10);

    if (a == 10) {
        result = 20;
    }
    
    while (a < b) {
        a = 10 + i;
        print(3);
    }

    if (a > b) {
        result = a; 
    }
    else {
        result = b;
    }

    return result;
}
