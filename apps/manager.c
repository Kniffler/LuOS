#include "manager.h"
apps = NULL;
uint8_t open_app_exists = 0;
uint8_t directive_write_referal = 0;

void manager_init()
{
	struct app *a = NULL;
	a = (struct app *)malloc(sizeof(struct app));
	
	strcpy(a->call_name, "iowiz");
	a->app_open=&iowiz_open;
	a->app_directive_sent=&iowiz_directive_sent;
	a->app_key_press=&iowiz_key_press;
	
	HASH_ADD_STR(apps, call_name, a);
	*a = NULL;
}

void manager_open_app(char app_name[16])
{
	struct app *a = NULL;
	HASH_FIND_STR(apps, app_name, a);
	a->app_open();
}
