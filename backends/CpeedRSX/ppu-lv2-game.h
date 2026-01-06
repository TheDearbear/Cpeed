#ifndef __PPU_LV2_GAME_H__
#define __PPU_LV2_GAME_H__

#include <ppu-lv2.h>

#ifdef __cplusplus
extern "C" {
#endif

LV2_INLINE s32 sys_game_set_system_sw_version(u64 version)
{
    lv2syscall1(375, version);
    return_to_user_prog(s32);
}

LV2_INLINE s32 sys_game_get_system_sw_version()
{
    lv2syscall0(376);
    return_to_user_prog(s32);
}

#ifdef __cplusplus
}
#endif

#endif
