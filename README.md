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

The application is Qt 6, QML and C++, built with CMake. Dependencies, Qt included, come through
vcpkg rather than a Qt distribution; point CMake at the vcpkg toolchain file
(`<vcpkg root>/scripts/buildsystems/vcpkg.cmake`). On the development machine the vcpkg root is
`E:\vcpkg`.

Build instructions follow with the first code.

## Code style

C++ and QML are formatted with clang-format, using the `.clang-format` at the repository root.

## Status

Repository and this README only. No code yet.
