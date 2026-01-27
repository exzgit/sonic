debug
```bash
gdb -batch -ex "set pagination off" -ex "set debuginfod enabled off" -ex run -ex "frame 5" -ex "info locals" --args ./build/sonic examples/src/main.sn 2>&1 | tail -30

```
