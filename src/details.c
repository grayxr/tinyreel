#include <pebble.h>
#include "details.h"

static void click_config_provider(void *context);
static void window_load(Window *window);
static void window_unload(Window *window);

static Window *window;

static BitmapLayer  *header_bitmap_layer;
static GBitmap      *header_bitmap = NULL;

static TextLayer    *likes_text_layer = NULL;
static TextLayer    *comments_text_layer = NULL;
static TextLayer    *caption_text_layer = NULL;

static ScrollLayer* scroll_layer = NULL;

const int vert_scroll_text_padding = 4;

void details_init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    #ifdef PBL_SDK_2
    window_set_fullscreen(window, true);
    #endif
}

void details_deinit(void) {
    window_destroy(window);
}

void show_details(void) {
    window_stack_push(window, true);
}

void update_caption(char ca[2048]){
    text_layer_set_text(caption_text_layer, ca);
    scroll_layer_set_content_size(scroll_layer, GSize(144, text_layer_get_content_size(caption_text_layer).h + 40));
};

void update_likes(char li[4]){
    text_layer_set_text(likes_text_layer, li);
};
void update_comments(char co[4]){
    text_layer_set_text(comments_text_layer, co);
};

static void click_config_provider(void *context) {
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    window_set_background_color(window, GColorWhite);

    #ifdef PBL_COLOR
		header_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DETAILS_B);
	#else
		header_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DETAILS_A);
	#endif
	header_bitmap_layer = bitmap_layer_create(GRect(0, -3, bounds.size.w, 44));
	bitmap_layer_set_bitmap(header_bitmap_layer, header_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(header_bitmap_layer));

	likes_text_layer = text_layer_create(GRect(0, 6, 50, 22));
    text_layer_set_font(likes_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text(likes_text_layer, "...");
    text_layer_set_background_color(likes_text_layer, GColorClear);
    #ifdef PBL_COLOR
        text_layer_set_text_color(likes_text_layer, TRAltColor);
    #else
        text_layer_set_text_color(likes_text_layer, GColorBlack);
    #endif
	text_layer_set_text_alignment(likes_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(likes_text_layer));

	comments_text_layer = text_layer_create(GRect(50, 6, 50, 22));
    text_layer_set_font(comments_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text(comments_text_layer, "...");
    text_layer_set_background_color(comments_text_layer, GColorClear);
    #ifdef PBL_COLOR
        text_layer_set_text_color(comments_text_layer, TRAltColor);
    #else
        text_layer_set_text_color(comments_text_layer, GColorBlack);
    #endif
	text_layer_set_text_alignment(comments_text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(comments_text_layer));

	scroll_layer = scroll_layer_create(GRect(0, 38, 144, 140));
	scroll_layer_set_content_size(scroll_layer, GSize(144, 140));
    #ifdef PBL_COLOR
        scroll_layer_set_shadow_hidden(scroll_layer, true);
    #endif
	scroll_layer_set_click_config_onto_window(scroll_layer, window);
	scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
		.click_config_provider = click_config_provider,
	});
    layer_add_child(window_get_root_layer(window), scroll_layer_get_layer(scroll_layer));

	caption_text_layer = text_layer_create(GRect(4, 2, 136, 1024));
	text_layer_set_font(caption_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(caption_text_layer, "...");
    #ifdef PBL_COLOR
        text_layer_set_text_color(caption_text_layer, TRAltColor);
    #else
        text_layer_set_text_color(caption_text_layer, GColorBlack);
    #endif
    text_layer_set_background_color(caption_text_layer, GColorClear);
	text_layer_set_text_alignment(caption_text_layer, GTextAlignmentLeft);
    scroll_layer_add_child(scroll_layer, text_layer_get_layer(caption_text_layer));
}

static void window_unload(Window *window) {
	gbitmap_destroy(header_bitmap);
    bitmap_layer_destroy(header_bitmap_layer);
	scroll_layer_destroy(scroll_layer);
	text_layer_destroy(caption_text_layer);
	text_layer_destroy(likes_text_layer);
	text_layer_destroy(comments_text_layer);
}
