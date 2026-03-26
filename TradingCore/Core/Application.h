#pragma once

#include "LifeCycle.h"

class Application : public LifeCycle
{
public:
	static Application& GetInstance();

	void Initialize() override;

private:
	Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
};

