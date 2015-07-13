#pragma once

#ifdef PBL_COLOR
#define TRMainColor (GColor8){.argb=GColorSunsetOrangeARGB8}
#define TRAltColor (GColor8){.argb=GColorJazzberryJamARGB8}
#endif

void show_details(void);

void update_caption(char ca[512]);
void update_likes(char li[4]);
void update_comments(char co[4]);

void details_init(void);

void details_deinit(void);
