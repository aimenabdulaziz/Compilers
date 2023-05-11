; ModuleID = '../tests/ir_generator/p1.c'
source_filename = "../tests/ir_generator/p1.c"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
  %i = alloca i32, align 4
  store i32 %0, ptr %i, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %2 = load i32, ptr %a, align 4
  %3 = load i32, ptr %i, align 4
  %4 = icmp slt i32 %2, %3
  br i1 %4, label %5, label %16

5:                                                ; preds = %1
  br label %6

6:                                                ; preds = %10, %5
  %7 = load i32, ptr %b, align 4
  %8 = load i32, ptr %i, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %13

10:                                               ; preds = %6
  %11 = load i32, ptr %b, align 4
  %12 = add i32 %11, 20
  store i32 %12, ptr %b, align 4
  br label %6

13:                                               ; preds = %6
  %14 = load i32, ptr %b, align 4
  %15 = add i32 10, %14
  store i32 %15, ptr %a, align 4
  br label %23

16:                                               ; preds = %1
  %17 = load i32, ptr %b, align 4
  %18 = load i32, ptr %i, align 4
  %19 = icmp slt i32 %17, %18
  br i1 %19, label %20, label %22

20:                                               ; preds = %16
  %21 = load i32, ptr %a, align 4
  store i32 %21, ptr %b, align 4
  br label %22

22:                                               ; preds = %20, %16
  br label %23

23:                                               ; preds = %22, %13
  %24 = load i32, ptr %a, align 4
  ret i32 %24
}
