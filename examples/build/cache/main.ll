; ModuleID = 'sonic_module'
source_filename = "sonic_module"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define internal i64 @sn_main.cast() {
sn_entry_0:
  ret i64 0
}

define internal void @main() {
sn_entry_1:
  %a = alloca i128, align 16
  store i128 1, ptr %a, align 16
  %b = alloca i128, align 16
  %a1 = load i128, ptr %a, align 16
  store i128 %a1, ptr %b, align 16
  ret void
}
