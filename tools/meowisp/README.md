# MeowISP

MeowISP is a pure-Rust first-flash desktop tool for BinaryKeyboard.

Current status:

- Cargo workspace under `tools/meowisp`
- Slint-based rounded, high-DPI desktop UI
- Embedded `wchisp`-powered flashing logic through `meowisp-core`
- Terminal debug output for raw transport / chip / error-chain diagnostics

Planned direction:

- Frontend: pure Rust + Slint
- Default backend: `wchisp`
- First milestone: CH592F first-flash flow using `full.bin`

## Build

```bash
cargo build --manifest-path tools/meowisp/Cargo.toml --bin meowisp
```

## Run

```bash
./tools/meowisp/target/debug/meowisp
./tools/meowisp/target/debug/meowisp --doctor
```
