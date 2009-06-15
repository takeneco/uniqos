
#include <CUnit/CUnit.h>
#include "../cmdparse.c"
#include <stdio.h>

void test_strpart_1()
{
	const char tmp[] = "x";
	const char *res;

	res = strpart(NULL, tmp);

	CU_ASSERT_PTR_NULL(res);
}

void test_strpart_2()
{
	const char tmp[] = "x";
	const char *res;

	res = strpart(tmp, NULL);

	CU_ASSERT_PTR_NULL(res);
}

void test_strpart_3()
{
	const char tmp1[] = "abcdefg";
	const char tmp2[] = "abcdefghijkl";
	const char *res;

	res = strpart(tmp1, tmp2);

	CU_ASSERT_PTR_EQUAL(res, tmp2 + 7);
}

void test_strpart_4()
{
	const char tmp1[] = "abcdef";
	const char tmp2[] = "abcxyz";
	const char *res;

	res = strpart(tmp1, tmp2);

	CU_ASSERT_PTR_NULL(res);
}

void test_strpart_5()
{
	const char tmp1[] = "abcdefg";
	const char tmp2[] = "abc";
	const char *res;

	res = strpart(tmp1, tmp2);

	CU_ASSERT_PTR_NULL(res);
}

void test_shortargparse_1()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_SHORT_YES, "a" }, OPT_STYLE_NULL
	};
	struct opt_style style2[] = {
		{ OPTSTYLE_SHORT_NO, "b" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 },
		{ style2 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[2] = { -1, -1 };

	const char argv0[] = "ab";

	int res;

	res = shortargparse(argv0, type, parsed);

	CU_ASSERT_EQUAL(1, parsed[0].yesno);
	CU_ASSERT_EQUAL(0, parsed[1].yesno);
	CU_ASSERT_EQUAL(1, res);
}

void test_argparse_1()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_YES, "-a" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[1] = { -1 };

	int argc = 1;
	const char *argv[] = {
		"-a",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_EQUAL(1, parsed[0].yesno);
	CU_ASSERT_EQUAL(1, res);
}

void test_argparse_2()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_YES, "-a" },
		{ OPTSTYLE_YES, "-b" },
		{ OPTSTYLE_YES, "-c" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[3] = { -1 };

	int argc = 1;
	const char *argv[] = {
		"-b",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_EQUAL(1, parsed[0].yesno);
	CU_ASSERT_EQUAL(1, res);
}

void test_argparse_3()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_YES, "-a" },
		{ OPTSTYLE_NO,  "-b" },
		{ OPTSTYLE_YES, "-c" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[1] = { -1 };

	int argc = 1;
	const char *argv[] = {
		"-b",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_EQUAL(0, parsed[0].yesno);
	CU_ASSERT_EQUAL(1, res);
}

void test_argparse_4()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_YES, "-a" },
		{ OPTSTYLE_YES, "-b" },
		{ OPTSTYLE_YES, "-c" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[1] = { -1 };

	int argc = 1;
	const char *argv[] = {
		"-bc",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_EQUAL(-1, parsed[0].yesno);
	CU_ASSERT_EQUAL(0, res);
}

void test_argparse_5()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_AFTER, "-a" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[1] = { -1 };

	int argc = 1;
	const char *argv[] = {
		"-aXYZ",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_PTR_EQUAL(argv[0] + 2, parsed[0].ptr);
	CU_ASSERT_EQUAL(1, res);
}

void test_argparse_6()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_AFTER, "-a" },
		{ OPTSTYLE_AFTER, "-b" },
		{ OPTSTYLE_AFTER, "-c" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[1] = { -1 };

	int argc = 1;
	const char *argv[] = {
		"-bXYZ",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_PTR_EQUAL(argv[0] + 2, parsed[0].ptr);
	CU_ASSERT_EQUAL(1, res);
}

void test_argparse_7()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_NEXT, "-a" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[1] = { -1 };

	int argc = 2;
	const char *argv[] = {
		"-a", "XYZ",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_PTR_EQUAL(argv[1], parsed[0].ptr);
	CU_ASSERT_EQUAL(2, res);
}

void test_argparse_8()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_NEXT, "-a" },
		{ OPTSTYLE_NEXT, "-b" },
		{ OPTSTYLE_NEXT, "-c" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[1] = { -1 };

	int argc = 2;
	const char *argv[] = {
		"-b", "XYZ",
	};

	int res;

	res = argparse(argc, argv, type, parsed);

	CU_ASSERT_PTR_EQUAL(argv[1], parsed[0].ptr);
	CU_ASSERT_EQUAL(2, res);
}

void test_option_parse_1()
{
	struct opt_style style1[] = {
		{ OPTSTYLE_YES, "-a" }, OPT_STYLE_NULL
	};
	struct opt_style style2[] = {
		{ OPTSTYLE_NO, "-b" }, OPT_STYLE_NULL
	};
	struct opt_style style3[] = {
		{ OPTSTYLE_AFTER, "-c" }, OPT_STYLE_NULL
	};
	struct opt_style style4[] = {
		{ OPTSTYLE_NEXT, "-d" }, OPT_STYLE_NULL
	};
	struct opt_style style5[] = {
		{ OPTSTYLE_SHORT_YES, "e" }, OPT_STYLE_NULL
	};
	struct opt_style style6[] = {
		{ OPTSTYLE_SHORT_NO, "f" }, OPT_STYLE_NULL
	};
	struct opt_type type[] = {
		{ style1 },
		{ style2 },
		{ style3 },
		{ style4 },
		{ style5 },
		{ style6 }, OPT_TYPE_NULL
	};
	struct opt_parsed parsed[6] = { -1 };

	int argc = 6;
	const char *argv[] = {
		"-a", "-b", "-cXYZ", "-d", "ABC", "-ef"
	};

	int res;

	res = option_parse(argc, argv, type, parsed);

	CU_ASSERT_EQUAL(1, parsed[0].yesno);
	CU_ASSERT_EQUAL(0, parsed[1].yesno);
	CU_ASSERT_PTR_EQUAL(argv[2] + 2, parsed[2].ptr);
	CU_ASSERT_PTR_EQUAL(argv[4], parsed[3].ptr);
	CU_ASSERT_EQUAL(1, parsed[4].yesno);
	CU_ASSERT_EQUAL(0, parsed[5].yesno);
	CU_ASSERT_EQUAL(6, res);
}

void list_cmdparse()
{
	CU_TestInfo test[] = {
		{ "strpart_1", test_strpart_1 },
		{ "strpart_2", test_strpart_2 },
		{ "strpart_3", test_strpart_3 },
		{ "strpart_4", test_strpart_4 },
		{ "strpart_5", test_strpart_5 },
		{ "shortargparse_1", test_shortargparse_1 },
		{ "argparse_1", test_argparse_1 },
		{ "argparse_2", test_argparse_2 },
		{ "argparse_3", test_argparse_3 },
		{ "argparse_4", test_argparse_4 },
		{ "argparse_5", test_argparse_5 },
		{ "argparse_6", test_argparse_6 },
		{ "argparse_7", test_argparse_7 },
		{ "argparse_8", test_argparse_8 },
		{ "option_parse_1", test_option_parse_1 },
		CU_TEST_INFO_NULL
	};

	CU_SuiteInfo suite[] = {
		{ "cmdparse", NULL, NULL, test },
		CU_SUITE_INFO_NULL
	};

	CU_register_suites(suite);
}

