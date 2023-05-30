; ModuleID = '../tests/ir_generator/p2.c'
source_filename = "../tests/ir_generator/p2.c"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
  %m = alloca i32, align 4
  store i32 %0, ptr %m, align 4
  %a = alloca i32, align 4
  %n = alloca i32, align 4
  store i32 5, ptr %n, align 4
  %2 = load i32, ptr %m, align 4
  %3 = load i32, ptr %n, align 4
  %4 = icmp slt i32 %2, %3
  br i1 %4, label %5, label %7

5:                                                ; preds = %1
  %6 = load i32, ptr %m, align 4
  store i32 %6, ptr %a, align 4
  br label %9

7:                                                ; preds = %1
  %8 = load i32, ptr %n, align 4
  store i32 %8, ptr %a, align 4
  br label %9

9:                                                ; preds = %7, %5
  br label %10

10:                                               ; preds = %21, %9
  %11 = load i32, ptr %m, align 4
  %12 = load i32, ptr %n, align 4
  %13 = icmp slt i32 %11, %12
  br i1 %13, label %14, label %22

14:                                               ; preds = %10
  %15 = load i32, ptr %m, align 4
  %16 = add i32 %15, 10
  store i32 %16, ptr %m, align 4
  %17 = load i32, ptr %m, align 4
  %18 = icmp slt i32 %17, 30
  br i1 %18, label %19, label %21

19:                                               ; preds = %14
  %20 = load i32, ptr %m, align 4
  store i32 %20, ptr %a, align 4
  br label %21

21:                                               ; preds = %19, %14
  br label %10

22:                                               ; preds = %10
  %23 = load i32, ptr %a, align 4
  ret i32 %23
}
