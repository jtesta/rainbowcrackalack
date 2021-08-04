.PHONY:	archive clean tests test all

CC=gcc
COMPILE_OPTIONS=-Wall -g -O3
LINK_OPTIONS=-lpthread

# If we're doing a Windows build...
ifneq ($(WINDOWS_BUILD),)
  COMPILE_OPTIONS += -I$(CL_INCLUDE)
  LINK_OPTIONS += -static -lbcrypt

  ENUMERATE_PROG=enumerate_chain.exe
  GEN_PROG=crackalack_gen.exe
  GETCHAIN_PROG=get_chain.exe
  LOOKUP_PROG=crackalack_lookup.exe
  #PERFECTIFY_PROG=perfectify.exe
  RTC2RT_PROG=crackalack_rtc2rt.exe
  UNITTEST_PROG=crackalack_unit_tests.exe
  VERIFY_PROG=crackalack_verify.exe
else
  LINK_OPTIONS += -ldl

  ENUMERATE_PROG=enumerate_chain
  GEN_PROG=crackalack_gen
  GETCHAIN_PROG=get_chain
  LOOKUP_PROG=crackalack_lookup
  PERFECTIFY_PROG=perfectify
  RTC2RT_PROG=crackalack_rtc2rt
  UNITTEST_PROG=crackalack_unit_tests
  VERIFY_PROG=crackalack_verify
endif

ifneq ($(TRAVIS_BUILD),)
  COMPILE_OPTIONS += -D TRAVIS_BUILD=1
endif


all:	$(GEN_PROG) $(UNITTEST_PROG) $(LOOKUP_PROG) $(RTC2RT_PROG) $(GETCHAIN_PROG) $(VERIFY_PROG) $(PERFECTIFY_PROG) $(ENUMERATE_PROG)


%.o: %.c
	$(CC) $(COMPILE_OPTIONS) -o $@ -c $<

$(GEN_PROG):	charset.o clock.o cpu_rt_functions.o crackalack_gen.o file_lock.o gws.o hash_validate.o misc.o opencl_setup.o rtc_decompress.o verify.o
	$(CC) $(COMPILE_OPTIONS) -o $(GEN_PROG) charset.o clock.o cpu_rt_functions.o crackalack_gen.o file_lock.o gws.o hash_validate.o misc.o opencl_setup.o rtc_decompress.o verify.o $(LINK_OPTIONS)

$(UNITTEST_PROG):	charset.o cpu_rt_functions.o crackalack_unit_tests.o hash_validate.o misc.o opencl_setup.o  test_chain.o test_chain_ntlm9.o test_hash.o test_hash_ntlm9.o test_hash_to_index.o test_hash_to_index_ntlm9.o test_index_to_plaintext.o test_index_to_plaintext_ntlm9.o test_shared.o file_lock.o
	$(CC) $(COMPILE_OPTIONS) -o $(UNITTEST_PROG) charset.o cpu_rt_functions.o crackalack_unit_tests.o hash_validate.o misc.o opencl_setup.o test_chain.o test_chain_ntlm9.o test_hash.o test_hash_ntlm9.o test_hash_to_index.o test_hash_to_index_ntlm9.o test_index_to_plaintext.o test_index_to_plaintext_ntlm9.o test_shared.o file_lock.o $(LINK_OPTIONS)

$(GETCHAIN_PROG):	get_chain.o
	$(CC) $(COMPILE_OPTIONS) -o $(GETCHAIN_PROG) get_chain.o $(LINK_OPTIONS)

$(VERIFY_PROG):	charset.o cpu_rt_functions.o crackalack_verify.o file_lock.o hash_validate.o misc.o rtc_decompress.o verify.o
	$(CC) $(COMPILE_OPTIONS) -o $(VERIFY_PROG) charset.o cpu_rt_functions.o crackalack_verify.o file_lock.o hash_validate.o misc.o rtc_decompress.o verify.o $(LINK_OPTIONS)

$(RTC2RT_PROG):	rtc_decompress.o crackalack_rtc2rt.o
	$(CC) $(COMPILE_OPTIONS) -o $(RTC2RT_PROG) crackalack_rtc2rt.o rtc_decompress.o $(LINK_OPTIONS)

$(LOOKUP_PROG): clock.o cpu_rt_functions.o charset.o file_lock.o hash_validate.o crackalack_lookup.o misc.o opencl_setup.o rtc_decompress.o test_shared.o verify.o
	$(CC) $(COMPILE_OPTIONS) -o $(LOOKUP_PROG) charset.o clock.o cpu_rt_functions.o crackalack_lookup.o file_lock.o hash_validate.o misc.o opencl_setup.o rtc_decompress.o test_shared.o verify.o $(LINK_OPTIONS)

$(PERFECTIFY_PROG):	clock.o perfectify.o
	$(CC) $(COMPILE_OPTIONS) -o $(PERFECTIFY_PROG) clock.o perfectify.o

$(ENUMERATE_PROG):	cpu_rt_functions.o enumerate_chain.o test_shared.o
	$(CC) $(COMPILE_OPTIONS) -o $(ENUMERATE_PROG) cpu_rt_functions.o enumerate_chain.o test_shared.o


clean:
	rm -f *~ *.o *.exe *.zip *.sig crackalack_gen crackalack_unit_tests get_chain crackalack_verify crackalack_rtc2rt crackalack_lookup perfectify enumerate_chain

archive: clean
	./scripts/archive.sh

test:	$(UNITTEST_PROG) $(LOOKUP_PROG) $(GEN_PROG)
	./crackalack_unit_tests
	python3 crackalack_tests.py

tests:	$(UNITTEST_PROG) $(LOOKUP_PROG) $(GEN_PROG)
	./crackalack_unit_tests
	python3 crackalack_tests.py

.PHONY:	test tests clean archive
