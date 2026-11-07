# signature-master

Digital signature tool for Sigmastone releases. A desktop application that runs on the machine
holding the code signing certificate and signs release files when asked over a REST API, so a
release job can stop, ask for a signature, and carry on with the signed files.

## How it works

- The application runs on the signing desktop as an ordinary process in the user's session, which
  is what lets the certificate's own PIN dialog appear.
- It opens a port and listens over a REST API. Release jobs send the files to sign there and fetch
  the signed copies back.
- Callers authenticate through GitHub.
- Files are signed with the certificate in the user's certificate store. Until the production
  certificate is in place, a temporary self-signed certificate is used; it is not trusted by
  anyone and must not be used for anything that is published.

## Settings

| Setting | Effect |
|---|---|
| Automatically sign | Requests that pass authentication are signed without a confirmation step. Off by default. |
| Remember PIN | The certificate PIN is kept in the operating system's credential store and supplied on request. Off by default. |

The PIN is never written to a settings file or a log.

## Building

The application is Qt 6 (QML and C++), built with CMake and Ninja. Every dependency, Qt included,
comes through vcpkg in manifest mode (`vcpkg.json`) rather than a Qt distribution. You need:

- Visual Studio 2026 (or Build Tools) with the C++ workload; CMake and Ninja ship with it.
- A vcpkg checkout at or after the baseline in `vcpkg.json`, with `VCPKG_ROOT` pointing at it.
  The first configure builds Qt from source, which takes an hour or more; later ones come from
  vcpkg's binary cache. Warming the cache ahead of time, from a developer shell:

  ```powershell
  vcpkg install --triplet x64-windows "qtbase[core,async-io,brotli,concurrent,dnslookup,doubleconversion,freetype,gui,harfbuzz,jpeg,network,opengl,openssl,pcre2,png,sessionmanager,testlib,thread,widgets,windeployqt,zstd]" qtdeclarative qthttpserver qtsvg openssl
  ```

From a Visual Studio developer shell:

```powershell
cmake --preset dev
cmake --build --preset dev
build\dev\signature-master.exe
```

`dev` is RelWithDebInfo with a console; `debug` and `release` exist too. Each build runs
`windeployqt`, so the executable runs straight from the build directory. A `CMakeUserPresets.json`
(ignored by git) is the place for a preset that sets `VCPKG_ROOT` for your machine.

## Code style

C++ and QML are formatted with clang-format, using the `.clang-format` at the repository root.

## Status

The application shell: the window with its three sections, the settings, the tray icon and a REST
listener answering `GET /health`. Requests, signing, the log and GitHub authentication are not
built yet.
