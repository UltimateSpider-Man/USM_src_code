#pragma once

// ... for now, while this madness is still ongoing ...
// load v14 is possible but need verify v10+v14

// xbp
// v10  10.486.265.487.246 
// v14  14.589.301.585.277 
// v14  14.590.302.588.278 

#if defined(TARGET_XBOX) || defined(OPENUSM_XBPACK_MODE)
#define OPENUSM_XBOX_MASH_FORMAT 1
#else
#define OPENUSM_XBOX_MASH_FORMAT 0
#endif
