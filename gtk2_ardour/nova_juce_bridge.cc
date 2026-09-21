#if defined(__x86_64__) || defined(__MINGW64__) || defined(_WIN64)
  #ifndef _WIN64
    #define _WIN64 1
  #endif
  #ifndef _M_X64
    #define _M_X64 1
  #endif
  #ifndef _M_AMD64
    #define _M_AMD64 1
  #endif
#endif

#include "nova_juce_header.h"
#include "nova_juce_bridge.h"

bool NovaJuceBridge::_initialized = false;

void
NovaJuceBridge::init()
{
	if (_initialized) return;

	juce::initialiseJuce_GUI();
	_initialized = true;
}

void
NovaJuceBridge::shutdown()
{
	if (!_initialized) return;

	juce::shutdownJuce_GUI();
	_initialized = false;
}

bool
NovaJuceBridge::is_initialized()
{
	return _initialized;
}