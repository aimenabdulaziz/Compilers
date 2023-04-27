//Safety check for common subexpression elimination
int func(int i){
 	int a, b;
	
	a = i*10;
	i = 20;
	b = i*10;	
	
	return(a+b); 
}
