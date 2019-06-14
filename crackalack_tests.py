#!/usr/bin/python3

#
# This program performs a series of tests the rainbow table generation and
# lookup code.
#


# Ensure that this script is run with Python3.
import shutil, sys, time
if sys.version_info.major != 3:
    print('Error: you must invoke this script with python3, not python.')
    exit(-1)

import glob, hashlib, os, platform, subprocess, shutil, tempfile

CLR = "\033[0m";
WHITEB = "\033[1;97m"; # White + bold
ITALICIZE = "\033[3m";
GREEN = "\033[0;32m";
RED = "\033[0;31m";
GREENB = "\033[1;32m"; # Green + bold
REDB = "\033[1;31m";   # Red + bold

GEN_PROG_NAME='crackalack_gen'
LOOKUP_PROG_NAME='crackalack_lookup'

CYGWIN=False
if platform.system().startswith('CYGWIN'):
    CYGWIN=True

VERBOSE=False


# Generation tests to run.  The crackalack_gen program is given the specified
# arguments, and the output table must match the specified sha256 hash.
# Python doesn't allow lists to be dictionary keys, so the hashes weirdly come first.
GEN_TESTS = {

# The LM tests below are valid, but LM chain generation is useless these days
# (LM hashes are mostly better off being brute-forced).
    #'8d95bf8d6e342f0b035b8ccb9c8b21e907f7ad7aa82390727ff131284f883116':
    #    ['lm', 'alpha-numeric-symbol32-space', '1', '7', '0', '15200', '20480', '0'],
    #'90a66f17ab7ecc7592d2294f9bbe7cf93024853ef5f758fd62d71628b41e13c9':
    #    ['lm', 'alpha-numeric-symbol32-space', '1', '7', '0', '15200', '20480', '3'],
    #'d0d8d33c48a47430dd80ca2d105136a3af84030d060ea9b032d7cbf132359489':
    #    ['lm', 'alpha-numeric-symbol32-space', '1', '7', '3', '15200', '20480', '0'],
    #'b1bc9339f64a54585deea18190c3908a19a57d9f97f20354a4ba09c4e38db208':
    #    ['lm', 'alpha', '1', '7', '5', '15200', '20480', '5'],
    #'c7b78031861588d6bad6011e6f6ebdf8e261c6d93fb18ef34e79eace22bed7fa':
    #    ['lm', 'mixalpha-numeric', '1', '7', '8', '15200', '20480', '8'],

# The test below is valid, but we have too many as it is, and its better to have
# coverage on the real-world parameters with 8 and 9 character tables.
#    'e8f8b8dca8f8b8ea31a8faa0ad6160edcb9a0413e45c3acf2d0bd6a4e5780dbd':
#        ['ntlm', 'ascii-32-65-123-4', '1', '8', '0', '1111', '8192', '0'],

    # Generic NTLM 8-character generation.
    'cad901a98addf66cd05460134d4baf6240c00fa0e17929a33a56c61c46b567bc':
        ['ntlm', 'ascii-32-95', '8', '8', '0', '10240', '10240', '333'],

    # Optimized NTLM8 kernel test.
    '3203f5596449254dc8f0639a8bbf54555c911aebc69d018b1e45cf0e8be27086':
        ['ntlm', 'ascii-32-95', '8', '8', '0', '422000', '10240', '0'],

    'c3e6a35810e2a70bac583cd9767d6181aa026fbb53325f230aa48b863c274b06':
        ['ntlm', 'ascii-32-95', '9', '9', '0', '10240', '10240', '666'],

    # Optimized NTLM9 kernel test.
#    'd80a48c8216ed71d50d35da735a51d8c32c1c779296142fc82582f8ff643d153':
#        ['ntlm', 'ascii-32-95', '9', '9', '0', '1000000', '10240', '0'],
}

# The 'table' key is the filename to use to make a fake rainbow table (as the
# precomputation parameters will be inferred from it).  The 'precalc_hash' key
# is the sha256 hash of the rcracki.precalc.0 file.  The 'index_hash' key is the
# sha256 hash of the rcracki.precalc.0.index file.
PRECOMP_TESTS = {
    0: {'table': 'ntlm_loweralpha#8-8_0_100x1024_0.rt', 'password_hash': '49e5bfaab1be72a6c5236f15736a3e15', 'precalc_hash': '19d665d6181415aa70f8c5487585a778526b4ca39ccb5dcfb04f7a0bc508593b', 'index_hash': '0a4a8f162529d5e41f8df5e0a5438ff3890ca7f7dbbec2599eac8d15ca0c2e03'},
    1: {'table': 'ntlm_ascii-32-95#8-8_16_100x1024_0.rt', 'password_hash': '49e5bfaab1be72a6c5236f15736a3e15', 'precalc_hash': '79b4cf5ccae26544d35aacd424fd97c39e024c9e5430c6c546e99f6dfd2dccf0', 'index_hash': '183c7687853f9344e8c047bf3d5f30b627988a233e2a6cafb71ee00151aa536f'},
    2: {'table': 'ntlm_ascii-32-95#9-9_1024_100x1024_0.rt', 'password_hash': '49e5bfaab1be72a6c5236f15736a3e15', 'precalc_hash': 'feadc19362a8daa7291ae60f1ee5b68b059f42cd7287dd7044b4535876f30142', 'index_hash': 'ea7081e000872ec06a115e068643048dcdecda047fe665ef6ca74ee68dd7e384'},
}

pot_filename = 'temp.pot'


# Ensures that the precalc files match their expacted values.  If the expected values
# are None, then the precalc files must not exist.  Returns True when expected values
# match, otherwise False.
def check_precalc_files(temp_dir, index, precalc_index_file_hash_expected, precalc_file_hash_expected, delete=False):
    precalc_file = os.path.join(temp_dir, "rcracki.precalc.%u" % index)
    precalc_index_file = os.path.join(temp_dir, "rcracki.precalc.%u.index" % index)

    # If the expected hashes are both None, then the files are expected to not exist.
    if precalc_index_file_hash_expected is None and precalc_file_hash_expected is None:
        precalc_file_exists = os.path.exists(precalc_file)
        precalc_index_file_exists = os.path.exists(precalc_index_file)
        if precalc_file_exists or precalc_index_file_exists:
            print("FAILED: precalc and/or index file exists: %s: %r; %s: %r" % (precalc_file, precalc_file_exists, precalc_index_file, precalc_index_file_exists))
            return False
        else:
            return True

    # Check that the precalc and precalc index files are properly created.
    precalc_file_hash_actual = get_hash(precalc_file)
    precalc_index_file_hash_actual = get_hash(precalc_index_file)
    if precalc_file_hash_actual != precalc_file_hash_expected:
        print("FAILED: precalc file hash mismatch!: %s\n\tExpected: %s\n\tActual:   %s" % (precalc_file, precalc_file_hash_expected, precalc_file_hash_actual))
        return False

    if precalc_index_file_hash_actual != precalc_index_file_hash_expected:
        print("FAILED: precalc index file hash mismatch!: %s\n\tExpected: %s\n\tActual:   %s" % (precalc_index_file, precalc_index_file_hash_expected, precalc_index_file_hash_actual))
        return False

    # Delete the precalc files if the caller prefers.
    if delete:
        os.unlink(precalc_file)
        os.unlink(precalc_index_file)

    return True


# Checks that the pot file contains all the specified plaintexts (if None, the pot file
# is expected to not exist).  Returns True if the expected case is found, otherwise
# False.
def check_pot_file(pot_filename, plaintexts):

    # If no hash was specified, then the pot file must not exist.
    if (plaintexts is None) or (len(plaintexts) == 0):
        if os.path.exists(pot_filename):
            print("FAILED: pot file exists when it shouldn't!: %s" % pot_filename)
            return False
        else:
            return True

    pot_lines = []
    with open(get_real_path(pot_filename), 'r') as f:
        for line in f:
            pot_lines.append(line)

    # Make a deep copy of the pot lines.
    unmatched_lines = []
    for pot_line in pot_lines:
        unmatched_lines.append(pot_line)

    for plaintext in plaintexts:
        # Check each unmatched line in the pot file to see if this plaintext is in it.
        found = False
        for pot_line in pot_lines:
            if pot_line.strip().endswith(plaintext):
                found = True

                # Since this line is matched, remove it from the unmatched list.
                unmatched_lines.remove(pot_line)
                break

        if found is False:
            print("FAILED: pot file does not have the following plaintext: %s\npot file: [%s]" % (plaintext, "\n".join(pot_lines)))
            return False

    # Ensure that extra lines aren't in the pot file.
    if len(unmatched_lines) > 0:
        print("FAILED: extra entries found in pot file!: %s" % ''.join(unmatched_lines))
        return False

    return True


# Creates a sorted rainbow table that is either completely fake, or mostly fake with
# a few valid chains sprinkled in.
def create_rt_table(rt_dir, filename, num_chains, real_chains=[]):
    path = os.path.join(rt_dir, filename)

    start_time = time.time()

    # Add the user-supplied authentic chains.  Convert the start index to bytes, but
    # leave the end index as an integer (so they can be sorted below).
    chains = []
    for i in range(0, len(real_chains)):
        chains.append((real_chains[i][0].to_bytes(8, byteorder='little'), real_chains[i][1]))

    # Generate random start & end indices for the rest of the table.
    for i in range(len(chains), num_chains):
        chains.append((os.urandom(8), int.from_bytes(os.urandom(8), byteorder='little')))

    # Sort the table by the end indices.
    sorted_chains = sorted(chains, key=lambda x: x[1])

    # Write the chains to the specified file path.
    with open(path, 'wb') as f:
        for i in range(0, num_chains):
            f.write(sorted_chains[i][0])
            f.write(sorted_chains[i][1].to_bytes(8, byteorder='little'))
              
    # If the the calls to os.urandom() take too long, warn the user.
    total_time = int(time.time() - start_time)
    if total_time > 3:
        print("\nWARNING: creation of fake table %s with %u chains took %u seconds.\n" % (path, num_chains, total_time))

    return path


# Returns the SHA-256 hash of a file, or None if the file does not exist.
def get_hash(filename):

    ret = None
    try:
        with open(filename, 'rb') as f:
            ret = hashlib.sha256(f.read()).hexdigest()
    except FileNotFoundError as e:
        pass

    return ret


# Do the precomputation tests.
def do_precomp_tests(temp_dir):
    all_passed = True
    for test_number in PRECOMP_TESTS:
        pot_filepath, rt_dir = begin_lookup_test(temp_dir)

        test_data = PRECOMP_TESTS[test_number]
        table_filename = test_data['table']
        password_hash = test_data['password_hash']
        precalc_hash_expected = test_data['precalc_hash']
        index_hash_expected = test_data['index_hash']

        # Create a fake rainbow table and run the lookup program against it.  The
        # precomputed files will be generated using parameters inferred from the
        # table's filename.  Also, the lookup will fail, leaving the precompute files
        # behind for us to examine.
        fake_table = create_rt_table(rt_dir, table_filename, 1024)

        print('Precomputing hash %s against %s... ' % (password_hash, table_filename), end='', flush=True)
        run_lookup(rt_dir, password_hash)
        os.unlink(fake_table);

        if not check_precalc_files(temp_dir, 0, index_hash_expected, precalc_hash_expected):
            all_passed = False
        else:
            print(" %spassed.%s" % (GREEN, CLR))

    return all_passed


def get_real_path(path):
    if CYGWIN:
        return subprocess.run(['cygpath', '-w', path], stdout=subprocess.PIPE).stdout.decode('ascii').strip()
    else:
        return path


# Run the lookup program with the specified rainbow table directory and password hash
# (or file path to password hashes).
def run_lookup(rt_dir, password_hash, pot_filepath=None):

    # If the password hash is actually a file on disk, translate it to the real path (on Cygwin).
    if os.path.exists(password_hash):
        password_hash = get_real_path(password_hash)

    args = [lookup_prog_path] + [get_real_path(rt_dir), password_hash]

    if pot_filepath is not None:
        args.append(get_real_path(pot_filepath))

    # If verbose mode is on, print the output to stdout and stderr.
    so = stdout=subprocess.DEVNULL
    se = stderr=subprocess.DEVNULL
    if VERBOSE:
        so = None
        se = None

    subprocess.run(args, stdout=so, stderr=se)


# Do the lookup tests.
def do_lookup_tests(temp_dir):
    all_passed = True

    if do_lookup_test_1(temp_dir):
        print("\t* Lookup test #1 %spassed.%s" % (GREEN, CLR))
    else:
        print("%sFailed%s lookup test #1" % (RED, CLR))
        all_passed = False

    if do_lookup_test_2(temp_dir):
        print("\t* Lookup test #2 %spassed.%s" % (GREEN, CLR))
    else:
        print("%sFailed%s lookup test #2" % (RED, CLR))
        all_passed = False

    if do_lookup_test_3(temp_dir):
        print("\t* Lookup test #3 %spassed.%s" % (GREEN, CLR))
    else:
        print("%sFailed%s lookup test #3" % (RED, CLR))
        all_passed = False

    if do_lookup_test_4(temp_dir):
        print("\t* Lookup test #4 %spassed.%s" % (GREEN, CLR))
    else:
        print("%sFailed%s lookup test #4" % (RED, CLR))
        all_passed = False

    if do_lookup_test_5(temp_dir):
        print("\t* Lookup test #5 %spassed.%s" % (GREEN, CLR))
    else:
        print("%sFailed%s lookup test #5" % (RED, CLR))
        all_passed = False

    return all_passed


# Run lookup on single hash (via command line) against bogus table, then run it against
# a table with the solution.
def do_lookup_test_1(temp_dir):
    pot_filepath, rt_dir = begin_lookup_test(temp_dir)

    # Create an entirely bogus rainbow table.
    bogus_table_path = create_rt_table(rt_dir, 'ntlm_ascii-32-95#8-8_128_100x1024_0.rt', 1024)

    password_hash = 'a1ce652747dc7ad8f1a1579f2e5552f9'

    # The lookup process will find nothing.
    run_lookup(rt_dir, password_hash, pot_filepath)
    os.unlink(bogus_table_path)

    # Ensure that the pot file does not exist.
    if not check_pot_file(pot_filepath, None):
        return False

    # Check the hashes of the precalc files.
    if not check_precalc_files(temp_dir, 0, 'f8d0743b62efb72fb4e3fc4ecf933974e8904269560be671da57dc4a887390a0', '838d5c9d2b91e46291644e9d7d8fbd726f4572ae724de7f9bf5ad25718bd13a4'):
        return False

    # Create a table with the correct chain.
    real_table = create_rt_table(rt_dir, 'ntlm_ascii-32-95#8-8_128_100x1024_0.rt', 1024, [(666, 814103150699223)])

    # This time, the lookup will succeed.
    run_lookup(rt_dir, password_hash, pot_filepath)
    os.unlink(real_table)

    # The precalc and precalc index files should not exist, as they should have been
    # deleted upon successful lookup.
    if not check_precalc_files(temp_dir, 0, None, None):
        return False

    # Ensure that the pot file exists and has the correct contents.
    if not check_pot_file(pot_filepath, ['FYpzudMN']):
        return False

    shutil.rmtree(rt_dir)
    return True


# Perform lookup on hash from command line on one table with the solution.
def do_lookup_test_2(temp_dir):
    pot_filepath, rt_dir = begin_lookup_test(temp_dir)

    # Create a table with the correct chain.
    real_table = create_rt_table(rt_dir, 'ntlm_ascii-32-95#8-8_64_100x1024_0.rt', 1024, [(985, 433833498526988)])
    run_lookup(rt_dir, '1ae8e2c70bd95334f716edb522653a44', pot_filepath)
    os.unlink(real_table)

    if not check_pot_file(pot_filepath, ['MEH*^~7F']):
        return False

    return True


# Put four hashes into a file.  Run against a bogus table first.  Next, crack the first
# hash.  Lastly, crack hashes #2 and #3.
def do_lookup_test_3(temp_dir):
    pot_filepath, rt_dir = begin_lookup_test(temp_dir)

    # Write four hashes to hashes.txt.  Only the first three will be cracked.
    hashes_file = os.path.join(temp_dir, "hashes.txt")
    with open(hashes_file, 'w') as f:
        f.write("cbd0ab7936e84a60cf94ce55ab9c1448\n2627ce94b7adcc0b5be394ec6e2293dc\n76f1948b006c026b606886b39653f812\n4ecc2ad7428a2c641500a58bfd02009f")

    # Create a fake table, and attempt a lookup.
    fake_table = create_rt_table(rt_dir, 'ntlm_ascii-32-95#8-8_32_100x1024_0.rt', 16384)
    run_lookup(rt_dir, hashes_file, pot_filepath)
    os.unlink(fake_table)

    if not check_precalc_files(temp_dir, 0, '77028afda2ec9749dfe5f921267a0cc8d9cb4d36a75b4e1d821f5a67702fba1d', '2ee034e24967c8be383d0ebccc370ebfabe13e75ed1f663eb04925166630f71c'):
        return False
    if not check_precalc_files(temp_dir, 1, '7afb3c6210f231514ac1b2af0e840d4a687a3bc85dc324c2bb4b50e8bdc743d0', 'f0e08c079910983a3f57513a1f837b4497da9a72ba2bcd939a6435abd8e5e988'):
        return False
    if not check_precalc_files(temp_dir, 2, '2bab1297cbee537c16a909869f677469fbb08d8df77ec5da05de3126693c6de4', '49d1cf85db647faf87469dc42b8d67ddb781311f73cca81aa79c7c23c17d05e3'):
        return False
    if not check_precalc_files(temp_dir, 3, '3d64b323a1732f5bb1aa957fe786b13f5fa90efbcc479ac0a40e69029adee307', '45f6259dc0d404e8c1a6afb0012faca5d0edcb4baf13d92a845bab0229e0be3f'):
        return False

    # Ensure the pot file is still non-existent.
    if not check_pot_file(pot_filepath, None):
        return False


    # Solve one hash out of the four provided.
    real_table = create_rt_table(rt_dir, 'ntlm_ascii-32-95#8-8_32_100x1024_0.rt', 16384, [(955, 467938381128153)])
    run_lookup(rt_dir, hashes_file, pot_filepath)
    os.unlink(real_table)

    if not check_precalc_files(temp_dir, 0, None, None):
        return False
    if not check_precalc_files(temp_dir, 1, '7afb3c6210f231514ac1b2af0e840d4a687a3bc85dc324c2bb4b50e8bdc743d0', 'f0e08c079910983a3f57513a1f837b4497da9a72ba2bcd939a6435abd8e5e988'):
        return False
    if not check_precalc_files(temp_dir, 2, '2bab1297cbee537c16a909869f677469fbb08d8df77ec5da05de3126693c6de4', '49d1cf85db647faf87469dc42b8d67ddb781311f73cca81aa79c7c23c17d05e3'):
        return False
    if not check_precalc_files(temp_dir, 3, '3d64b323a1732f5bb1aa957fe786b13f5fa90efbcc479ac0a40e69029adee307', '45f6259dc0d404e8c1a6afb0012faca5d0edcb4baf13d92a845bab0229e0be3f'):
        return False

    # Ensure the pot file is still non-existent.
    if not check_pot_file(pot_filepath, ['v&Uf*Ml\\']):
        return False


    # Solve two hashes out of the four provided.
    real_table = create_rt_table(rt_dir, 'ntlm_ascii-32-95#8-8_32_100x1024_1.rt', 16384, [(1655, 478778248563219), (1047, 4236649556986690)])
    run_lookup(rt_dir, hashes_file, pot_filepath)
    os.unlink(real_table)

    if not check_precalc_files(temp_dir, 0, None, None):
        return False
    if not check_precalc_files(temp_dir, 1, None, None):
        return False
    if not check_precalc_files(temp_dir, 2, None, None):
        return False
    if not check_precalc_files(temp_dir, 3, '3d64b323a1732f5bb1aa957fe786b13f5fa90efbcc479ac0a40e69029adee307', '45f6259dc0d404e8c1a6afb0012faca5d0edcb4baf13d92a845bab0229e0be3f'):
        return False

    # Ensure the pot file is updated.
    if not check_pot_file(pot_filepath, ['v&Uf*Ml\\', '<krj:VsG', 'bOk;;UI[']):
        return False

    return True


# Put three hashes into a file.  Crack them all in one table.
def do_lookup_test_4(temp_dir):
    pot_filepath, rt_dir = begin_lookup_test(temp_dir)

    # Write three hashes to hashes.txt.  They will all be cracked.
    hashes_file = os.path.join(temp_dir, "hashes.txt")
    with open(hashes_file, 'w') as f:
        f.write("cbd0ab7936e84a60cf94ce55ab9c1448\n2627ce94b7adcc0b5be394ec6e2293dc\n76f1948b006c026b606886b39653f812")

    # Create a table with all the solutions, and crack all the hashes at once.
    real_table = create_rt_table(rt_dir, 'ntlm_ascii-32-95#8-8_32_100x1024_0.rt', 16384, [(955, 467938381128153), (1655, 478778248563219), (1047, 4236649556986690)])
    run_lookup(rt_dir, hashes_file, pot_filepath)
    os.unlink(real_table)

    # Ensure no precalc files exist.
    if not check_precalc_files(temp_dir, 0, None, None):
        return False
    if not check_precalc_files(temp_dir, 1, None, None):
        return False
    if not check_precalc_files(temp_dir, 2, None, None):
        return False

    # Ensure the pot file is updated.
    if not check_pot_file(pot_filepath, ['v&Uf*Ml\\', 'bOk;;UI[', '<krj:VsG']):
        return False

    return True


# RTC table lookup on four hashes.
def do_lookup_test_5(temp_dir):
    pot_filepath, rt_dir = begin_lookup_test(temp_dir)

    # Write four hashes to hashes.txt.  They will all be cracked.
    hashes_file = os.path.join(temp_dir, "hashes.txt")
    with open(hashes_file, 'w') as f:
        f.write("cbc066670f34690d3025a483492549d8\n88bc7eb324fb187557765cf82690cc25\ncc5905a8a1fa3ac6703e6cb8caad97cb\na187d0d4e369ab1eea93cdafd92dfc78")

    # Write a real RTC table.
    rtc_table_filename = os.path.join(rt_dir, 'ntlm_ascii-32-95#8-8_96_100x128_0.rtc')
    with open(rtc_table_filename, 'wb') as f:
        f.write(b'RTC0\x07\x002\x00\x00\x00\x00\x00\x00\x00\x00\x00\xd7\x97\x06\x12\x84\x95\xfe\xff\x7f\xb5\xec\xbd\xc4.\x00\x00\xee\x83\x81]\xb6}\xcb\x00>\x8a\xd6V\x1c\x13\xbb\x00.\x94,\xb9+j\xb6\x00o\xa0\xb06\xa8\x15\xc1\x00\xf4\xa4\x84\xb7\xab2\xb0\x00z\xb1\xf9m\xc7\x9a\xa5\x00\xa7A*dwx\x9d\x00\x0e\xafM\x16\xe8\n\xb6\x00+\xe8\xd3\xc1\xbd\x8c\xd4\x00\xfco\xf2\xc2\xc1\x06\xd0\x00\xcd\xcd\x87-\xbf\n\xd8\x00\xb4\x93zO\xf0\xef\xc7\x00\xb0\xb19v\xe2\x95\xb6\x00\xdf4\xb3`\n\xdf\xa2\x00\xac\xc5\xc5\x1f}$\x91\x00\x929p\x91\xcc\x95\x8f\x00\xc6HCy\xb1\xf0\xa1\x00%\x17*x-E\x99\x00\x9b\xc0Z\xe8A\xe2\x8c\x00\xb9E\xa0A\x1cV\x85\x00\x15$\xe07\xbd9\x8c\x00\xfb\xec\xc4\xbdXC\xb2\x007\xfa[\xce\xb0=\xd1\x003\xdb\x89\xd62\x8e\xd6\x00Z\xfc\xa0\xcc\xa7s\xbf\x00\xe6\xde\xab\x93\x94\xc4\xc3\x00\xba\x14g?)\xe9\xb6\x00\xb6X\xe8E\x05\x12\xa4\x00\x86\x87Ub\xe7\x8e\x8d\x00\xadO\x93kE\x98\x06\x01L\xf0ke\xd6\x8c\xf9\x00\x8chd\xb3/|\xed\x00\xd1\xe2a*\xa6k\xf7\x00u\xb7,\x7f\x06&\xea\x00\xfe\x9e\xd2\xa7\x97J\xec\x00\x85\xfcON\'\xc9\xfb\x00#C.\xebT\x0f\xf4\x00Xa\xe2\x17\xb6\x95\xf8\x00=\xa4k*\xa7C\xef\x00\xc7\x05\xab^\x02\x12\xfc\x00\xc04\xf9$0\x1a\xef\x00\xc3\n,\x10v\x1e\xe4\x00\x1e\xbc\x80\xc0\xfa\xe9\xe5\x00\xea`\xf1\x93\x08\xc9\xda\x00R\x90\xf2\x1b\x04\xa1\xcc\x00\x07\xe2\x02\xd2\xc4\xb4\xda\x00\x198M\x93\x7f2\xe3\x00\x83E\xc3\xce\x97l\xe6\x00\x0f\xc3{\xaf\x12\xb6\xd4\x00\xb8\x1bs\xbe\xd0\x92\xea\x00\xc5s>oV\x05\xd9\x00W\x91do_\xdb\xed\x00&\x86\xf76\x03Y\xe2\x00\t\xc8&\x9c\xf0\x0b\xcc\x00\x01|\x88\xc6\xa2\x12\xc7\x00\xfduR,\x02G\xbb\x00\xa8x[\xd3#\xad\xbf\x00\x13/\xaa\x04\xbb\xc9\xb6\x00\x1a{\xfa\xc0\x10\x96\xb4\x00A\xd7\xc2\xf5\x0f\x9a\xa3\x00\x17\x90\xed:\xb2\xe0\x96\x00\xd4\xf0\xa6\xb9\x9f\x8d\x90\x00\x88\xa8\xc2B\xaf\\\x84\x00\xd6w\xe6z-\xe7\x9c\x00\xd05J\xc0\xb7\x12\x8e\x00\x8di\xc5r\xcaj\x95\x00\xde\xdftlR\xa4\xa0\x00 $\xd8c\xc0\x93\x97\x00\xe7KR\xb4\xb8\x84\x96\x00\xa9V\xf0\xe8`;\xb6\x00\x7f\xe2b\xf2\x9bj\xaa\x00\x14\xd5\x0c\x1e|l\xe2\x00b\x08\x8d\xfd6\xf6\xfd\x00*\xe3\xb7\xd9\xf0U\xe8\x001\x84\xf1\x13\xdf\x9c\xd4\x00\x1f\xb8\xf6\xa8\xfb\xfb\xcb\x00q\x0c\xda\x95\x0f\xd5\xc4\x00/\xa4p\x17$K\xb2\x00\xf9x7M\xc0\xf4\xa6\x00v\xc9W\xea)~\x91\x00ro\xe9\xa8\xd2\x92z\x00\xf7L9\xb5\xaaQ\x81\x00\x00\xc2t\xfc\xf5Jj\x00"Z\x8d\xa0?`x\x00\xb5\xe2\xf98\x8d\x91g\x00\xce5\xb6\x02\x9b\xa8a\x00\xd9>\x02\xba\xb4TX\x00[\xaah\x14\x95/Y\x00\xf8\x01\xe0\xc5\xd8NE\x00\xcb\xcc\xb1\x18\xa0E=\x00D\xf4\xbeC<\xcc@\x00k\xb8\x14\x80Xm4\x00\x8a\x9b\xe2\x1d9\\3\x00d\xef\xa0a\x16A\x1d\x00\x90!\x1c\t\xbc\xccF\x00h\x16\x892:Y=\x00\xc8\x8f\xfc\xd1\xd5\xf6)\x00\xa1\xa0,6\xd3F\x13\x00O4\x85\xde\xca-\x00\x00\x0b\x07a\x97}\xb6\x19\x00\xdd~\xd1~\xd7\xb2\x18\x00\xf0f\x0c\xb4\xa1\x7f$\x00;\xc3\x1b\xbf,\xd3\\\x00\xdc\xd7\xcf\xb3*yN\x00\x11\xfch\xc7\xb0|:\x00\x04g@\nD\xf2\'\x00\x96\xe2#\x8f2/\x1d\x00\xbc\xa4N\x1a\xba\x03\x08\x00m\x00\x00\x00\x00\x00\x00\x00U>E\n\xbd\x1d"\x00S\xee\xd4\x10(h\x1e\x00\xe3d\xd3\x9dj%\x13\x00\xf3-\xbc\xd2X\x03i\x00\xc9\xca\xf4r\xf9\x8e\x88\x00i\x07\xea6\xab\x10\x99\x00\xca\xff\x1eQ$\xe3\xa3\x00\xc2p\xf5\x9e`*\x9a\x00\xbf\xa6\xcdo\x80$\xb6\x00a\xea\xd0D\xdc\xae\xee\x00\x1dAj\xd8\'\x91\xe3\x00\xe0\xdc\x1c"\x140\xd8\x00\xe5\x1c\x8b\xda\xae\x01\xd0\x00\x9c\xe8~\x02N\xe8\xce\x00\xb2Z;\x15\x996\xcb\x00\xa4\x8f\x05C\x91\xbf\xb5\x00l\\\x08`\r\x88\xa0\x00\x02\xbd\'\xff\xf0p\xb6\x00\x98\xbd\x81]\xb6}\xcb\x00')

    run_lookup(rt_dir, hashes_file, pot_filepath)
    os.unlink(rtc_table_filename)

    # Ensure no precalc files exist.
    if not check_precalc_files(temp_dir, 0, None, None):
        return False
    if not check_precalc_files(temp_dir, 1, None, None):
        return False
    if not check_precalc_files(temp_dir, 2, None, None):
        return False
    if not check_precalc_files(temp_dir, 3, None, None):
        return False

    # Ensure the pot file is updated.
    if not check_pot_file(pot_filepath, ['"{;iFoa{', ',u}&jU 6', '(EeFTfAS', 'r.Sq&7eN']):
        return False

    return True


# Deletes the pot file if it exists, along with all precompute files.  Creates the
# rainbowtable directory.  Returns paths to the pot file and rainbow table directory.
def begin_lookup_test(path):

    # Delete the pot file if it exists.
    pot_filepath = os.path.join(path, pot_filename)
    if os.path.exists(pot_filepath):
        os.unlink(pot_filepath)

    # Delete all the precompute files, if any.
    for f in os.listdir(path):
        if f.startswith('rcracki.precalc.'):
            os.unlink(os.path.join(path, f))

    # Create the rainbow table directory.
    rt_dir = os.path.join(temp_dir, "lookup_rt_%u" % int.from_bytes(os.urandom(4), byteorder='little'))
    os.mkdir(rt_dir)

    return pot_filepath, rt_dir

        
if __name__ == '__main__':
    tests_to_run = 'all'

    if (len(sys.argv) == 2) and (sys.argv[1] == '--verbose'):
        VERBOSE = True
    elif (len(sys.argv) == 3) and (sys.argv[1] in ['precomp', 'lookup', 'generate']):
        tests_to_run = sys.argv[1]
        VERBOSE = True
    elif (len(sys.argv) == 2) and (sys.argv[1] == '--help'):
        print("\nUsage: %s [precomp | lookup | generate | --verbose]\n\nWith no args, all tests are run.  Otherwise, the 'precomp', 'lookup', or 'generate' arguments will run those respective tests only (with verbose mode on).\n" % sys.argv[0])
        exit(0)

    # NVIDIA caches old kernels in ~/.nv/ComputeCache.  We need to delete it so we're
    # sure the tests run the actual kernels.
    compute_cache_dir = os.path.join(os.path.expandvars('~'), '/.nv/ComputeCache/')
    if os.path.isdir(compute_cache_dir):
        shutil.rmtree(compute_cache_dir)

    # AMD ROCm caches old kernels. in ~/.AMD/CLCache_rocm.
    compute_cache_dir = os.path.join(os.path.expandvars('~'), '/.AMD/CLCache_rocm/')
    if os.path.isdir(compute_cache_dir):
        shutil.rmtree(compute_cache_dir)
        
    # Since we will be changing the current working directory, get the absolute path
    # to the crackalack_gen program.
    gen_prog_path = os.path.abspath(GEN_PROG_NAME)
    lookup_prog_path = os.path.abspath(LOOKUP_PROG_NAME)

    # Make a temporary directory for us to generate tables in.
    temp_dir = tempfile.mkdtemp(prefix='crackalack_tests')

    # Copy the OpenCL code to the temp dir so the programs can find them.
    cl_dir = os.path.join(temp_dir, "CL")
    shutil.copytree('CL', cl_dir)
    shutil.copy('shared.h', cl_dir)

    # Change the working directory to the temporary directory.  This way crackalack_gen
    # creates files in here.
    os.chdir(temp_dir)

    all_passed = True

    if (tests_to_run == 'all') or (tests_to_run == 'precomp'):
        print("Performing pre-computation tests...\n")
        all_passed = all_passed and do_precomp_tests(temp_dir)

    if (tests_to_run == 'all') or (tests_to_run == 'lookup'):
        print("\n\nPerforming lookup tests...")
        all_passed = all_passed and do_lookup_tests(temp_dir)

    if (tests_to_run == 'all') or (tests_to_run == 'generate'):
        print("\n\nPerforming generation tests...\n")
        for expected_hash in GEN_TESTS:
            prog_args = GEN_TESTS[expected_hash]

            print('Generating table with args: %s... ' % ' '.join(prog_args), end='', flush=True)
            subprocess.run([gen_prog_path] + prog_args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

            test_passed = False
            # Go through all the files in the temp directory.  Calculate the sha256 hash
            # on the *.rt file, and compare it with the expected value.
            for filename in os.listdir(temp_dir):
                if filename.endswith('.rt'):
                    actual_hash = None
                    with open(filename, 'rb') as f:
                        actual_hash = hashlib.sha256(f.read()).hexdigest()
                    if actual_hash == expected_hash:
                        test_passed = True
                        print('%spassed.%s' % (GREEN, CLR))
                    else:
                        print("%sFAILED!%s\n  Expected: %s\n  Actual:   %s\n" % (RED, CLR, expected_hash, actual_hash))
                        print("Test directory: %s" % temp_dir)
                        sys.exit(-1)
                        all_passed = False

                    os.unlink(filename)

            all_passed = all_passed and test_passed


    # Delete the temporary directory and any files within it.
    shutil.rmtree(temp_dir)

    if all_passed:
        print("\n\t%sALL TESTS PASS!%s\n" % (GREENB, CLR))
    else:
        print("\n\n\tSOME TESTS %sFAILED!!%s\n" % (REDB, CLR))

    sys.exit(0 if all_passed == True else -1)
