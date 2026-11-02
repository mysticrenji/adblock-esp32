Place the generated `blocklist.bin` here before running `pio run -t uploadfs`.

The binary is intentionally not committed. Build it from a reviewed, local list:

```sh
python3 tools/build_blocklist.py data/blocklist.bin path/to/reviewed-domains.txt
```
