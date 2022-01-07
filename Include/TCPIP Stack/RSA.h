/*********************************************************************
 *
 *				RSA Public Key Encryption Library Header
 *
 *********************************************************************
 * FileName:        RSA.h
 * Dependencies:    BigInt.h
 * Processor:       PIC18F97J60
 * Compiler:        HI-TECH PICC-18 STD 9.50PL3 or higher
 ********************************************************************/

#ifndef __RSA_H
#define __RSA_H

#define RSA_KEY_WORDS	(SSL_RSA_KEY_SIZE/BIGINT_DATA_SIZE)	// Words
#define RSA_PRIME_WORDS	(SSL_RSA_KEY_SIZE/BIGINT_DATA_SIZE/2)	// Words

// TODO: properly select these options
//#define STACK_USE_RSA_DECRYPT
#define STACK_USE_RSA_ENCRYPT
#define STACK_USE_RSA

typedef enum _SM_RSA {
	SM_RSA_IDLE = 0u,
	SM_RSA_ENCRYPT_START,
	SM_RSA_ENCRYPT,
	SM_RSA_DECRYPT_START,
	SM_RSA_DECRYPT_FIND_M1,
	SM_RSA_DECRYPT_FIND_M2,
	SM_RSA_DECRYPT_FINISH,
	SM_RSA_DONE
} SM_RSA;

typedef enum _RSA_STATUS {
	RSA_WORKING = 0u,
	RSA_FINISHED_M1,
	RSA_FINISHED_M2,
	RSA_DONE
} RSA_STATUS;

typedef enum _RSA_DATA_FORMAT {
	RSA_BIG_ENDIAN = 0u,
	RSA_LITTLE_ENDIAN
} RSA_DATA_FORMAT;

typedef enum _RSA_OP {
	RSA_OP_ENCRYPT = 0u,
	RSA_OP_DECRYPT
} RSA_OP;

#define RSABeginDecrypt()	RSABeginUsage(RSA_OP_DECRYPT, 0)
#define RSABeginEncrypt(a)	RSABeginUsage(RSA_OP_ENCRYPT, a)
#define RSAEndDecrypt()		RSAEndUsage()
#define RSAEndEncrypt()		RSAEndUsage()

void RSAInit(void);

BOOL RSABeginUsage(RSA_OP op, unsigned char keyBytes);
void RSAEndUsage(void);

void RSASetData(unsigned char *data, unsigned char len,
				RSA_DATA_FORMAT format);
void RSASetE(unsigned char *data, unsigned char len,
			 RSA_DATA_FORMAT format);
void RSASetN(unsigned char *data, RSA_DATA_FORMAT format);
void RSASetResult(unsigned char *data, RSA_DATA_FORMAT format);

RSA_STATUS RSAStep(void);

#endif
