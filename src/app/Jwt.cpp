// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

#include "Jwt.h"

#include <QJsonDocument>

#include <memory>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>

QByteArray Jwt::base64UrlDecode(const QByteArray& text) {
    return QByteArray::fromBase64(text,
                                  QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QByteArray Jwt::base64UrlEncode(const QByteArray& bytes) {
    return bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

std::optional<Jwt> Jwt::parse(const QByteArray& token) {
    const QList<QByteArray> parts = token.trimmed().split('.');
    if (parts.size() != 3 || parts.at(0).isEmpty() || parts.at(1).isEmpty() ||
        parts.at(2).isEmpty()) {
        return std::nullopt;
    }
    const QJsonDocument header = QJsonDocument::fromJson(base64UrlDecode(parts.at(0)));
    const QJsonDocument payload = QJsonDocument::fromJson(base64UrlDecode(parts.at(1)));
    if (!header.isObject() || !payload.isObject()) {
        return std::nullopt;
    }
    Jwt jwt;
    jwt.header = header.object();
    jwt.payload = payload.object();
    jwt.signingInput = parts.at(0) + '.' + parts.at(1);
    jwt.signature = base64UrlDecode(parts.at(2));
    if (jwt.signature.isEmpty()) {
        return std::nullopt;
    }
    return jwt;
}

namespace {

struct BnFree {
    void operator()(BIGNUM* bn) const { BN_free(bn); }
};
struct ParamBldFree {
    void operator()(OSSL_PARAM_BLD* bld) const { OSSL_PARAM_BLD_free(bld); }
};
struct ParamFree {
    void operator()(OSSL_PARAM* params) const { OSSL_PARAM_free(params); }
};
struct PkeyCtxFree {
    void operator()(EVP_PKEY_CTX* ctx) const { EVP_PKEY_CTX_free(ctx); }
};
struct PkeyFree {
    void operator()(EVP_PKEY* key) const { EVP_PKEY_free(key); }
};
struct MdCtxFree {
    void operator()(EVP_MD_CTX* ctx) const { EVP_MD_CTX_free(ctx); }
};

std::unique_ptr<EVP_PKEY, PkeyFree> publicKeyFrom(const RsaPublicKey& key) {
    const std::unique_ptr<BIGNUM, BnFree> n(
        BN_bin2bn(reinterpret_cast<const unsigned char*>(key.modulus.constData()),
                  key.modulus.size(),
                  nullptr));
    const std::unique_ptr<BIGNUM, BnFree> e(
        BN_bin2bn(reinterpret_cast<const unsigned char*>(key.exponent.constData()),
                  key.exponent.size(),
                  nullptr));
    if (!n || !e) {
        return nullptr;
    }
    const std::unique_ptr<OSSL_PARAM_BLD, ParamBldFree> builder(OSSL_PARAM_BLD_new());
    if (!builder || !OSSL_PARAM_BLD_push_BN(builder.get(), OSSL_PKEY_PARAM_RSA_N, n.get()) ||
        !OSSL_PARAM_BLD_push_BN(builder.get(), OSSL_PKEY_PARAM_RSA_E, e.get())) {
        return nullptr;
    }
    const std::unique_ptr<OSSL_PARAM, ParamFree> params(OSSL_PARAM_BLD_to_param(builder.get()));
    const std::unique_ptr<EVP_PKEY_CTX, PkeyCtxFree> context(
        EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr));
    if (!params || !context || EVP_PKEY_fromdata_init(context.get()) <= 0) {
        return nullptr;
    }
    EVP_PKEY* raw = nullptr;
    if (EVP_PKEY_fromdata(context.get(), &raw, EVP_PKEY_PUBLIC_KEY, params.get()) <= 0) {
        return nullptr;
    }
    return std::unique_ptr<EVP_PKEY, PkeyFree>(raw);
}

} // namespace

bool Jwt::verifyRs256(const RsaPublicKey& key) const {
    if (header.value(QStringLiteral("alg")).toString() != QStringLiteral("RS256")) {
        return false;
    }
    const std::unique_ptr<EVP_PKEY, PkeyFree> pkey = publicKeyFrom(key);
    if (!pkey) {
        return false;
    }
    const std::unique_ptr<EVP_MD_CTX, MdCtxFree> context(EVP_MD_CTX_new());
    if (!context ||
        EVP_DigestVerifyInit(context.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) <= 0) {
        return false;
    }
    return EVP_DigestVerify(context.get(),
                            reinterpret_cast<const unsigned char*>(signature.constData()),
                            static_cast<size_t>(signature.size()),
                            reinterpret_cast<const unsigned char*>(signingInput.constData()),
                            static_cast<size_t>(signingInput.size())) == 1;
}
