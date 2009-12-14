#ifndef _XSSL_H_
#define _XSSL_H_
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#define XSSL(ptr) ((SSL *)ptr)
#define XSSL_CTX(ptr) ((SSL_CTX *)ptr)
#endif
