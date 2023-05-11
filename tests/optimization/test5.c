//Common subexpression elimination
int func(int i){
 	int a, b, c;
	
	a = i*10;
	b = i*10;	
	c = i*b;

	return(a+b); 
}
