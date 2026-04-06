# C8L Language Reference
 
C8L is a compiled language targeting the CHIP-8 virtual machine.
Programs compile to a binary ROM loaded at address `0x200`.
All values are 8-bit. Registers are `v0`–`vF`. Addresses are 12-bit.
 
---
 
## Registers
 
| Name | Description |
|---|---|
| `v0`–`vE` | General purpose 8-bit registers |
| `vF` | Flag register — set automatically by arithmetic, draw, and shift operations. Avoid using as general storage. |
| `I` | 16-bit address register. Points to memory for draw, store, load. |
 
---
 
## Assignment
 
```
vX = N       # vX = immediate value (0–255)
vX = vY      # vX = vY
```
 
---
 
## Arithmetic
 
```
vX += N      # vX = vX + N          (no carry flag)
vX += vY     # vX = vX + vY         vF = carry
vX -= vY     # vX = vX - vY         vF = NOT borrow (1 if no underflow)
```
 
---
 
## Bitwise
 
```
vX |= vY     # vX = vX OR vY
vX &= vY     # vX = vX AND vY
vX ^= vY     # vX = vX XOR vY
vX >>= 1     # vX = vX >> 1         vF = shifted-out bit
vX <<= 1     # vX = vX << 1         vF = shifted-out bit
```
 
---
 
## Control Flow
 
```
label:                        # declares a jump target
 
goto label                    # unconditional jump
 
if vX == N  goto label        # jump if vX equals immediate
if vX == vY goto label        # jump if vX equals vY
if vX != N  goto label        # jump if vX not equals immediate
if vX != vY goto label        # jump if vX not equals vY
```
```
call label                    # call subroutine
ret                           # return from subroutine

if vX == N  goto label        # call if vX equals immediate
if vX == vY goto label        # call if vX equals vY
if vX != N  goto label        # call if vX not equals immediate
if vX != vY goto label        # call if vX not equals vY
```
 
## Input
 
CHIP-8 has a 16-key hex keypad (keys `0`–`F`).
 
```
waitkey vX          # block until any key pressed; store key index in vX
 
ifkey  vX goto label    # jump if key[vX] is currently pressed
ifnkey vX goto label    # jump if key[vX] is currently NOT pressed

ifkey  vX call label    # call if key[vX] is currently pressed
ifnkey vX call label    # call if key[vX] is currently NOT pressed
```
 
---
 
## Timers
 
Both timers count down at 60Hz. When the sound timer is nonzero, the buzzer sounds.
 
```
delayset vX     # delay timer = vX
delayget vX     # vX = delay timer
soundset vX     # sound timer = vX
```
 
Typical usage — wait for a timer to expire:
```
v0 = 10
delayset v0
wait:
delayget v0
if v0 != 0 goto wait
```
 
---

## Random Number Generation

To generate number use `rnd`:
```
rnd vX kk       # vX - register where the value will be stored; kk - 8-bit value
```
---
 
## Graphics
 
The screen is 64×32 pixels, monochrome. Drawing is XOR — drawing over a lit pixel turns it off. `vF` is set to `1` if any pixel was erased (collision).
 
```
sprite name {
    0b11110000
    0b10010000    # up to 15 rows, one byte per row (8 pixels wide)
}
 
I = name          # point I at sprite data
draw vX vY N      # draw N rows of sprite at screen position (vX, vY)
```
 
To clean screen you shall use `cls` word:

```
cls
```

---
 
## Memory
 
```
I = 0xNNN       # set I to literal address
I = name        # set I to sprite or label address
 
store vX        # write v0..vX into memory starting at I
load  vX        # read  v0..vX from memory starting at I
```
 
---
 
## Literals
 
| Format | Example | Notes |
|---|---|---|
| Decimal | `10`, `255` | |
| Hexadecimal | `0xFF`, `0x200` | |
| Binary | `0b11110000` | Useful for sprite rows |
 
---
 
## Comments
 
```
# this is a comment
v0 = 10   # inline comment
```
 
---
 
## vF Behaviour Summary
 
Many operations set `vF` as a side effect. If you use `vF` for storage, these will overwrite it silently.
 
| Operation | vF set to |
|---|---|
| `vX += vY` | `1` if carry, else `0` |
| `vX -= vY` | `1` if no borrow (vX >= vY), else `0` |
| `vX >>= 1` | bit shifted out (LSB before shift) |
| `vX <<= 1` | bit shifted out (MSB before shift) |
| `draw vX vY N` | `1` if any pixel erased, else `0` |