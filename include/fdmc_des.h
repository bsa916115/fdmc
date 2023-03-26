#ifndef _FDMC_DES_INCLUDE
#define _FDMC_DES_INCLUDE

// -------------- DES functions --------------
// unsigned char traffic_key[8];
// unsigned char traffic_key_dbl[16];
// Single des with predefined key - traffic_key, must be declared as extern
 int fdmc_des_block_encrypt(char *buf, int len);
 int fdmc_des_block_decrypt(char *buf, int len);
// Triple des with predefined key - traffic_key_dbl, must be declared as extern
 int fdmc_des_block_trpl_encrypt(char *buf, int len);
 int fdmc_des_block_trpl_decrypt(char *buf, int len);
// Single and triple des with custom key
 int fdmc_des_buffer_encrypt(char *buf, int len, unsigned char *key);
 int fdmc_des_buffer_decrypt(char *buf, int len, unsigned char *key);
 int fdmc_des_buffer_trpl_encrypt(char *buf, int len, unsigned char *dblkey);
 int fdmc_des_buffer_trpl_decrypt(char *buf, int len, unsigned char *dblkey);
// Triple des with XOR and custom key
 int fdmc_des_buffer_trplcbc_encrypt(unsigned char *buff, unsigned len, unsigned char *dblkey);
 int fdmc_des_buffer_trplcbc_decrypt(unsigned char *buff, unsigned len, unsigned char *dblkey);
// ----------- Usefull utilities ------------
// ASCII/binary key convertations. key_type must be 1 for single or 2 for double key
 char* fdmc_des_key_to_hex(unsigned char *key, char *hex, int key_type);
 unsigned char* fdmc_des_hex_to_key(char *hex, unsigned char *key, int key_type);
// Key parity check
 int fdmc_des_parity_check(unsigned char *key, int key_type);
 unsigned char* fdmc_des_parity_set(unsigned char *key, int len);
 char fdmc_luhn_calc(char* card_num);
#endif
