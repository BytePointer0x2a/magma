
/**
 * @file /magma/src/providers/prime/keys/users.c
 *
 * @brief DESCRIPTIONxxxGOESxxxHERE
 *
 * $Author$
 * $Date$
 * $Revision$
 *
 */

#include "magma.h"

void user_key_free(prime_user_key_t *user) {

	if (user) {
		if (user->signing) st_free(user->signing);
		if (user->encryption) secp256k1_free(user->encryption);
		mm_free(user);
	}

	return;
}

prime_user_key_t * user_key_alloc(void) {

	prime_user_key_t *user = NULL;

	if (!(user = mm_alloc(sizeof(prime_user_key_t)))) {
		log_pedantic("Allocation of the PRIME user key failed.");
		return NULL;
	}

	mm_wipe(user, sizeof(prime_user_key_t));

	return user;
}

prime_user_key_t * user_key_generate(void) {

	prime_user_key_t *user = NULL;

	if (!(user = mm_alloc(sizeof(prime_user_key_t)))) {
		return NULL;
	}

	if (!(user->signing = ed25519_key_generate()) || !(user->encryption = secp256k1_generate())) {
		log_pedantic("PRIME user key generation failed.");
		user_key_free(user);
		return NULL;
	}

	return user;
}

size_t user_key_length(prime_user_key_t *user) {

	// We know the keys will be 76 bytes, at least for now. A 5 byte header, 2x 1 byte field identifiers, 2x 1 byte
	// field lengths, and 2x private keys each 32 bytes in length.
	size_t result = 76;
	if (!user || !user->signing || !user->encryption) result = 0;
	return result;
}

size_t user_key_get(prime_user_key_t *user, stringer_t *output) {
	#error Unfinished.
	return 0;
}