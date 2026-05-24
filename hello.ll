; ModuleID = 'opticoc'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"


define void @main() {
  %t1 = alloca i32
  store i32 42, i32* %t1
  ret void
}
