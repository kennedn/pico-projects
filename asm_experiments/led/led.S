.global setup_pins 
.global toggle_pins
 
.data
  GPIO_FUNC_SELECT_BASE: .word 0x40014000
  GPIO_SIO_BASE: .word 0xd0000000

.text
setup_pins:
  push {r4, r5, r6, lr}
  //lsl r1, #2                          // r1 *= 4;                // convert array count to addr offset (i*4)
  mov r6, #0                          // r6 = 0;
1:
  cmp r1, #0                          // if (r1 == 0)
  beq 2f                              //   return;

  sub r1, #1                          // r1 -= 1;
  ldrb r4, [r0, r1]                   // r4 = *(r0 + r1)         // Get array element at offset
  
  ldr r5, =GPIO_FUNC_SELECT_BASE      // r5 = 0xd0000000;
  ldr r5, [r5]                        
  mov r2, #5                          // r2 = 5;                 // SIO function select value
  lsl r3, r4, #3                      // r3 = (r4 << 3) + 0x4;   // Calculate function select offset (pin * 0x8 + 0x4)
  add r3, #4
  str r2, [r5, r3]                    // *(0xd0000000 + r3) = r2 // Set pin function select to SIO
  mov r2, #1                          // r6 |= (1 << r2);        // Update mask with current pin  
  lsl r2, r4
  orr r6, r2
  b 1b

2:
  ldr r5, =GPIO_SIO_BASE              // r5 = 0x40014000;
  ldr r5, [r5]
  str r6, [r5, #0x24]                 // *(0x40014000 + 0x24) = r6 // Enable output for pin mask
  pop {r4, r5, r6, pc}

.equ DELAY, 0x555555
toggle_pins:
  push {r4, r5, r6, lr}
  ldr r5, =GPIO_SIO_BASE              // r5 = 0x40014000;
  ldr r5, [r5]
  cmp r2, #0                          // if (r2 == 0)              // Perform forward or backward looping based on direction(r2)
  beq 2f                              //   backward_loop();  
  mov r6, r1                          
  mov r1, #0
  sub r1, #1
1: // -- Forward loop
  cmp r2, #0                          // if (r2 == 0)
  beq 2f                              //   backward_loop

  cmp r1, r6                          // if (r1 == r6)
  beq 5f                              //   return;
  add r1, #1                          // r1++
  b 3f
2: // -- Backward loop
  cmp r1, #0                          // if (r1 == 0)
  beq 5f                              //   return;
  sub r1, #1                          // r1--;

3: // -- Main
  ldrb r4, [r0, r1]                   // r4 = *(r0 + r1)         // Get array element at offset
  
  mov r3, #1                          // r2 = 1 << r4;           // Set pin mask (1 << pin)
  lsl r3, r4
  str r3, [r5, #0x1c]                 // *(0x40014000 + 0x1c) = r3 // Set pin value with mask

  ldr r3, =DELAY                      // r3 = DELAY              // Get delay
4: // -- Delay
  sub  r3, #1                         // while(r3 > 0){r3--;};   // Delay for a set time                      
  bne 4b
  b 1b
5: // -- Return
  pop {r4, r5, r6, pc}