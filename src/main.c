#include <pebble.h>
#include "details.h"

static Window 		*window;

static BitmapLayer  *image_layer;
static GBitmap      *image = NULL;
static uint8_t      *data_image = NULL;
static uint32_t     data_size;

static TextLayer    *message_text_layer;
static TextLayer    *error_text_layer;
static TextLayer    *username_text_layer;

static char         msg_str[30];
static char         usr_str[30];
static char			capt_str[2048];
static char			likes_str[4];
static char			comments_str[4];

static int ErrorExists = 0;
static int ImgLoaded = 0;
static int Loading = 0;

#define KEY_IMAGE       0
#define KEY_INDEX       1
#define KEY_MESSAGE     2
#define KEY_SIZE        3
#define KEY_USERNAME    4
#define KEY_CAPTION     5
#define KEY_LIKES       6
#define KEY_COMMENTS    7
#define KEY_ERROR       8
#define KEY_NEXT        9
#define KEY_PREVIOUS    10
#define KEY_DETAILS     11

#define CHUNK_SIZE 1500


void prepend(char* s, const char* t)
{
    size_t len = strlen(t);
    size_t i;
    memmove(s + len, s, strlen(s) + 1);
    for (i = 0; i < len; ++i)
    {
        s[i] = t[i];
    }
}

static void cb_in_received_handler(DictionaryIterator *iter, void *context) {

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming data");

	// Reset
	ErrorExists = 0;
    text_layer_set_text(error_text_layer, "");

	// Set any messages
    Tuple *message_tuple = dict_find(iter, KEY_MESSAGE);
    if(message_tuple){
        //text_layer_set_background_color(message_text_layer, GColorClear);
        strcpy(msg_str, message_tuple->value->cstring);
        text_layer_set_text(message_text_layer, msg_str);
    }

    // Get the bitmap
    Tuple *size_tuple  = dict_find(iter, KEY_SIZE);
    if(size_tuple){
        if(data_image)
            free(data_image);
            data_size = size_tuple->value->uint32;
            data_image = malloc(data_size);
    }

	// Set the image
    Tuple *image_tuple = dict_find(iter, KEY_IMAGE);
    Tuple *index_tuple = dict_find(iter, KEY_INDEX);
    if (index_tuple && image_tuple) {
        int32_t index = index_tuple->value->int32;
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "image received index=%ld size=%d", index, image_tuple->length);
        memcpy(data_image + index,&image_tuple->value->uint8,image_tuple->length);
        if(image_tuple->length < CHUNK_SIZE){

			// Clear the image
			if(image){
				gbitmap_destroy(image);
				image = NULL;
			}

            #ifdef PBL_COLOR
            	image = gbitmap_create_from_png_data(data_image, data_size);
            #else
            	image = gbitmap_create_with_data(data_image);
            #endif

            bitmap_layer_set_bitmap(image_layer, image);
			text_layer_set_text(message_text_layer,"");
            layer_mark_dirty(bitmap_layer_get_layer(image_layer));

            ImgLoaded = 1;
            Loading = 0;

            text_layer_set_text(username_text_layer, usr_str);
    		text_layer_set_background_color(username_text_layer, GColorWhite);
    		if(ErrorExists == 1){
    			text_layer_set_background_color(username_text_layer, GColorClear);
    		}
        }
    }

	// Set any errors
    Tuple *error_tuple = dict_find(iter, KEY_ERROR);
    if(error_tuple){
        text_layer_set_text(message_text_layer,"");

		// Clear the image
		if(image){
			gbitmap_destroy(image);
			image = NULL;
		}
		bitmap_layer_set_bitmap(image_layer, image);

        Loading = 0;
		ErrorExists = 1;
		#ifdef PBL_COLOR
			image = gbitmap_create_with_resource(RESOURCE_ID_ERROR_B);
            text_layer_set_text_color(error_text_layer, GColorBlack);
		#else
			image = gbitmap_create_with_resource(RESOURCE_ID_ERROR_A);
		#endif
		bitmap_layer_set_bitmap(image_layer, image);
        text_layer_set_text(error_text_layer, error_tuple->value->cstring);
    }

	// Prepare the username
    Tuple *username_tuple = dict_find(iter, KEY_USERNAME);
    if(username_tuple){
        strcpy(usr_str, username_tuple->value->cstring);
		prepend(usr_str, " ");
        //text_layer_set_text(username_text_layer, usr_str);
    }

	// Set the likes
	Tuple *likes_tuple = dict_find(iter, KEY_LIKES);
    if(likes_tuple){
        strcpy(likes_str, likes_tuple->value->cstring);
		update_likes(likes_str);
    }

	// Set the comments
	Tuple *comments_tuple = dict_find(iter, KEY_COMMENTS);
    if(comments_tuple){
        strcpy(comments_str, comments_tuple->value->cstring);
		update_comments(comments_str);
    }

	// Set the caption
	Tuple *caption_tuple = dict_find(iter, KEY_CAPTION);
    if(caption_tuple){
        strcpy(capt_str, caption_tuple->value->cstring);
		update_caption(capt_str);
    }

}

static void load_next(){

    Loading = 1;

    // Clear the image
	if(image){
		gbitmap_destroy(image);
		image = NULL;
		#ifdef PBL_COLOR
			image = gbitmap_create_with_resource(RESOURCE_ID_LOADING_B);
		#else
			image = gbitmap_create_with_resource(RESOURCE_ID_LOADING_A);
		#endif
	}
	bitmap_layer_set_bitmap(image_layer, image);

	// Set loading text
	text_layer_set_text(message_text_layer, "DEVELOPING...");
    text_layer_set_text(username_text_layer, "");
    text_layer_set_background_color(username_text_layer, GColorClear);
};

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {

    if(Loading == 1){
	    text_layer_set_text(message_text_layer, "PATIENCE!");
    }

	if(ErrorExists == 0 && Loading == 0){

		show_details();

	    DictionaryIterator *iter;
	    uint8_t value = 1;
	    app_message_outbox_begin(&iter);
	    dict_write_int(iter, KEY_DETAILS, &value, 1, true);
	    dict_write_end(iter);
	    app_message_outbox_send();
	}
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {

    if(Loading == 1){
	    text_layer_set_text(message_text_layer, "PATIENCE!");
    }

	if(ErrorExists == 0 && Loading == 0){

		load_next();

	    DictionaryIterator *iter;
	    uint8_t value = 1;
	    app_message_outbox_begin(&iter);
	    dict_write_int(iter, KEY_NEXT, &value, 1, true);
	    dict_write_end(iter);
	    app_message_outbox_send();
	}
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {

    if(Loading == 1){
	    text_layer_set_text(message_text_layer, "PATIENCE!");
    }

	if(ErrorExists == 0 && Loading == 0){

		load_next();

	    DictionaryIterator *iter;
	    uint8_t value = 1;
	    app_message_outbox_begin(&iter);
	    dict_write_int(iter, KEY_PREVIOUS, &value, 1, true);
	    dict_write_end(iter);
	    app_message_outbox_send();
	}
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}

static void app_message_init() {
    // Register message handlers
    app_message_register_inbox_received(cb_in_received_handler);
    // Init buffers
    app_message_open(app_message_inbox_size_maximum(), APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

static void window_load(Window *window) {

    Loading = 1;

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    image_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_alignment(image_layer, GAlignTop);
	#ifdef PBL_COLOR
	    bitmap_layer_set_background_color(image_layer, TRMainColor);
    #else
    	bitmap_layer_set_background_color(image_layer, GColorBlack);
    #endif

	// display initial app graphic
	#ifdef PBL_COLOR
		image = gbitmap_create_with_resource(RESOURCE_ID_INIT_B);
	#else
		image = gbitmap_create_with_resource(RESOURCE_ID_INIT_A);
	#endif
	bitmap_layer_set_bitmap(image_layer, image);
	layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));

    message_text_layer = text_layer_create(GRect(0, bounds.size.h - 160, bounds.size.w, bounds.size.h));
    text_layer_set_text(message_text_layer, "Spooling...");
    text_layer_set_font(message_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(message_text_layer, GTextAlignmentCenter);
	text_layer_set_overflow_mode(message_text_layer, GTextOverflowModeWordWrap);
    #ifdef PBL_COLOR
        text_layer_set_text_color(message_text_layer, GColorBlack);
    #else
        text_layer_set_text_color(message_text_layer, GColorWhite);
    #endif
    text_layer_set_background_color(message_text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(message_text_layer));

    error_text_layer = text_layer_create(GRect(0, bounds.size.h - 148, bounds.size.w, 80));
    text_layer_set_text(error_text_layer, "");
    text_layer_set_font(error_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(error_text_layer, GTextAlignmentCenter);
    text_layer_set_text_color(error_text_layer, GColorWhite);
    text_layer_set_background_color(error_text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(error_text_layer));

    username_text_layer = text_layer_create(GRect(0, bounds.size.h - 25, bounds.size.w, 25));
    text_layer_set_text(username_text_layer, "");
    text_layer_set_font(username_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(username_text_layer, GTextAlignmentLeft);
	text_layer_set_background_color(username_text_layer, GColorClear);
	#ifdef PBL_COLOR
	    text_layer_set_text_color(username_text_layer, TRAltColor);
    #else
    	text_layer_set_text_color(username_text_layer, GColorBlack);
    #endif
    layer_add_child(window_layer, text_layer_get_layer(username_text_layer));
}

static void window_unload(Window *window) {
    //text_layer_destroy(message_text_layer);
    text_layer_destroy(error_text_layer);

    text_layer_destroy(username_text_layer);
    bitmap_layer_destroy(image_layer);

    if(image){
        gbitmap_destroy(image);
    }
    if(data_image){
        free(data_image);
    }
}

static void init(void) {
    app_message_init();
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    #ifdef PBL_SDK_2
    window_set_fullscreen(window, true);
    #endif
    window_stack_push(window, true);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
	init();
	details_init();
    app_event_loop();
	details_deinit();
	deinit();
}
