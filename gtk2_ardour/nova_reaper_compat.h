#ifndef __nova_reaper_compat_h__
#define __nova_reaper_compat_h__

struct lua_State;

class NovaReaperCompat {
public:
	static void inject(lua_State* L);
};

#endif /* __nova_reaper_compat_h__ */