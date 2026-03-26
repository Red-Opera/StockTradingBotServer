#include "Application.h"
#include "Log.h"

Application& Application::GetInstance()
{
	static Application instance;

	return instance;
}

void Application::Initialize()
{
	LifeCycle::Initialize();
}

Application::Application()
{
	Log::GetInstance();
}
