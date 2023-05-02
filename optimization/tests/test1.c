//Common subexpression elimination
int func(int i){
 	int a, b;
	
	a = i*10;
	b = i*10;	
	
	return(a+b); 
}

  %2 = alloca i32, align 4
  %3 = alloca i32, align 4 // a
  %4 = alloca i32, align 4
  store i32 %0, i32* %2, align 4 // i
  %5 = load i32, i32* %2, align 4
  %6 = mul nsw i32 %5, 10 // i*10
  store i32 %6, i32* %3, align 4 // a = i*10
  %7 = load i32, i32* %2, align 4
  %8 = mul nsw i32 %7, 10
  store i32 %8, i32* %4, align 4 // b = i * 10
  %9 = load i32, i32* %3, align 4
  %10 = load i32, i32* %4, align 4
  %11 = add nsw i32 %9, %10
  ret i32 %11