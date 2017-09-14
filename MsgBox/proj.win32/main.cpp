#include "stdafx.h"
#include "AppDelegate.h"

USING_NS_CC;

struct coroutine
{
	int result;
	int value;
	coroutine(int val_) :value(val_) {}

	int step_ = 0;
	void goNext()
	{
		switch (step_)
		{
		case 0:
			++step_;
			std::cout << "step 0" << std::endl;
			result = value * (rand() % 4);
			break;
		case 1:
			++step_;
			std::cout << "step 1" << std::endl;
			result = value * (rand() % 4);
			break;
		case 2:
			++step_;
			std::cout << "step 2" << std::endl;
			result = value * (rand() % 4);
			break;
		default:
			step_ = -1;
			break;
		}
	}
	bool done() const { return step_ < 0; }
	int currentValue() const { return result; }
};

#define CAT(a, b) a ## b
#define __GLOBAL_ABC(type, var, line) \
	static void CAT(foo, line)(){} \
	type var

#define GLOBAL_ABC(type, var) __GLOBAL_ABC(type, var, __LINE__)

#define ABC(a) __FUNCTION__ ? a : a;

 GLOBAL_ABC(int, globalValue);

int WINAPI _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	//GLOBAL_ABC(int, localValue);

	coroutine c(5);
	for (c.goNext(); !c.done(); c.goNext())
	{
		std::cout << c.currentValue() << std::endl;
	}

    // create the application instance
    AppDelegate app;
    return Application::getInstance()->run();
}
