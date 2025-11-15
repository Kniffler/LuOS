#ifndef __LUOS_APP_LIST__
#define __LUOS_APP_LIST__

//	All apps have to be #include'ed here


//	------------------------------------

typedef struct app
{
	int id;	//	Cannot be 0, 0 means that the focus is on the directive terminal
	void (*keyEnter)(int);	//	Received every time a key is pressed
} app;

const app apps[] =
{
	
};

#endif	//	__LUOS_APP_LIST__
