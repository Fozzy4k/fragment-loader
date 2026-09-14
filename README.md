# fragment loader

Desktop launcher for [fragment](https://fragment.lol). Handles the licence key,
keeps `fragment.exe` up to date, and starts it.

## Build

```
MSBuild examples\example_win32_directx11\example_win32_directx11.vcxproj /p:Configuration=Release /p:Platform=x64
```

Output: `examples\example_win32_directx11\Release\FragmentLoader.exe`.
The project targets `v145`; retarget `PlatformToolset` for older toolsets.

## Runtime layout

Both executables sit in the same folder:

```
FragmentLoader.exe   this loader
fragment.exe         the overlay (downloaded and updated by the loader)
```

`FragmentLoader.exe` is deliberately **not** named `fragment.exe`.

## What it does

1. **Licence** — one field, key mode via `POST /api/verify`. The returned session is
   stored in `%APPDATA%\fragment\loader_session.dat` (XOR-masked with a
   MachineGuid-derived key) and re-verified in session mode on later launches, so the
   key is only typed once while it stays valid.
2. **Updates** — `POST /api/latest` gives the manifest; the SHA-256 of the local
   `fragment.exe` is compared against it and `POST /api/download` is used when they
   differ. Comparing hashes removes any need for a version stamp.
3. **Launch** — checks that `RobloxPlayerBeta.exe` is running, then starts
   `fragment.exe --session=<token> --no-update`:
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
      loader.cpp/.h        licence, update, launch, status
      net.cpp/.h           WinHTTP, SHA-256/HMAC, HWID
```

## Notes

- Images are decoded with `stb_image` into D3D11 textures; there is no DirectX SDK
  dependency.
- `fragment.exe` is launched with `--no-update` so it does not race the loader's own
  updater.
- Licensed under the MIT licence inherited from Dear ImGui (see `LICENSE.txt`).
