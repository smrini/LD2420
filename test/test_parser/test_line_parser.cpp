#include <unity.h>

#include "LD2420Parser.h"

void test_parses_on_line() {
    LD2420ParsedLine result = ld2420ParseLine("ON");
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::PresenceOn, (int)result.type);
}

void test_parses_off_line() {
    LD2420ParsedLine result = ld2420ParseLine("OFF");
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::PresenceOff, (int)result.type);
}

void test_parses_range_line() {
    LD2420ParsedLine result = ld2420ParseLine("Range 105");
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::Range, (int)result.type);
    TEST_ASSERT_EQUAL_INT(105, result.distanceCm);
}

void test_trims_trailing_carriage_return() {
    // Lines arrive with a trailing \r in the real protocol (\r\n); the
    // parser should handle it whether or not the caller stripped it.
    LD2420ParsedLine result = ld2420ParseLine("Range 27\r");
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::Range, (int)result.type);
    TEST_ASSERT_EQUAL_INT(27, result.distanceCm);
}

void test_parses_multi_digit_and_single_digit_range() {
    TEST_ASSERT_EQUAL_INT(6, ld2420ParseLine("Range 6").distanceCm);
    TEST_ASSERT_EQUAL_INT(999, ld2420ParseLine("Range 999").distanceCm);
}

void test_ignores_unknown_line() {
    LD2420ParsedLine result = ld2420ParseLine("garbage");
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::Unknown, (int)result.type);
}

void test_ignores_empty_line() {
    LD2420ParsedLine result = ld2420ParseLine("");
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::Unknown, (int)result.type);
}

void test_handles_null_input() {
    LD2420ParsedLine result = ld2420ParseLine(nullptr);
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::Unknown, (int)result.type);
}

void test_range_prefix_is_case_and_space_sensitive() {
    // The real sensor always sends exactly "Range " (capital R, one
    // space) -- confirm we don't accidentally accept near-misses that
    // would silently misparse the distance.
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::Unknown, (int)ld2420ParseLine("range 105").type);
    TEST_ASSERT_EQUAL_INT((int)LD2420LineType::Unknown, (int)ld2420ParseLine("Range105").type);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_parses_on_line);
    RUN_TEST(test_parses_off_line);
    RUN_TEST(test_parses_range_line);
    RUN_TEST(test_trims_trailing_carriage_return);
    RUN_TEST(test_parses_multi_digit_and_single_digit_range);
    RUN_TEST(test_ignores_unknown_line);
    RUN_TEST(test_ignores_empty_line);
    RUN_TEST(test_handles_null_input);
    RUN_TEST(test_range_prefix_is_case_and_space_sensitive);
    return UNITY_END();
}
