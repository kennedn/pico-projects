.global gcd
 
gcd:
  push {lr}
  cmp r0, r1
  beq 2f
  blt 1f
  sub r0, r1
  bl gcd
1:
  sub r1, r0
  bl gcd
2:
  pop {pc}