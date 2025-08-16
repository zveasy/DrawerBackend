#include <gtest/gtest.h>
#include "util/ed25519_verify.hpp"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

static std::string pub_pem(EVP_PKEY* pkey) {
  BIO* bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PUBKEY(bio, pkey);
  BUF_MEM* mem; BIO_get_mem_ptr(bio, &mem);
  std::string s(mem->data, mem->length);
  BIO_free(bio);
  return s;
}

static std::string sign_msg(EVP_PKEY* pkey, const std::string& msg) {
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey);
  size_t len=0; EVP_DigestSign(ctx, nullptr, &len, (const unsigned char*)msg.data(), msg.size());
  std::string sig(len, '\0');
  EVP_DigestSign(ctx, (unsigned char*)sig.data(), &len, (const unsigned char*)msg.data(), msg.size());
  EVP_MD_CTX_free(ctx);
  BIO* b64 = BIO_new(BIO_f_base64()); BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO* mem = BIO_new(BIO_s_mem()); b64 = BIO_push(b64, mem);
  BIO_write(b64, sig.data(), len); BIO_flush(b64);
  BUF_MEM* buf; BIO_get_mem_ptr(b64, &buf);
  std::string out(buf->data, buf->length);
  BIO_free_all(b64);
  return out;
}

TEST(OtaSigVerify, Verify) {
  EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
  EVP_PKEY* pkey = nullptr;
  EVP_PKEY_keygen_init(pctx);
  EVP_PKEY_keygen(pctx, &pkey);
  std::string pub = pub_pem(pkey);
  std::string msg = "test-message";
  std::string sig = sign_msg(pkey, msg);
  EXPECT_TRUE(ed25519::verify_pem(pub, msg, sig));
  EXPECT_FALSE(ed25519::verify_pem(pub, msg+"x", sig));
  EVP_PKEY_free(pkey);
  EVP_PKEY_CTX_free(pctx);
}

