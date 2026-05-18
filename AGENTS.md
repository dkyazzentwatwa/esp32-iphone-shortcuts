# Repository Guidelines

## Project Structure & Module Organization
This repository contains six standalone Arduino sketches plus one shared helper header:

- `esp32_gpio_api/`: ESP32 single-pin HTTP GPIO API
- `esp32_gpio_api_all/`: ESP32 multi-pin variant with `/pins`
- `esp32_gpio_api_s3/`: ESP32-S3 single-pin variant
- `esp32_gpio_api_s3_all/`: ESP32-S3 multi-pin variant
- `esp32_c3_supermini_gpio_api_all/`: ESP32-C3 Super Mini multi-pin variant
- `esp32_gpio_api_universal/`: universal ESP32-family sketch with runtime chip detection
- `esp32_shortcut_api.h`: shared HTTP, Wi-Fi, mDNS, BLE, and GPIO route helpers

Generated Arduino build output lives in `.arduino-build*` directories and should not be treated as source. Keep changes scoped to the sketch folder you are updating.

## Build, Test, and Development Commands
Use Arduino IDE for the default workflow: open a sketch folder, select the matching board, then upload.

For CLI builds, `arduino-cli` is the fastest validation path:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app esp32_gpio_api
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app esp32_gpio_api_all
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app esp32_gpio_api_universal
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app esp32_gpio_api_s3
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app esp32_gpio_api_s3_all
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=huge_app esp32_c3_supermini_gpio_api_all
```

The discovery features use BLE and need the Huge APP partition. If an S3 compile fails with core-linker errors after switching board profiles, retry once with `--clean` to rebuild the Arduino core cache.

After flashing, use Serial Monitor to capture the assigned IP and verify endpoints such as `/health`, `/gpio/23/on`, or `/pins`.

## Coding Style & Naming Conventions
Match the existing Arduino/C++ style:

- Use two-space indentation and keep braces on the same line.
- Prefer `kCamelCase` for constants, `camelCase` for functions, and descriptive route/helper names such as `handleHealth()` or `registerPinRoutes()`.
- Keep board-specific pin allowlists and defaults near the top of each sketch.
- Favor small helper functions over inline route logic.

## Testing Guidelines
There is no automated test suite in this repository. Validation is compile-and-smoke-test based:

- Compile the changed sketch before submitting.
- Verify boot logs, Wi-Fi connection, and at least one `on`, `off`, and `status` request on hardware.
- If you change routes or JSON output, include example requests and responses in the PR notes.

## Commit & Pull Request Guidelines
Use short imperative commit messages, for example: `Add pin allowlist to ESP32-S3 sketch`.

PRs should state which sketch changed, which board was tested, the endpoints verified, and any hardware assumptions. Include screenshots only when sharing Serial Monitor output or Shortcut configuration is useful.

## Security & Configuration Tips
Do not commit real Wi-Fi credentials. Replace `ssid` and `password` with placeholders before opening a PR, and avoid expanding the safe output pin list without confirming board-level electrical safety.
