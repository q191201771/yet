/**
 * @file   yet_hmac_sha256_adapter.hpp
 * @author pengrl
 *
 */

#pragma once

//#define YET_HMAC_SHA256_OPENSSL
#define YET_HMAC_SHA256_CHEF

#if defined(YET_HMAC_SHA256_OPENSSL)
  #include <openssl/hmac.h>
  #include <openssl/sha.h>
#elif defined(YET_HMAC_SHA256_CHEF)
  #include "chef_base/chef_crypto_hmac_sha256.hpp"
#endif

namespace yet {

template <class Dummy>
struct HMACSHA256_static {
#if defined(YET_HMAC_SHA256_OPENSSL)
  static HMAC_CTX *hmac_;
#endif
};

#if defined(YET_HMAC_SHA256_OPENSSL)
template<class Dummy>
HMAC_CTX *HMACSHA256_static<Dummy>::hmac_ = nullptr;
#endif

class HMACSHA256 : public HMACSHA256_static<void> {
  public:
    void init(const uint8_t *key, size_t key_len) {
#if defined(YET_HMAC_SHA256_OPENSSL)
      if (!hmac_) {
  #if OPENSSL_VERSION_NUMBER < 0x10100000L
        static HMAC_CTX  shmac;
        hmac_ = &shmac;
        HMAC_CTX_init(hmac_);
  #else
        hmac_ = HMAC_CTX_new();
  #endif
      }

      HMAC_Init_ex(hmac_, key, key_len, EVP_sha256(), nullptr);
#elif defined(YET_HMAC_SHA256_CHEF)
      ctx_ = std::make_shared<chef::crypto_hmac_sha256>(key, key_len);
#endif
    }

    void update(const uint8_t *buf, size_t buf_len) {
#if defined(YET_HMAC_SHA256_OPENSSL)
      HMAC_Update(hmac_, buf, buf_len);
#elif defined(YET_HMAC_SHA256_CHEF)
      ctx_->update(buf, buf_len);
#endif
    }

    void final(uint8_t *dst) {
#if defined(YET_HMAC_SHA256_OPENSSL)
      unsigned int len;
      HMAC_Final(hmac_, dst, &len);
#elif defined(YET_HMAC_SHA256_CHEF)
    ctx_->final(dst);
#endif
    }

  public:
    HMACSHA256() {}
  private:
    HMACSHA256(const HMACSHA256 &) = delete;
    HMACSHA256 &operator=(const HMACSHA256 &) = delete;

  private:
#if defined(YET_HMAC_SHA256_OPENSSL)
#elif defined(YET_HMAC_SHA256_CHEF)
    std::shared_ptr<chef::crypto_hmac_sha256> ctx_;
#endif
};

}
