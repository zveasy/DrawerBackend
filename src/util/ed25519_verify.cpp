#include "util/ed25519_verify.hpp"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <vector>
#include <memory>

namespace ed25519 {

static std::vector<unsigned char> b64decode(const std::string& in) {
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO* mem = BIO_new_mem_buf(in.data(), in.size());
  mem = BIO_push(b64, mem);
  std::vector<unsigned char> out(in.size());
  int len = BIO_read(mem, out.data(), out.size());
  if (len < 0) len = 0;
  out.resize(len);
  BIO_free_all(mem);
  return out;
}

bool verify_pem(const std::string& pubkey_pem, const std::string& data, const std::string& sig_b64) {
  std::vector<unsigned char> sig = b64decode(sig_b64);
  BIO* bio = BIO_new_mem_buf(pubkey_pem.data(), pubkey_pem.size());
  if (!bio) return false;
  EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);
  if (!pkey) return false;
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) { EVP_PKEY_free(pkey); return false; }
  bool ok = false;
  if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) == 1) {
    if (EVP_DigestVerify(ctx, sig.data(), sig.size(),
                         reinterpret_cast<const unsigned char*>(data.data()), data.size()) == 1) {
      ok = true;
    }
  }
  EVP_MD_CTX_free(ctx);
  EVP_PKEY_free(pkey);
  return ok;
}

} // namespace ed25519

