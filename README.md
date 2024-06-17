# ACEE: ARM Concolic Execution Engine
Gonna be a powerful concolic execution tool against arm assembly.

## Environment setup
For mac user, add the libz3.dylib path to DYLD_LIBRARY_PATH
```
export DYLD_LIBRARY_PATH=/Library/Developer/CommandLineTools/Library/Frameworks/Python3.framework/Versions/3.9/lib:$DYLD_LIBRARY_PATH
```

### Installing Z3
From anywhere, install z3 by following these steps

```
git clone https://github.com/Z3Prover/z3.git
cd z3//
python scripts/mk_make.py
cd build
make
sudo make install
```



## Quick start
```
make run <input_file>.s
```
ACEE will create a logfile under the name `acee_<input_file>.log`.



## Todo
### Support of differrnt indexing mode  

Now support writeback mode
Example: `str r0, [r1, #4]!` - This stores the word in r0 at the address (r1 + 4), and then adds 4 to r1.

#### Immediate offset / Pre-indexed addressing mode(supported)

In this mode, an immediate offset is added to or subtracted from the base register to form the memory address.

Example: `ldr r0, [r1, #4]` - This loads the word at the address (r1 + 4) into r0.

#### Register offset / Pre-indexed addressing mode(supported)

In this mode, the value of an offset register is added to or subtracted from the base register to form the memory address.

Example: `ldr r0, [r1, r2]` - This loads the word at the address (r1 + r2) into r0.

#### Scaled register offset / Pre-indexed addressing mode

In this mode, the value of an offset register is optionally shifted by a constant amount and then added to or subtracted from the base register to form the memory address.

Example: `ldr r0, [r1, r2, lsl #2]` - This loads the word at the address (r1 + (r2 << 2)) into r0.

#### Post-indexed addressing mode

In this mode, the base register contains the memory address for the current operation. After the operation, an offset is added to or subtracted from the base register.

Example: `ldr r0, [r1], #4` - This loads the word at the address in r1 into r0, and then adds 4 to r1.

#### Post-indexed register offset addressing mode

In this mode, the base register contains the memory address for the current operation. After the operation, the value of an offset register is added to or subtracted from the base register.

Example: `ldr r0, [r1], r2` - This loads the word at the address in r1 into r0, and then adds the value in r2 to r1.