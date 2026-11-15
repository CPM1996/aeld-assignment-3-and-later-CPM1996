# /dev/faulty OOPS analysis
```
#echo “hello_world” > /dev/faulty

Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x0000000096000044
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x04: level 0 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000044, ISS2 = 0x00000000
  CM = 0, WnR = 1, TnD = 0, TagAccess = 0
  GCS = 0, Overlay = 0, DirtyBit = 0, Xs = 0
user pgtable: 4k pages, 48-bit VAs, pgdp=0000000041b8f000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000
Internal error: Oops: 0000000096000044 [#1]  SMP
Modules linked in: faulty(O) scull(O)
CPU: 0 UID: 0 PID: 160 Comm: sh Tainted: G           O        6.18.7 #1 NONE 
Tainted: [O]=OOT_MODULE
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x4/0x10 [faulty]
lr : vfs_write+0xb8/0x3c0
sp : ffff800080f73d60
x29: ffff800080f73de0 x28: ffff000001bd0fc0 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040000000 x22: 0000ffff9ffd70e8 x21: 00000000ffffffff
x20: ffff000001c4b840 x19: ffff000001c4b840 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000001 x7 : ffff000001c4b840 x6 : ffff800078d49000
x5 : 0000000000000012 x4 : 0000aaaad8e18c20 x3 : ffff800080f73e00
x2 : 0000000000000012 x1 : 0000aaaad8e18c20 x0 : 0000000000000000
Call trace:
 faulty_write+0x4/0x10 [faulty] (P)
 ksys_write+0x64/0x100
 __arm64_sys_write+0x18/0x30
 invoke_syscall.constprop.0+0x40/0xf0
 el0_svc_common.constprop.0+0x38/0xe0
 do_el0_svc+0x1c/0x30
 el0_svc+0x34/0xf0
 el0t_64_sync_handler+0xa0/0xf0
 el0t_64_sync+0x198/0x19c
Code: ???????? ???????? ???????? d2800000 (b900001f) 
---[ end trace 0000000000000000 ]---
```
The command that triggers this oops is a write operation to the faulty device.
The first message indicates that the opps occured due to dereferencing a NULL adress. 
Finally the call trace indicates that the oops occured inside the faulty_write function.
