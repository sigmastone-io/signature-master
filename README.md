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

## Packaging

```powershell
.\packaging\windows\build-msi.ps1
```

Builds the `release` preset, installs it into `build\stage` (the executable, the Qt runtime, the
libraries vcpkg built and the Visual C++ runtime, flat), and packages that with WiX 5.0.2 into
`build\msi\signature-master-<version>-x64.msi`: per machine, `Program Files\Signature Master`, a
Start menu shortcut, and a running desk closed before an upgrade. The WiX tool is installed on
demand as a .NET global tool; WiX 6 and later are not used (their maintenance-fee EULA is not
accepted).

The icon is `src/qml/icons/app.svg`; `packaging/windows/signature-master.ico` is rendered from it
with `cmake --build --preset dev --target icon` and committed, since the executable's resources
and the MSI both carry it.

## Code style

C++ and QML are formatted with clang-format, using the `.clang-format` at the repository root.

## Documents

- `doc/01. gui-design.md`: the window, its sections, states, colours, tray and dialogs.
- `doc/02. rest-api.md`: the routes a release job calls, the manifest, the request and its checks.

## Tests

```powershell
ctest --test-dir build\dev --output-on-failure
```

`tst_api` drives a real listener on a loopback port through every route; `tst_signing` takes
requests through approval, signing and the log with a fake signer. Signing with the real
signtool is checked by hand: "Make a test certificate" in Settings, a request from a script, Sign.

## Status

The window, the settings, the tray icon, the REST API with the request store and its checks, the
request list and detail with Sign and Reject, signing with signtool (sign, check the signer,
verify), automatic signing for requests whose every check passed, and the signature log with its
viewer and CSV export. The stored PIN and GitHub authentication are not built yet; every caller
counts as unverified until the latter is, which keeps automatic signing off for them.
