#ifndef __nova_juce_bridge_h__
#define __nova_juce_bridge_h__

class NovaJuceBridge {
public:
	static void init();
	static void shutdown();
	static bool is_initialized();

private:
	static bool _initialized;
};

#endif /* __nova_juce_bridge_h__ */