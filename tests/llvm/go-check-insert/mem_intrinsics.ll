; RUN: opt -load-pass-plugin=%passes_dir/libGOIRPasses.so -passes=go-check-insert -S %s | FileCheck %s

%go.grade = type { i64, i64, i32, i32, i32, i32, i64, i32 }

define void @test_mem_intrinsics(ptr %dst, ptr %src, i64 %n) {
entry:
  ; CHECK: call void @llvm.go.check_store(ptr [[DST:%dst]], %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 [[N:%n]])
  ; CHECK: call void @llvm.go.check_load(ptr [[SRC:%src]], %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 [[N]])
  ; CHECK: call void @llvm.memcpy.p0.p0.i64(ptr [[DST]], ptr [[SRC]], i64 [[N]], i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %n, i1 false)

  ; CHECK: call void @llvm.go.check_store(ptr [[DST]], %go.grade { i64 0, i64 -1, i32 0, i32 0, i32 15, i32 0, i64 0, i32 0 }, i64 [[N]])
  ; CHECK: call void @llvm.memset.p0.i64(ptr [[DST]], i8 0, i64 [[N]], i1 false)
  call void @llvm.memset.p0.i64(ptr %dst, i8 0, i64 %n, i1 false)

  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)
declare void @llvm.go.check_load(ptr, %go.grade, i64)
declare void @llvm.go.check_store(ptr, %go.grade, i64)
