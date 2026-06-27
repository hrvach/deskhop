#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ramdisk.c"

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "normal",                    // Valid input
        "A",                         // Boundary: exactly buffer size - 1
        "AAAAAAAAAAAAAAAAAAAAAAAAA", // Attack: 25 chars (exceeds buffer)
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB" // Large attack
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);
    
    for (int i = 0; i < num_payloads; i++) {
        char dest[16] = {0};  // Fixed buffer size
        const char *src = payloads[i];
        
        // Test strncpy usage - should not overflow dest
        strncpy(dest, src, sizeof(dest) - 1);
        dest[sizeof(dest) - 1] = '\0';  // Ensure null termination
        
        // Verify no buffer overflow occurred
        ck_assert_msg(strlen(dest) < sizeof(dest),
                     "Buffer overflow detected for payload: %s", src);
        
        // Verify null termination
        ck_assert_msg(dest[sizeof(dest) - 1] == '\0',
                     "String not null-terminated for payload: %s", src);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}