; RUN: %opt -passes=go-init -S %s 2>&1 | FileCheck %s

; CHECK: GoInitPass running on module: {{.*}}

define i32 @main() {
  ret i32 0
}
