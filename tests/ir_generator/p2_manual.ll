; ModuleID = '../tests/ir_generator/p2.c'
source_filename = "../tests/ir_generator/p2.c"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
  %i = alloca i32, align 4
  store i32 %0, ptr %i, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  br label %2

2:                                                ; preds = %14, %1
  %3 = load i32, ptr %a, align 4
  %4 = load i32, ptr %i, align 4
  %5 = icmp slt i32 %3, %4
  br i1 %5, label %6, label %17

6:                                                ; preds = %2
  %a1 = alloca i32, align 4
  br label %7

7:                                                ; preds = %11, %6
  %8 = load i32, ptr %b, align 4
  %9 = load i32, ptr %i, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %14

11:                                               ; preds = %7
  %b2 = alloca i32, align 4
  %12 = load i32, ptr %b2, align 4
  %13 = add i32 %12, 20
  store i32 %13, ptr %b2, align 4
  br label %7

14:                                               ; preds = %7
  %15 = load i32, ptr %b, align 4
  %16 = add i32 10, %15
  store i32 %16, ptr %a1, align 4
  br label %2

17:                                               ; preds = %2
  %18 = load i32, ptr %a, align 4
  ret i32 %18
}
