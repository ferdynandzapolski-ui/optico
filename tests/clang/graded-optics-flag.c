// RUN: %goir-clang -fgraded-optics=diag -O0 -S -emit-llvm %s -o - 2>&1 | FileCheck %s --check-prefix=DIAG
// RUN: %goir-clang -fgraded-optics=hybrid -O0 -S -emit-llvm %s -o - 2>&1 | FileCheck %s --check-prefix=HYBRID
// RUN: %goir-clang -fgraded-optics=cap -O0 -S -emit-llvm %s -o - 2>&1 | FileCheck %s --check-prefix=CAP

// RUN: %goir-clang -fgraded-optics=diag -O0 %s -o %t
// RUN: readelf -d %t | FileCheck %s --check-prefix=LINK

// DIAG: GoInitPass running on module: {{.*}} (Tier: 0)
// DIAG: !{i32 1, !"go-tier", i32 0}

// HYBRID: GoInitPass running on module: {{.*}} (Tier: 1)
// HYBRID: !{i32 1, !"go-tier", i32 1}

// CAP: GoInitPass running on module: {{.*}} (Tier: 2)
// CAP: !{i32 1, !"go-tier", i32 2}

// LINK: (NEEDED) Shared library: [libgoirrt.so]

int main() {
    return 0;
}
