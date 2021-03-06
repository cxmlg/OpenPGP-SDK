/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. 
 * 
 * You may obtain a copy of the License at 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * 
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// FIXME: now that these tests print errors during parse, they are
// blatantly broken, but still pass.

#include "CUnit/Basic.h"

#include <openpgpsdk/defs.h>
#include <openpgpsdk/types.h>
#include "openpgpsdk/keyring.h"
#include <openpgpsdk/armour.h>
#include <openpgpsdk/create.h>
#include "openpgpsdk/packet.h"
#include "openpgpsdk/packet-parse.h"
#include "openpgpsdk/packet-show.h"
#include "openpgpsdk/util.h"
#include "openpgpsdk/std_print.h"
#include "openpgpsdk/readerwriter.h"
#include "openpgpsdk/validate.h"

// \todo change this once we know it works
#include "../src/lib/parse_local.h"

#include "tests.h"

static int debug=0;

static const char filename_rsa_large_noarmour_nopassphrase[]
  ="ops_rsa_signed_large_noarmour_nopassphrase.txt";
static const char filename_rsa_large_armour_nopassphrase[]
  ="ops_rsa_signed_large_armour_nopassphrase.txt";
static const char filename_rsa_noarmour_nopassphrase[]
  ="ops_rsa_signed_noarmour_nopassphrase.txt";
static const char filename_rsa_noarmour_passphrase[]
  ="ops_rsa_signed_noarmour_passphrase.txt";
static const char filename_rsa_armour_nopassphrase[]
  ="ops_rsa_signed_armour_nopassphrase.txt";
static const char filename_rsa_armour_passphrase[]
  ="ops_rsa_signed_armour_passphrase.txt";
static const char filename_rsa_clearsign_file_nopassphrase[]
  ="ops_rsa_signed_clearsign_file_nopassphrase.txt";
static const char filename_rsa_clearsign_file_passphrase[]
  ="ops_rsa_signed_clearsign_file_passphrase.txt";
static const char filename_rsa_clearsign_buf_nopassphrase[]
  ="ops_rsa_signed_clearsign_buf_nopassphrase.txt";
static const char filename_rsa_clearsign_buf_passphrase[]
  ="ops_rsa_signed_clearsign_buf_passphrase.txt";

/* Signature suite initialization.
 * Create temporary directory.
 * Create temporary test files.
 */

int init_suite_rsa_signature(void)
    {
    // Create test files

    create_small_testfile(filename_rsa_noarmour_nopassphrase);
    create_small_testfile(filename_rsa_noarmour_passphrase);
    create_small_testfile(filename_rsa_armour_nopassphrase);
    create_small_testfile(filename_rsa_armour_passphrase);
    create_small_testfile(filename_rsa_clearsign_file_nopassphrase);
    create_small_testfile(filename_rsa_clearsign_file_passphrase);
    create_small_testfile(filename_rsa_clearsign_buf_nopassphrase);
    create_small_testfile(filename_rsa_clearsign_buf_passphrase);

    create_large_testfile(filename_rsa_large_noarmour_nopassphrase);
    create_large_testfile(filename_rsa_large_armour_nopassphrase);

    // Return success
    return 0;
    }

int clean_suite_rsa_signature(void)
    {
    ops_finish();

    reset_vars();

    return 0;
    }

static void test_rsa_signature_clearsign_file(const char *filename,
					      const ops_secret_key_t *skey)
    {
    char myfile[MAXBUF];
    char signed_file[MAXBUF];
    ops_boolean_t overwrite;

    set_up_file_names(myfile, signed_file, filename, "asc");

    // sign file
    overwrite=ops_true;
    ops_sign_file_as_cleartext(myfile, NULL, skey, overwrite);

    check_sig(signed_file, ops_true);
    }

static void test_rsa_signature_clearsign_buf(const char *filename,
					     const ops_secret_key_t *skey)
    {
    char myfile[MAXBUF];
    char signed_file[MAXBUF];
    ops_memory_t *input=NULL;
    ops_memory_t *output=NULL;
    ops_boolean_t overwrite;
    int errnum=0;

    // (we are testing the function which signs a buf, but still want
    // to read/write the buffers from/to files for external viewing
    set_up_file_names(myfile, signed_file, filename, "asc");

    // read file contents
    input=ops_write_mem_from_file(myfile, &errnum);
    CU_ASSERT(errnum==0);

    // sign file
    ops_sign_buf_as_cleartext(ops_memory_get_data(input),
			      ops_memory_get_length(input), &output,skey);

    // write to file
    overwrite=ops_true;
    ops_write_file_from_buf(signed_file, ops_memory_get_data(output),
			    ops_memory_get_length(output), overwrite);

    check_sig(signed_file, ops_true);
    }

static void test_rsa_signature_sign(const int use_armour, const char *filename,
				    const ops_secret_key_t *skey)
    {
    char myfile[MAXBUF];
    char signed_file[MAXBUF];
    char *suffix= use_armour ? "asc" : "gpg";
    ops_boolean_t overwrite=ops_true;

    set_up_file_names(myfile, signed_file, filename, suffix);

    ops_sign_file(myfile, signed_file, skey, use_armour, overwrite);

    check_sig(signed_file, use_armour);
    }

static void test_rsa_signature_sign_stream(const int use_armour,
					   const char *filename,
                                           const ops_secret_key_t *skey)
    {
    char myfile[MAXBUF];
    char signed_file[MAXBUF];
    char buffer[MAXBUF];
    char *suffix= use_armour ? "streamed.asc" : "streamed.gpg";
    ops_boolean_t overwrite=ops_true;

    set_up_file_names(myfile, signed_file, filename, suffix);

    ops_create_info_t *info;
    ops_memory_t *tmp;
    ops_setup_memory_write(&info, &tmp, MAXBUF);
    ops_writer_push_signed(info, OPS_SIG_BINARY, skey);

    int input_fd = open(myfile, O_RDONLY | O_BINARY);
    CU_ASSERT(input_fd >= 0);
    for (;;)
        {
        ssize_t n = read(input_fd, buffer, MAXBUF);
        CU_ASSERT(n >= 0);
        if (n == 0)
	    break;
        ops_write(buffer, n, info);
        }
    close(input_fd);
    ops_writer_close(info);
    ops_memory_t *signed_memory = copy_partial_packet(tmp);
    ops_write_file_from_buf(signed_file, ops_memory_get_data(signed_memory),
			    ops_memory_get_length(signed_memory), overwrite);
    ops_teardown_memory_write(info, tmp);
    ops_memory_free(signed_memory);
    check_sig(signed_file, use_armour);
    }

static void test_rsa_signature_sign_memory(const int use_armour,
					   const void* input,
					   const int input_len,
					   const ops_secret_key_t *skey)
    {
    ops_memory_t* mem=NULL;
    ops_parse_info_t *pinfo=NULL;
    validate_data_cb_arg_t validate_arg;

    mem=ops_sign_buf(input, input_len, OPS_SIG_TEXT, skey, use_armour);

    /*
     * Validate output
     */

    if (debug)
        {
        fprintf(stderr,"\n***\n*** Starting to parse for validation\n***\n");
        }
    
    ops_write_file_from_buf("/tmp/memory.asc", ops_memory_get_data(mem),
			    ops_memory_get_length(mem), ops_true);

    ops_setup_memory_read(&pinfo, mem, &validate_arg, callback_verify,
			  ops_true);

    check_sig_with_ops_core(pinfo, use_armour, &validate_arg);

    ops_memory_free(mem);
    }

static void test_rsa_signature_large_noarmour_nopassphrase(void)
    {
    assert(pub_keyring.nkeys);
    test_rsa_signature_sign(OPS_UNARMOURED,
			    filename_rsa_large_noarmour_nopassphrase,
			    alpha_skey);

    test_rsa_signature_sign_stream(OPS_UNARMOURED,
                                   filename_rsa_large_noarmour_nopassphrase,
                                   alpha_skey);
    }

static void test_rsa_signature_large_armour_nopassphrase(void)
    {
    assert(pub_keyring.nkeys);
    test_rsa_signature_sign(OPS_ARMOURED,
			    filename_rsa_large_armour_nopassphrase, alpha_skey);

    test_rsa_signature_sign_stream(OPS_ARMOURED,
                                   filename_rsa_large_armour_nopassphrase,
                                   alpha_skey);
    }

static void test_rsa_signature_noarmour_nopassphrase(void)
    {
    unsigned char testdata[MAXBUF];
    assert(pub_keyring.nkeys);
    test_rsa_signature_sign(OPS_UNARMOURED, filename_rsa_noarmour_nopassphrase,
			    alpha_skey);
    create_testdata("test_rsa_signature_noarmour_nopassphrase", testdata,
		    MAXBUF);
    test_rsa_signature_sign_memory(OPS_UNARMOURED, testdata, MAXBUF,
				   alpha_skey);
    }
static void test_rsa_signature_noarmour_passphrase(void)
    {
    unsigned char testdata[MAXBUF];
    assert(pub_keyring.nkeys);
    test_rsa_signature_sign(OPS_UNARMOURED, filename_rsa_noarmour_passphrase,
			    bravo_skey);

    test_rsa_signature_sign_stream(OPS_UNARMOURED, filename_rsa_noarmour_passphrase,
                                   bravo_skey);

    create_testdata("test_rsa_signature_noarmour_passphrase", testdata, MAXBUF);
    test_rsa_signature_sign_memory(OPS_UNARMOURED, testdata, MAXBUF, bravo_skey);
    }

static void test_rsa_signature_armour_nopassphrase(void)
    {
    unsigned char testdata[MAXBUF];
    assert(pub_keyring.nkeys);
    test_rsa_signature_sign(OPS_ARMOURED, filename_rsa_armour_nopassphrase,
			    alpha_skey);
    test_rsa_signature_sign_stream(OPS_ARMOURED, filename_rsa_armour_nopassphrase,
                                   alpha_skey);

    create_testdata("test_rsa_signature_armour_nopassphrase", testdata, MAXBUF);
    test_rsa_signature_sign_memory(OPS_ARMOURED, testdata, MAXBUF, alpha_skey);
    }

static void test_rsa_signature_armour_passphrase(void)
    {
    unsigned char testdata[MAXBUF];

    assert(pub_keyring.nkeys);
    test_rsa_signature_sign(OPS_ARMOURED, filename_rsa_armour_passphrase,
			    bravo_skey);
    test_rsa_signature_sign_stream(OPS_ARMOURED, filename_rsa_armour_passphrase,
                                   bravo_skey);

    create_testdata("test_rsa_signature_armour_passphrase", testdata, MAXBUF);
    test_rsa_signature_sign_memory(OPS_ARMOURED, testdata, MAXBUF, bravo_skey);
    }

static void test_rsa_signature_clearsign_file_nopassphrase(void)
    {
    assert(pub_keyring.nkeys);
    test_rsa_signature_clearsign_file(filename_rsa_clearsign_file_nopassphrase,
				      alpha_skey);
    }

static void test_rsa_signature_clearsign_file_passphrase(void)
    {
    assert(pub_keyring.nkeys);
    test_rsa_signature_clearsign_file(filename_rsa_clearsign_file_passphrase,
				      bravo_skey);
    }

static void test_rsa_signature_clearsign_buf_nopassphrase(void)
    {
    assert(pub_keyring.nkeys);
    test_rsa_signature_clearsign_buf(filename_rsa_clearsign_buf_nopassphrase,
				     alpha_skey);
    }

static void test_rsa_signature_clearsign_buf_passphrase(void)
    {
    assert(pub_keyring.nkeys);
    test_rsa_signature_clearsign_buf(filename_rsa_clearsign_buf_passphrase,
				     bravo_skey);
    }
/*
static void test_todo(void)
    {
    CU_FAIL("Test FUTURE: Use other hash algorithms");
    CU_FAIL("Test FUTURE: Check for key expiry");
    CU_FAIL("Test FUTURE: Check for key revocation");
    CU_FAIL("Test FUTURE: Check for signature expiry");
    CU_FAIL("Test FUTURE: Check for signature revocation");
    }
*/

static int add_tests(CU_pSuite suite)
    {
    // add tests to suite
    
    if (NULL == CU_add_test(suite, "Unarmoured, no passphrase",
			    test_rsa_signature_noarmour_nopassphrase))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Unarmoured, passphrase",
			    test_rsa_signature_noarmour_passphrase))
	    return 0;
    if (NULL == CU_add_test(suite, "Clearsigned file, no passphrase",
			    test_rsa_signature_clearsign_file_nopassphrase))
	    return 0;

    if (NULL == CU_add_test(suite, "Clearsigned file, passphrase",
			    test_rsa_signature_clearsign_file_passphrase))
	    return 0;

    if (NULL == CU_add_test(suite, "Clearsigned buf, no passphrase",
			    test_rsa_signature_clearsign_buf_nopassphrase))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Clearsigned buf, passphrase",
			    test_rsa_signature_clearsign_buf_passphrase))
	    return 0;

    if (NULL == CU_add_test(suite, "Armoured, no passphrase",
			    test_rsa_signature_armour_nopassphrase))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Armoured, passphrase",
			    test_rsa_signature_armour_passphrase))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Large, no armour, no passphrase",
			    test_rsa_signature_large_noarmour_nopassphrase))
	    return 0;
    
    if (NULL == CU_add_test(suite, "Large, armour, no passphrase",
			    test_rsa_signature_large_armour_nopassphrase))
	    return 0;
    /*
    if (NULL == CU_add_test(suite, "Tests to be implemented", test_todo))
	    return 0;
    */
    return 1;
    }

CU_pSuite suite_rsa_signature()
    {
    CU_pSuite suite = NULL;

    suite = CU_add_suite("RSA Signature Suite", init_suite_rsa_signature,
			 clean_suite_rsa_signature);
    if (!suite)
	    return NULL;

    if (!add_tests(suite))
        return NULL;

    return suite;
    }

// EOF
