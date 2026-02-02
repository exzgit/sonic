debug
```bash
gdb -batch -ex "set pagination off" -ex "set debuginfod enabled off" -ex run -ex "frame 5" -ex "info locals" --args ./build/sonic compile examples 2>&1

```
