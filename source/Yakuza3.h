#include "config.h"

#if _DEBUG
inline constexpr bool isDEBUG = true;
#else
inline constexpr bool isDEBUG = false;
#endif // _DEBUG
extern config::Config CONFIG;


namespace Yakuza3 {
	bool Init();
}
