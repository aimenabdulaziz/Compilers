//Common subexpression elimination
int func(int i, int j){
 	int a, b;
	
	a = i*j;
	b = i*j;	
	
	return(a+b); 
}
