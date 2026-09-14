# fragment loader

Desktop launcher for [fragment](https://fragment.lol). Handles the licence key,
keeps `fragment.exe` up to date, and starts it.

## Build

```
MSBuild examples\example_win32_directx11\example_win32_directx11.vcxproj /p:Configuration=Release /p:Platform=x64
```

Output: `build\FragmentLoader.exe`.

The project targets `v145`; retarget `PlatformToolset` for older toolsets. The build
embeds `examples\example_win32_directx11\payload\fragment.exe` as a resource, so that
file must exist first. A PreBuildEvent copies it from the sibling `fragment` repo
(`..\..\..\fragment\build\fragment.exe`); building the loader anywhere else means
dropping a `fragment.exe` into `payload\` yourself. `payload\` is gitignored.

## One file for users

`FragmentLoader.exe` is the only thing users download - the overlay is embedded inside
it. On startup the loader writes that payload to `%APPDATA%\fragment\fragment.exe`
when it is missing or a different size, then keeps it current from the update server.
The overlay never appears next to the loader, and the loader stays a single file.

## What it does

1. **Licence** — one field, key mode via `POST /api/verify`. The returned session is
   stored in `%APPDATA%\fragment\loader_session.dat` (XOR-masked with a
   MachineGuid-derived key) and re-verified in session mode on later launches, so the
   key is only typed once while it stays valid. A network blip never clears it; only a
   signed denial (expired / banned / hwid mismatch / invalid session) does.
2. **Updates** — `POST /api/latest` gives the manifest; the SHA-256 of the local
   overlay is compared against it and `POST /api/download` is used when they differ.
   Comparing hashes removes any need for a version stamp.
3. **Launch** — checks that `RobloxPlayerBeta.exe` is running, then starts
   `%APPDATA%\fragment\fragment.exe --session=<token> --no-update`:
   - **Load** — hidden console, stdout piped, waits for fragment's `fragment ready`
     line, then closes itself.
   - **Load with debugger** — visible console, stays open.

Auth responses are HMAC-verified against the same `RESPONSE_SECRET` baked into
fragment, so a spoofed server cannot hand out a session.

## Layout

```
examples/example_win32_directx11/
  main.cpp                 window, D3D11 device, single Roblox entry, main loop
  resource.rc/.h, .ico     exe icon and version info
  menu/
    bytes/                 embedded images (backgrounds, logo, Roblox tile, icon font)
    defs/                  colours (fragment palette), fonts, textures
    helpers/               drawing/anim helpers, texture loader, stb_image, json
    interface/
      menu_i.cpp           the UI: login screen + one main screen
      loader.cpp/.h        licence, update, payload extraction, launch, status
      net.cpp/.h           WinHTTP, SHA-256/HMAC, HWID
```

## Notes

- Images are decoded with `stb_image` into D3D11 textures; there is no DirectX SDK
  dependency.
- `fragment.exe` is launched with `--no-update` so it does not race the loader's own
  updater, and from `%APPDATA%\fragment` so the overlay's configs and scripts stay
  beside it.
- Licensed under the MIT licence inherited from Dear ImGui (see `LICENSE.txt`).
