#pragma once

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <string>
#include <vector>
#include <string.h>


/**
 *
쉘에서 사용하는 방법.

# 암호화
echo -n "123qwe" | openssl enc -aes-256-cbc -base64 -pbkdf2 -iter 10000 -k mypassword

# 복호화
echo "U2FsdGVkX18dznWYciimbufUB+0knsGQdWGyCSTkXyA=" | openssl enc -aes-256-cbc -base64 -d -pbkdf2 -k mypassword
echo "U2FsdGVkX1+mPC3FLqRzgafjHIShOMBD7cVyd/Vgcd0=" | openssl enc -aes-256-cbc -base64 -d -pbkdf2 -iter 10000 -k mypassword
 *
 */

//int main()
//{
//    try
//    {
//        CnapsTextCrypto crypto;
//        std::string password = "mypassword";
//        std::string plaintext = "secret text";
//
//        // 암호화
//        std::string encrypted = crypto.encrypt(plaintext, password);
//        std::cout << "Encrypted: " << encrypted << std::endl;
//
//        // 복호화
//        std::string decrypted = crypto.decrypt(encrypted, password);
//        std::cout << "Decrypted: " << decrypted << std::endl;
//
//    }
//    catch (const std::exception& e)
//    {
//        std::cerr << "Error: " << e.what() << std::endl;
//        return 1;
//    }
//    return 0;
//}

class CnapsTextCrypto
{
public:
  std::string encrypt(const std::string& plaintext, const std::string& password)
  {
    unsigned char salt[SALT_SIZE];
    if (RAND_bytes(salt, SALT_SIZE) != 1)
    {
      throw std::runtime_error("Failed to generate salt");
    }

    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
    derive_key_and_iv(password, salt, key, iv);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    std::vector<unsigned char> ciphertext(plaintext.length() + EVP_MAX_BLOCK_LENGTH);
    int len1, len2;
    EVP_EncryptUpdate(ctx, ciphertext.data() + MAGIC.length() + SALT_SIZE,
                      &len1, (unsigned char*)plaintext.c_str(), plaintext.length());
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + MAGIC.length() + SALT_SIZE + len1, &len2);
    EVP_CIPHER_CTX_free(ctx);

    std::vector<unsigned char> result(MAGIC.length() + SALT_SIZE + len1 + len2);
    memcpy(result.data(), MAGIC.c_str(), MAGIC.length());
    memcpy(result.data() + MAGIC.length(), salt, SALT_SIZE);
    memcpy(result.data() + MAGIC.length() + SALT_SIZE,
           ciphertext.data() + MAGIC.length() + SALT_SIZE, len1 + len2);

    return base64_encode(result);
  }

  std::string decrypt(const std::string& base64_cipher, const std::string& password)
  {
    std::vector<unsigned char> ciphertext = base64_decode(base64_cipher);

    unsigned char salt[SALT_SIZE];
    memcpy(salt, ciphertext.data() + MAGIC.length(), SALT_SIZE);

    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
    derive_key_and_iv(password, salt, key, iv);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len1, len2;
    EVP_DecryptUpdate(ctx, plaintext.data(), &len1,
                      ciphertext.data() + MAGIC.length() + SALT_SIZE,
                      ciphertext.size() - MAGIC.length() - SALT_SIZE);
    EVP_DecryptFinal_ex(ctx, plaintext.data() + len1, &len2);
    EVP_CIPHER_CTX_free(ctx);

    return std::string((char*)plaintext.data(), len1 + len2);
  }

private:
  const std::string MAGIC = "Salted__";
  const int SALT_SIZE = 8;

  std::string base64_encode(const std::vector<unsigned char>& binary)
  {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, binary.data(), binary.size());
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
  }

  std::vector<unsigned char> base64_decode(const std::string& base64)
  {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new_mem_buf(base64.c_str(), base64.length());
    bmem = BIO_push(b64, bmem);
    std::vector<unsigned char> result((base64.length() * 3) / 4);
    int decoded_size = BIO_read(bmem, result.data(), result.size());
    BIO_free_all(bmem);
    if (decoded_size > 0)
    {
      result.resize(decoded_size);
      return result;
    }
    throw std::runtime_error("Failed to decode base64 string");
  }

  void derive_key_and_iv(const std::string& password, const unsigned char* salt,
                         unsigned char* key, unsigned char* iv)
  {
    const int key_length = 32;  // AES-256용 키 길이
    const int iv_length = 16;   // AES-256-CBC용 IV 길이

    unsigned char key_data[key_length + iv_length];

    if (!PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, SALT_SIZE,
                           10000, // 암복호화시 iter 값.
                           EVP_sha256(),
                           key_length + iv_length, key_data))
    {
      throw std::runtime_error("PBKDF2 key derivation failed");
    }

    memcpy(key, key_data, key_length);
    memcpy(iv, key_data + key_length, iv_length);
  }
};



