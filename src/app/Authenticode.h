#pragma once

#include <QString>

#include <optional>

namespace Authenticode {

// The SHA-1 thumbprint (lower-case hex) of the certificate that signed a PE or MSI file, read
// from its embedded PKCS#7 signature, or nothing when the file carries none. This says who
// signed, not whether the chain is trusted; signtool verify answers that.
std::optional<QString> signerThumbprint(const QString& path);

} // namespace Authenticode
