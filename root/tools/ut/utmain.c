
#include <CUnit/CUnit.h>

void list_cmdparse();

int main()
{
	CU_initialize_registry();

	list_cmdparse();

	CU_console_run_tests();
	CU_cleanup_registry();
}
