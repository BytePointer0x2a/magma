
/**
 * @file /magma/check/magma/providers/prime_check.c
 *
 * @brief DESCRIPTIONxxxGOESxxxHERE
 *
 * $Author$
 * $Date$
 * $Revision$
 *
 */

#include "magma_check.h"
#include "dime/ed25519/ed25519.h"


// Wrappers around ED25519 functions
typedef struct {
    ed25519_secret_key private_key;
    ed25519_public_key public_key;
} ED25519_KEY;


bool_t check_prime_writers_sthread(stringer_t *errmsg) {

	// The minimum valid key length is 68, so lets try that.
	if (status() && st_cmp_cs_eq(prime_header_org_key_write(68, MANAGEDBUF(5)), hex_decode_st(NULLER("07a0000044"), MANAGEDBUF(5)))) {
		st_sprint(errmsg, "Invalid PRIME header for an org key.");
		return false;
	}

	else if (status() && st_cmp_cs_eq(prime_header_user_key_write(68, MANAGEDBUF(5)), hex_decode_st(NULLER("07dd000044"), MANAGEDBUF(5)))) {
		st_sprint(errmsg, "Invalid PRIME header for a user key.");
		return false;
	}

	// We don't have minimums setup yet, so we're using 1024 for the length.
	else if (status() && st_cmp_cs_eq(prime_header_org_signet_write(1024, MANAGEDBUF(5)), hex_decode_st(NULLER("06f0000400"), MANAGEDBUF(5)))) {
		st_sprint(errmsg, "Invalid PRIME header for an org signet.");
		return false;
	}
	else if (status() && st_cmp_cs_eq(prime_header_encrypted_org_key_write(1024, MANAGEDBUF(5)), hex_decode_st(NULLER("079b000400"), MANAGEDBUF(5)))) {
		st_sprint(errmsg, "Invalid PRIME header for an encrypted org key.");
		return false;
	}
	else if (status() && st_cmp_cs_eq(prime_header_user_signet_write(1024, MANAGEDBUF(5)), hex_decode_st(NULLER("06fd000400"), MANAGEDBUF(5)))) {
		st_sprint(errmsg, "Invalid PRIME header for a user signet.");
		return false;
	}
	else if (status() && st_cmp_cs_eq(prime_header_user_signing_request_write(1024, MANAGEDBUF(5)), hex_decode_st(NULLER("04bf000400"), MANAGEDBUF(5)))) {
		st_sprint(errmsg, "Invalid PRIME header for a user signing request.");
		return false;
	}
	else if (status() && st_cmp_cs_eq(prime_header_encrypted_user_key_write(1024, MANAGEDBUF(5)), hex_decode_st(NULLER("07b8000400"), MANAGEDBUF(5)))) {
		st_sprint(errmsg, "Invalid PRIME header for an encrypted user key.");
		return false;
	}
	else if (status() && st_cmp_cs_eq(prime_header_encrypted_message_write(1024, MANAGEDBUF(6)), hex_decode_st(NULLER("073700000400"), MANAGEDBUF(6)))) {
		st_sprint(errmsg, "Invalid PRIME header for an encrypted message.");
		return false;
	}

	// Try creating objects that are intentionally too small.
	if (status() && prime_header_org_key_write(34, MANAGEDBUF(5))) {
		st_sprint(errmsg, "PRIME header returned for invalid org key size.");
		return false;
	}

	else if (status() && prime_header_user_key_write(34, MANAGEDBUF(5))) {
		st_sprint(errmsg, "PRIME header returned for invalid user key size.");
		return false;
	}

	return true;
}

bool_t check_prime_keys_sthread(stringer_t *errmsg) {

	prime_key_t *holder = NULL;

	// Allocate an org key.
	if (!(holder = prime_key_alloc(PRIME_ORG_KEY))) {
		st_sprint(errmsg, "Org key allocation failed.");
		return false;
	}
	else {
		prime_key_free(holder);
	}

	// Allocate a user key.
	if (!(holder = prime_key_alloc(PRIME_USER_KEY))) {
		st_sprint(errmsg, "User key allocation failed.");
		return false;
	}
	else {
		prime_key_free(holder);
	}

	// Attempt allocation of a non-key type using the key allocation function.
	if ((holder = prime_key_alloc(PRIME_ORG_SIGNET)) || (holder = prime_key_alloc(PRIME_USER_SIGNET)) || (holder = prime_key_alloc(PRIME_USER_SIGNING_REQUEST))) {
		st_sprint(errmsg, "User key allocation failed.");
		prime_key_free(holder);
		return false;
	}

	return true;
}

bool_t check_prime_secp256k1_fixed_sthread(stringer_t *errmsg) {

	int len;
	uchr_t padding[32];
	EC_KEY *key1 = NULL, *key2 = NULL;
	stringer_t *priv1 = hex_decode_st(NULLER("0000000000000000000000000000000000000000000000000000000000000045"), MANAGEDBUF(32)),
		*priv2 = hex_decode_st(NULLER("0000000c562e65f1e2603616804cec8dc4bf8bc5c183bffa66acc6148edbecc3"), MANAGEDBUF(32)),
		*pub1 = hex_decode_st(NULLER("025edd5cc23c51e87a497ca815d5dce0f8ab52554f849ed8995de64c5f34ce7143"), MANAGEDBUF(33)),
		*pub2 = hex_decode_st(NULLER("03a7f953b0fa3f407bbf37cec394800cceeadd0670c3f344524c347312f2c1da96"), MANAGEDBUF(33)),
		*kek = hex_decode_st(NULLER("97495184dc4de0a3dc614d15f699df6a8cb65a752434368fb7f1d2702c53ab19"), MANAGEDBUF(32)),
		*comparison;

	// Wipe the padding buffer. We compare this buffer with the leading bytes of the private key, with the length of the comparison
	// equal to the number of expected padding bytes.
	mm_wipe(padding, 32);

	// Build a comparison key object using the first serialized private key.
	if (!(key1 = secp256k1_private_set(priv1)) || EC_KEY_check_key_d(key1) != 1) {
		st_sprint(errmsg, "The first serialized secp256k1 private key doesn't appear to be valid.");
		if (key1) secp256k1_free(key1);
		return false;
	}

	// Make sure the private key is leads with the proper number of zero'ed out padding bytes.
	else if ((len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key1))) != 32 && mm_cmp_cs_eq(padding, st_data_get(priv1), 32 - len)) {
		st_sprint(errmsg, "The first serialized secp256k1 private key was not padded properly.");
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized private key against the original hard coded value.
	else if (!(comparison = secp256k1_private_get(key1, MANAGEDBUF(32))) || st_cmp_cs_eq(priv1, comparison) ||
		(len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key1))) != 1 || mm_cmp_cs_eq(padding, st_data_get(comparison), 32 - len)) {
		st_sprint(errmsg, "The first secp256k1 private key doesn't match the expected value.");
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized public key with the hard coded value.
	else if (!(comparison = secp256k1_public_get(key1, MANAGEDBUF(33))) || st_cmp_cs_eq(pub1, comparison)) {
		st_sprint(errmsg, "The first secp256k1 public key doesn't match the expected value.");
		secp256k1_free(key1);
		return false;
	}

	// Build a comparison key object using the second serialized private key.
	if (!(key2 = secp256k1_private_set(priv2)) || EC_KEY_check_key_d(key2) != 1) {
		st_sprint(errmsg, "The second serialized secp256k1 private key doesn't appear to be valid.");
		if (key2) secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Make sure the private key is leads with the proper number of zero'ed out padding bytes.
	else if ((len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key2))) != 32 && mm_cmp_cs_eq(padding, st_data_get(priv2), 32 - len)) {
		st_sprint(errmsg, "The second serialized secp256k1 private key was not padded properly.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized private key against the original hard coded value.
	else if (!(comparison = secp256k1_private_get(key2, MANAGEDBUF(32))) || st_cmp_cs_eq(priv2, comparison) ||
		(len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key2))) != 29 || mm_cmp_cs_eq(padding, st_data_get(comparison), 32 - len)) {
		st_sprint(errmsg, "The second secp256k1 private key doesn't match the expected value.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized public key with the hard coded value.
	else if (!(comparison = secp256k1_public_get(key2, MANAGEDBUF(33))) || st_cmp_cs_eq(pub2, comparison)) {
		st_sprint(errmsg, "The second secp256k1 public key doesn't match the expected value.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Compute the key encryption key, then swap public/private keys around and compute the key encryption key again.
	if (!(comparison = secp256k1_compute_kek(key1, key2, MANAGEDBUF(32))) || st_cmp_cs_eq(kek, comparison) ||
		!(comparison = secp256k1_compute_kek(key2, key1, MANAGEDBUF(32))) || st_cmp_cs_eq(kek, comparison) ) {
		st_sprint(errmsg, "The key encryption key computation on the secp256k1 keys didn't produced the expected result.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	secp256k1_free(key2);
	secp256k1_free(key1);

	return true;
}

bool_t check_prime_secp256k1_keys_sthread(stringer_t *errmsg) {

	int len;
	uchr_t padding[32];
	EC_KEY *original = NULL, *comparison = NULL, *ephemeral = NULL;
	stringer_t *priv = NULL, *pub = NULL, *kek1 = NULL, *kek2 = NULL;

	// Wipe the padding buffer. We compare this buffer with the leading bytes of the private key, with the length of the comparison
	// equal to the number of expected padding bytes.
	mm_wipe(padding, 32);

	for (uint64_t i = 0; status() && i < PRIME_CHECK_ITERATIONS; i++) {

		// Generate a new key pair.
		if (!(original = secp256k1_generate()) || EC_KEY_check_key_d(original) != 1) {
			st_sprint(errmsg, "Curve secp256k1 key generation failed.");
			if (original) secp256k1_free(original);
			return false;
		}
		// Extract the public and private components.
		else if (!(pub = secp256k1_public_get(original, MANAGEDBUF(33))) || !(priv = secp256k1_private_get(original, MANAGEDBUF(32)))) {
			st_sprint(errmsg, "Curve secp256k1 exponent serialization failed.");
			secp256k1_free(original);
			return false;
		}

		// Confirm the serialized output is the correct length.
		else if (st_length_get(priv) != 32 || st_length_get(pub) != 33) {
			st_sprint(errmsg, "The serialized secp256k1 keys are not the expected length.");
			secp256k1_free(original);
			return false;
		}

		// Make sure the private key is leads with the proper number of zero'ed out padding bytes.
		else if ((len = BN_num_bytes_d(EC_KEY_get0_private_key_d(original))) != 32 && mm_cmp_cs_eq(padding, st_data_get(priv), 32 - len)) {
			st_sprint(errmsg, "The serialized secp256k1 private key was not padded properly.");
			secp256k1_free(original);
			return false;
		}

		// Confirm the octet stream starts with 0x02 or 0x03, in accordance with the compressed point representation
		// described by ANSI standard X9.62 section 4.3.6.
		if (*((uchr_t *)st_data_get(pub)) != 2 && *((uchr_t *)st_data_get(pub)) != 3) {
			st_sprint(errmsg, "Curve secp256k1 public key point does not appear to be represented properly as a compressed point.");
			secp256k1_free(original);
			return false;
		}

		// Build a comparison key object using the serialized private key.
		else if (!(comparison = secp256k1_private_set(priv)) || EC_KEY_check_key_d(comparison) != 1) {
			st_sprint(errmsg, "The serialized secp256k1 private key doesn't appear to be valid.");
			if (comparison) secp256k1_free(comparison);
			secp256k1_free(original);
			return false;
		}

		// Compare the key object created against the original serialized key value, BN_cmp() will return 0 if the BIGNUM values are equivalent.
		else if (BN_cmp_d(EC_KEY_get0_private_key_d(original), EC_KEY_get0_private_key_d(comparison))) {
			st_sprint(errmsg, "The derived secp256k1 private key doesn't match our original key object.");
			secp256k1_free(comparison);
			secp256k1_free(original);
			return false;
		}
		// Compare the elipitical curve points, EC_POINT_cmp() will return 0 if the two representations are equivalent.
		else if (EC_POINT_cmp_d(EC_KEY_get0_group_d(original), EC_KEY_get0_public_key_d(original), EC_KEY_get0_public_key_d(comparison), NULL)) {
			st_sprint(errmsg, "The derived secp256k1 public key doesn't match our original key object.");
			secp256k1_free(comparison);
			secp256k1_free(original);
			return false;
		}

		// Build a comparison key object using the serialized public key.
		secp256k1_free(comparison);

		if (!(comparison = secp256k1_public_set(pub)) || EC_KEY_check_key_d(comparison) != 1) {
			st_sprint(errmsg, "The serialized secp256k1 public key doesn't appear to be valid.");
			if (comparison) secp256k1_free(comparison);
			secp256k1_free(original);
			return false;
		}
		else if (EC_POINT_cmp_d(EC_KEY_get0_group_d(original), EC_KEY_get0_public_key_d(original), EC_KEY_get0_public_key_d(comparison), NULL)) {
			st_sprint(errmsg, "The derived secp256k1 public key doesn't match our original key object.");
			secp256k1_free(comparison);
			secp256k1_free(original);
			return false;
		}

		// Generate an ephemeral key for checking the key encryption key functions.
		else if (!(ephemeral = secp256k1_generate()) || EC_KEY_check_key_d(ephemeral) != 1) {
			st_sprint(errmsg, "Curve secp256k1 key generation failed.");
			if (ephemeral) secp256k1_free(ephemeral);
			secp256k1_free(comparison);
			secp256k1_free(original);
			return false;
		}

		// Compute a KEK using the ephemeral private key, and the original public key.
		else if (!(kek1 = secp256k1_compute_kek(ephemeral, comparison, MANAGEDBUF(32))) || st_length_get(kek1) != 32) {
			st_sprint(errmsg, "Key encryption key derivation failed using a secp256k1 ephemeral key, and a public key object.");
			secp256k1_free(comparison);
			secp256k1_free(ephemeral);
			secp256k1_free(original);
			return false;
		}

		// Compute a KEK using the ephemeral public key, and the original private key.
		else if (!(kek2 = secp256k1_compute_kek(original, ephemeral, MANAGEDBUF(32))) || st_length_get(kek2) != 32) {
			st_sprint(errmsg, "Key encryption key derivation failed using a secp256k1 private key, and the public portion of the ephemeral public key object.");
			secp256k1_free(comparison);
			secp256k1_free(ephemeral);
			secp256k1_free(original);
			return false;
		}

		// If everything worked properly then both KEK values will be equal.
		else if (st_cmp_cs_eq(kek1, kek2)) {
			st_sprint(errmsg, "The two key encryption key values failed to match.");
			secp256k1_free(comparison);
			secp256k1_free(ephemeral);
			secp256k1_free(original);
			return false;
		}

		secp256k1_free(comparison);
		secp256k1_free(ephemeral);
		secp256k1_free(original);
	}

	return true;
}

bool_t check_prime_secp256k1_parameters_sthread(stringer_t *errmsg) {

	EC_POINT *uncompressed = NULL;
	EC_KEY *key = NULL, *ephemeral = NULL;

	// Test a NULL input.
	if ((key = secp256k1_private_set(NULL))) {
		st_sprint(errmsg, "The secp256k1 private key setup function accepted a NULL input value.");
		secp256k1_free(key);
		return false;
	}
	// Try again with a value that isn't long enough.
	else if ((key = secp256k1_private_set(hex_decode_st(NULLER("00"), MANAGEDBUF(1))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function accepted a private key value that wasn't long enough.");
		secp256k1_free(key);
		return false;
	}
	// Try again with a value that is too long.
	else if ((key = secp256k1_private_set(hex_decode_st(NULLER("000000000000000000000000000000000000000000000000000000000000000001"), MANAGEDBUF(33))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function accepted a private key value that was too long.");
		secp256k1_free(key);
		return false;
	}
	// The private key value must be 1 or higher and properly padded.
	else if ((key = secp256k1_private_set(hex_decode_st(NULLER("0000000000000000000000000000000000000000000000000000000000000000"), MANAGEDBUF(32))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function accepted a private key value of zero.");
		secp256k1_free(key);
		return false;
	}
	// The private key value must be equal to or less than 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141.
	else if ((key = secp256k1_private_set(hex_decode_st(NULLER("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), MANAGEDBUF(32))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function accepted a private key value larger than allowed.");
		secp256k1_free(key);
		return false;
	}
	// Try with a value that is only two bits higher than allowed.
	else if ((key = secp256k1_private_set(hex_decode_st(NULLER("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364142"), MANAGEDBUF(32))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function accepted a private key value two bits higher than allowed.");
		secp256k1_free(key);
		return false;
	}
	// Try with a value that is only two bits higher than allowed.
	else if ((key = secp256k1_private_set(hex_decode_st(NULLER("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"), MANAGEDBUF(32))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function accepted a private key value one bit higher than allowed.");
		secp256k1_free(key);
		return false;
	}
	// Try with a value that is precisely equal to the maximum legal value.
	else if (!(key = secp256k1_private_set(hex_decode_st(NULLER("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140"), MANAGEDBUF(32))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function did not accept the maximum value for a private key.");
		return false;
	}

	secp256k1_free(key);

	// Try with a value that is precisely equal to the minimum legal value.
	if (!(key = secp256k1_private_set(hex_decode_st(NULLER("0000000000000000000000000000000000000000000000000000000000000001"), MANAGEDBUF(32))))) {
		st_sprint(errmsg, "The secp256k1 private key setup function did not accept the maximum value for a private key.");
		return false;
	}

	secp256k1_free(key);

	// Test a NULL input.
	if ((key = secp256k1_public_set(NULL))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a NULL input value.");
		secp256k1_free(key);
		return false;
	}
	// Try with a value of the wrong length.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("0200"), MANAGEDBUF(2))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value that wasn't long enough.");
		secp256k1_free(key);
		return false;
	}
	// Try a value with an invalid prefix.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("000000000000000000000000000000000000000000000000000000000000000000"), MANAGEDBUF(33))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value with an invalid prefix.");
		secp256k1_free(key);
		return false;
	}
	// Try a valid uncompressed point using the appropriate uncomressed prefix. This should fail, as we require compressed points.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8"), MANAGEDBUF(65))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value with an invalid prefix.");
		secp256k1_free(key);
		return false;
	}
	// Try a value with a valid prefix, but invalid value for X.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("020000000000000000000000000000000000000000000000000000000000000000"), MANAGEDBUF(33))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value that was invalid.");
		secp256k1_free(key);
		return false;
	}
	// Try a value with a valid prefix, but invalid value for X.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("030000000000000000000000000000000000000000000000000000000000000000"), MANAGEDBUF(33))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value that was invalid.");
		secp256k1_free(key);
		return false;
	}
	// Try a value with a valid prefix, but invalid value for X.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("02ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), MANAGEDBUF(33))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value that was invalid.");
		secp256k1_free(key);
		return false;
	}
	// Try a value with a valid prefix, but invalid value for X.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("03ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"), MANAGEDBUF(33))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value that was invalid.");
		secp256k1_free(key);
		return false;
	}
	// Take a valid value, and padd the X value with 1 extra padding octet thus making the representation of the value too long.
	else if ((key = secp256k1_public_set(hex_decode_st(NULLER("020079be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"), MANAGEDBUF(34))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value that was too long.");
		secp256k1_free(key);
		return false;
	}

	// Take the public key value corresponding to a private key value of 0x01, and create a valid key object for further testing.
	else if (!(key = secp256k1_public_set(hex_decode_st(NULLER("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"), MANAGEDBUF(33))))) {
		st_sprint(errmsg, "The secp256k1 public key setup function accepted a public key value that was too long.");
		return false;
	}

	// Create a point object using the uncompressed form so we can compare the uncompressed point with the public key point created using the compressed form.
	else if (!(uncompressed = EC_POINT_hex2point_d(EC_KEY_get0_group_d(key), "0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8", NULL, NULL))) {
		st_sprint(errmsg, "The secp256k1 public key expressed in uncompressed form could not be converted into a point object for comparison purposes.");
		secp256k1_free(key);
		return false;

	}

	// Compare the uncompressed point with the public key point we created using the compressed format above.
	else if (EC_POINT_cmp_d(EC_KEY_get0_group_d(key), EC_KEY_get0_public_key_d(key), uncompressed, NULL)) {
		st_sprint(errmsg, "The secp256k1 public key expressed in uncompressed form could not be converted into a point object for comparison purposes.");
		EC_POINT_free_d(uncompressed);
		secp256k1_free(key);
		return false;
	}

	EC_POINT_free_d(uncompressed);

	// Try and fetch a private key from an object that only has a public key.
	if (secp256k1_private_get(key, MANAGEDBUF(32))) {
		st_sprint(errmsg, "A secp256k1 private key was returned when only a public key should have been available.");
		secp256k1_free(key);
		return false;
	}

	// Try and get a public key but provide an output buffer that is too small.
	else if (secp256k1_public_get(key, MANAGEDBUF(32))) {
		st_sprint(errmsg, "A secp256k1 public key was returned in an output buffer that wasn't large enough to hold the value.");
		secp256k1_free(key);
		return false;
	}

	// Try and get a public key using a valid output buffer.
	else if (!secp256k1_public_get(key, MANAGEDBUF(33))) {
		st_sprint(errmsg, "A secp256k1 public key wasn't returned even though a valid output buffer was provided.");
		secp256k1_free(key);
		return false;
	}

	// Generate an ephemeral key for further testing.
	else if (!(ephemeral = secp256k1_generate())) {
		st_sprint(errmsg, "The secp256k1 ephemeral key couldn't be generated.");
		secp256k1_free(key);
		return false;
	}

	// Try computing a KEK value but flip the EC values so the object with only a public key is provided where the epemeral value should have been.
	else if (secp256k1_compute_kek(key, ephemeral, MANAGEDBUF(32))) {
		st_sprint(errmsg, "The secp256k1 key encryption key computation worked when no private key was available.");
		secp256k1_free(ephemeral);
		secp256k1_free(key);
		return false;
	}

	// Try computing a KEK with the values in the correct order, but supply an output buffer that is 1 byte too small.
	else if (secp256k1_compute_kek(ephemeral, key, MANAGEDBUF(31))) {
		st_sprint(errmsg, "The secp256k1 key encryption key computation worked even though the output buffer was too small.");
		secp256k1_free(ephemeral);
		secp256k1_free(key);
		return false;
	}

	// Finally try the computation with valid values.
	else if (!secp256k1_compute_kek(ephemeral, key, MANAGEDBUF(32))) {
		st_sprint(errmsg, "The secp256k1 key encryption key computation didn't work even though all of the input should have been correct.");
		secp256k1_free(ephemeral);
		secp256k1_free(key);
		return false;
	}

	secp256k1_free(ephemeral);
	secp256k1_free(key);
	return true;

}

bool_t check_prime_ed25519_fixed_sthread(stringer_t *errmsg) {

	ed25519_key_t *key = NULL;
	stringer_t *priv1 = hex_decode_st(NULLER("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"), MANAGEDBUF(32)),
		*priv2 = hex_decode_st(NULLER("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb"), MANAGEDBUF(32)),
		*priv3 = hex_decode_st(NULLER("c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7"), MANAGEDBUF(32)),
		*pub1 = hex_decode_st(NULLER("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"), MANAGEDBUF(32)),
		*pub2 = hex_decode_st(NULLER("3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c"), MANAGEDBUF(32)),
		*pub3 = hex_decode_st(NULLER("fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025"), MANAGEDBUF(32)),
		*signature1 = hex_decode_st(NULLER("e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"), MANAGEDBUF(64)),
		*signature2 = hex_decode_st(NULLER("92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"), MANAGEDBUF(64)),
		*signature3 = hex_decode_st(NULLER("6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a"), MANAGEDBUF(64)),
		*message1 = NULLER(""),
		*message2 = hex_decode_st(NULLER("72"), MANAGEDBUF(1)),
		*message3 = hex_decode_st(NULLER("af82"), MANAGEDBUF(2)),
		*comparison;

	// Build a comparison key object using the first serialized private key.
	if (!(key1 = secp256k1_private_set(priv1)) || EC_KEY_check_key_d(key1) != 1) {
		st_sprint(errmsg, "The first serialized secp256k1 private key doesn't appear to be valid.");
		if (key1) secp256k1_free(key1);
		return false;
	}

	// Make sure the private key is leads with the proper number of zero'ed out padding bytes.
	else if ((len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key1))) != 32 && mm_cmp_cs_eq(padding, st_data_get(priv1), 32 - len)) {
		st_sprint(errmsg, "The first serialized secp256k1 private key was not padded properly.");
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized private key against the original hard coded value.
	else if (!(comparison = secp256k1_private_get(key1, MANAGEDBUF(32))) || st_cmp_cs_eq(priv1, comparison) ||
		(len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key1))) != 1 || mm_cmp_cs_eq(padding, st_data_get(comparison), 32 - len)) {
		st_sprint(errmsg, "The first secp256k1 private key doesn't match the expected value.");
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized public key with the hard coded value.
	else if (!(comparison = secp256k1_public_get(key1, MANAGEDBUF(33))) || st_cmp_cs_eq(pub1, comparison)) {
		st_sprint(errmsg, "The first secp256k1 public key doesn't match the expected value.");
		secp256k1_free(key1);
		return false;
	}

	// Build a comparison key object using the second serialized private key.
	if (!(key2 = secp256k1_private_set(priv2)) || EC_KEY_check_key_d(key2) != 1) {
		st_sprint(errmsg, "The second serialized secp256k1 private key doesn't appear to be valid.");
		if (key2) secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Make sure the private key is leads with the proper number of zero'ed out padding bytes.
	else if ((len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key2))) != 32 && mm_cmp_cs_eq(padding, st_data_get(priv2), 32 - len)) {
		st_sprint(errmsg, "The second serialized secp256k1 private key was not padded properly.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized private key against the original hard coded value.
	else if (!(comparison = secp256k1_private_get(key2, MANAGEDBUF(32))) || st_cmp_cs_eq(priv2, comparison) ||
		(len = BN_num_bytes_d(EC_KEY_get0_private_key_d(key2))) != 29 || mm_cmp_cs_eq(padding, st_data_get(comparison), 32 - len)) {
		st_sprint(errmsg, "The second secp256k1 private key doesn't match the expected value.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Compare the serialized public key with the hard coded value.
	else if (!(comparison = secp256k1_public_get(key2, MANAGEDBUF(33))) || st_cmp_cs_eq(pub2, comparison)) {
		st_sprint(errmsg, "The second secp256k1 public key doesn't match the expected value.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	// Compute the key encryption key, then swap public/private keys around and compute the key encryption key again.
	if (!(comparison = secp256k1_compute_kek(key1, key2, MANAGEDBUF(32))) || st_cmp_cs_eq(kek, comparison) ||
		!(comparison = secp256k1_compute_kek(key2, key1, MANAGEDBUF(32))) || st_cmp_cs_eq(kek, comparison) ) {
		st_sprint(errmsg, "The key encryption key computation on the secp256k1 keys didn't produced the expected result.");
		secp256k1_free(key2);
		secp256k1_free(key1);
		return false;
	}

	secp256k1_free(key2);
	secp256k1_free(key1);

	return true;
}

bool_t check_prime_ed25519_keys_sthread(stringer_t *errmsg) {

	size_t len = 0;
	ed25519_key_t *key = NULL;
	uint8_t signature[ED25519_SIGNATURE_LEN];
	stringer_t *fuzzer = MANAGEDBUF(PRIME_CHECK_SIZE_MAX);
	unsigned char ed25519_donna_public_key[ED25519_KEY_PUB_LEN];

	for (uint64_t i = 0; status() && i < PRIME_CHECK_ITERATIONS; i++) {

		// Generate a random ed25519 key pair.
		if (!(key = ed25519_generate())) {
			st_sprint(errmsg, "Failed to generate an ed25519 key pair.");
			return false;
		}

		// Calculate the public key value using the alternate implementation.
		ed25519_publickey(key->private, ed25519_donna_public_key);

		// Compare the values.
		if (st_cmp_cs_eq(PLACER(ed25519_donna_public_key, ED25519_KEY_PUB_LEN), PLACER(key->public, ED25519_KEY_PUB_LEN))) {
			st_sprint(errmsg, "The alternate implementation failed to derive an identical ed25519 public key.");
			ed25519_free(key);
			return false;
		}

		// How much random data will we fuzz with.
		len = (rand() % (PRIME_CHECK_SIZE_MAX - PRIME_CHECK_SIZE_MIN)) + PRIME_CHECK_SIZE_MIN;

		// Create a buffer filled with random data to sign.
		rand_write(PLACER(st_data_get(fuzzer), len));

		// Generate an ed25519 signature using OpenSSL.
		if (ED25519_sign_d(&signature[0], st_data_get(fuzzer), len, key->private) != 1) {
			st_sprint(errmsg, "The ed25519 signature operation failed.");
			ed25519_free(key);
			return false;
		}
		// Verify the ed25519 signature using the alternate implementation.
		else if (ed25519_sign_open(st_data_get(fuzzer), len, key->public, signature)) {
			st_sprint(errmsg, "The alternate implementation failed to verify the ed25519 signature.");
			ed25519_free(key);
			return false;
		}

		// How much random data will we fuzz with for the second check.
		len = (rand() % (PRIME_CHECK_SIZE_MAX - PRIME_CHECK_SIZE_MIN)) + PRIME_CHECK_SIZE_MIN;

		// Create a buffer filled with random data to sign.
		rand_write(PLACER(st_data_get(fuzzer), len));

		// Generate an ed25519 signature using the alternate implementation.
		ed25519_sign(st_data_get(fuzzer), len, key->private, key->public, signature);

		// Verify the ed25519 signature using OpenSSL.
		if (ED25519_verify_d(st_data_get(fuzzer), len, signature, key->public) != 1) {
			st_sprint(errmsg, "The ed25519 signature generated using the alternate implementation failed to verify.");
			ed25519_free(key);
			return false;
		}

		ed25519_free(key);

	}
	return true;
}
