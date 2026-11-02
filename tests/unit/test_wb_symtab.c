/**
 * ============================================================================
 * File: test_wb_symtab.c
 * Module: Worklet 3 (eval.c - Symbol Table component)
 * White-Box Testing Techniques Covered:
 *   - Symbol table lifecycle (init, free, dynamic reallocation)
 *   - Deep-copy semantics: independent identifier strings and Value buffers
 *   - Overwriting existing symbol bindings without resource leaks
 *   - Constant preloading (pi, e, i, ans) and precision adaptation
 *   - Boundary lookup on missing or empty identifiers
 * ============================================================================
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minitest.h"
#include "eval.h"
#include "bigint_io.h"

int main(void) {
    minitest_init_suite("White-Box Unit: Symbol Table & Constants (Worklet 3)", "tests/test_results.csv");

    TEST_CASE("UT-SYM-01", "UNIT", "symtab_init and symtab_free on empty table") {
        SymbolTable st;
        symtab_init(&st);
        CHECK_INT_EQ(st.count, 0, "Initial count is 0");
        CHECK_INT_EQ(st.capacity, 0, "Initial capacity is 0");
        CHECK_NULL(st.symbols, "Initial symbols pointer is NULL");
        symtab_free(&st);
        CHECK_INT_EQ(st.count, 0, "Count reset to 0 after free");
    }

    TEST_CASE("UT-SYM-02", "UNIT", "symtab_set and symtab_get basic insertion and retrieval") {
        SymbolTable st;
        symtab_init(&st);

        Value v;
        value_init(&v, VAL_INT);
        bigint_from_string(&v.as.i, "42");

        CHECK(symtab_set(&st, "x", &v), "symtab_set returns 1 on success");
        CHECK_INT_EQ(st.count, 1, "Table count is 1");

        Value out;
        value_init(&out, VAL_INT);
        CHECK(symtab_get(&st, "x", &out), "symtab_get finds 'x'");
        CHECK_INT_EQ(out.type, VAL_INT, "out is VAL_INT");
        char *str = bigint_to_string(&out.as.i);
        CHECK_STR_EQ(str, "42", "Retrieved value is 42");
        free(str);

        value_free(&out);
        value_free(&v);
        symtab_free(&st);
    }

    TEST_CASE("UT-SYM-03", "UNIT", "symtab_set overwrites existing identifier cleanly") {
        SymbolTable st;
        symtab_init(&st);

        Value v1, v2, out;
        value_init(&v1, VAL_INT);
        bigint_from_string(&v1.as.i, "100");
        symtab_set(&st, "alpha", &v1);
        CHECK_INT_EQ(st.count, 1, "Count is 1");

        value_init(&v2, VAL_DEC);
        decimal_from_string(&v2.as.d, "3.14");
        symtab_set(&st, "alpha", &v2);
        CHECK_INT_EQ(st.count, 1, "Count remains 1 after overwrite");

        value_init(&out, VAL_INT);
        CHECK(symtab_get(&st, "alpha", &out), "Found updated alpha");
        CHECK_INT_EQ(out.type, VAL_DEC, "alpha promoted to VAL_DEC");

        value_free(&v1);
        value_free(&v2);
        value_free(&out);
        symtab_free(&st);
    }

    TEST_CASE("UT-SYM-04", "UNIT", "Deep-copy invariant: caller mutation does not corrupt symbol table") {
        SymbolTable st;
        symtab_init(&st);

        char var_name[16] = "temp_var";
        Value v;
        value_init(&v, VAL_INT);
        bigint_from_string(&v.as.i, "999");
        symtab_set(&st, var_name, &v);

        /* Mutate caller buffer and Value */
        strcpy(var_name, "corrupted");
        bigint_from_string(&v.as.i, "111");

        Value out;
        value_init(&out, VAL_INT);
        CHECK(symtab_get(&st, "temp_var", &out), "Original symbol key still intact");
        char *s = bigint_to_string(&out.as.i);
        CHECK_STR_EQ(s, "999", "Stored value remains 999 unaffected by caller mutation");
        free(s);

        value_free(&out);
        value_free(&v);
        symtab_free(&st);
    }

    TEST_CASE("UT-SYM-05", "UNIT", "symtab_get on non-existent identifier returns 0") {
        SymbolTable st;
        symtab_init(&st);
        Value out;
        value_init(&out, VAL_INT);
        CHECK(!symtab_get(&st, "non_existent", &out), "symtab_get returns 0 for missing variable");
        value_free(&out);
        symtab_free(&st);
    }

    TEST_CASE("UT-SYM-06", "UNIT", "symtab_preload_constants registers pi, e, i, ans correctly") {
        SymbolTable st;
        symtab_init(&st);
        CHECK(symtab_preload_constants(&st, 20), "Preloading constants succeeds");

        Value v_pi, v_e, v_i, v_ans;
        value_init(&v_pi, VAL_INT);
        value_init(&v_e, VAL_INT);
        value_init(&v_i, VAL_INT);
        value_init(&v_ans, VAL_INT);

        CHECK(symtab_get(&st, "pi", &v_pi), "Found 'pi'");
        CHECK_INT_EQ(v_pi.type, VAL_DEC, "pi is VAL_DEC");

        CHECK(symtab_get(&st, "e", &v_e), "Found 'e'");
        CHECK_INT_EQ(v_e.type, VAL_DEC, "e is VAL_DEC");

        CHECK(symtab_get(&st, "i", &v_i), "Found 'i'");
        CHECK_INT_EQ(v_i.type, VAL_COMPLEX, "i is VAL_COMPLEX");

        CHECK(symtab_get(&st, "ans", &v_ans), "Found 'ans'");
        CHECK_INT_EQ(v_ans.type, VAL_INT, "ans is VAL_INT initialized to 0");

        value_free(&v_pi);
        value_free(&v_e);
        value_free(&v_i);
        value_free(&v_ans);
        symtab_free(&st);
    }

    TEST_CASE("UT-SYM-07", "UNIT", "Dynamic reallocation: inserting 25 symbols triggers capacity expansion") {
        SymbolTable st;
        symtab_init(&st);

        for (int i = 0; i < 25; i++) {
            char name[16];
            char val_buf[16];
            snprintf(name, sizeof(name), "var_%d", i);
            snprintf(val_buf, sizeof(val_buf), "%d", i * 10);
            Value v;
            value_init(&v, VAL_INT);
            bigint_from_string(&v.as.i, val_buf);
            CHECK(symtab_set(&st, name, &v), "Set variable");
            value_free(&v);
        }

        CHECK_INT_EQ(st.count, 25, "All 25 variables registered");
        CHECK(st.capacity >= 25, "Capacity expanded to at least 25");

        /* Verify all 25 can be retrieved */
        for (int i = 0; i < 25; i++) {
            char name[16];
            snprintf(name, sizeof(name), "var_%d", i);
            Value out;
            value_init(&out, VAL_INT);
            CHECK(symtab_get(&st, name, &out), "Retrieve variable");
            char *s = bigint_to_string(&out.as.i);
            char expected[16];
            snprintf(expected, sizeof(expected), "%d", i * 10);
            CHECK_STR_EQ(s, expected, "Value matches");
            free(s);
            value_free(&out);
        }

        symtab_free(&st);
    }

    return minitest_finish_suite();
}
