#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define DEBUG 0

#define MY_UUID { 0x42, 0x35, 0x46, 0xE7, 0x54, 0x18, 0x4F, 0x47, 0x96, 0x63, 0xF0, 0xDB, 0x98, 0x7C, 0x04, 0x40 }
PBL_APP_INFO(MY_UUID,
             "Filmplakat", "lastfuture",
             1, 3, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
#if DEBUG
             APP_INFO_STANDARD_APP
#else
			 APP_INFO_WATCH_FACE
#endif
);

void itoa(int num, char* buffer) {
    const char digits[11] = "0123456789";
    if(num > 99) {
        buffer[0] = '9';
        buffer[1] = '9';
        return;
    } else if(num > 9) {
        buffer[0] = digits[num / 10];
    } else {
        buffer[0] = '0';
    }
    buffer[1] = digits[num % 10];
}

Window window;
TextLayer row_1, row_2, row_3, row_4, row_5;
PropertyAnimation anim_1, anim_2, anim_3, anim_4, anim_5;
static char row_1_buffer[20], row_3_buffer[20], row_4_buffer[20], row_3_oldbuf[20], row_4_oldbuf[20], row_5_buffer[20];
static int row_1_x, row_2_x, row_3_x, row_4_x, row_5_x, row_1_y, row_2_y, row_3_y, row_4_y, row_5_y;
static bool row_3_asc, row_4_asc, has_row_3, has_row_4, firstblood;
GFont fontHour, fontUhr, fontMinutes, fontDate;

PblTm t;


/////////////////////////////////////////////////////////////////////////

#include "string.h"
static const char* MONTHS[] = {
    "Januar",
    "Februar",
    "März",
    "April",
    "Mai",
    "Juni",
    "Juli",
    "August",
    "September",
    "Oktober",
    "November",
    "Dezember",
};
static const char* WEEKDAYS[] = {
    "So",
    "Mo",
    "Di",
    "Mi",
    "Do",
    "Fr",
    "Sa",
};
static const char* TEENS[] = {
    "null",
    "ein",
    "zwei",
    "drei",
    "vier",
    "fünf",
    "sechs",
    "sieben",
    "acht",
    "neun",
    "zehn",
    "elf",
    "zwölf",
    "dreizehn",
    "vierzehn",
    "fünfzehn",
    "sechzehn",
    "siebzehn",
    "achtzehn",
    "neunzehn",
};
static const char* TENS[] = {
    "zwanzig",
    "dreissig",
    "vierzig",
    "fünfzig",
    "sechzig",
};
static const bool TEENS_ASC[] = {
    true,
    true,
    false,
    true,
    true,
    true,
    false,
    true,
    true,
    false,
    true,
    true,
    false,
    true,
    true,
    true,
    false,
    true,
    true,
    false,
};
static const bool TENS_ASC[] = {
    false,
    true,
    false,
    true,
    true,
};

static const char* STR_UND = "und";
static const char* STR_SPACE = " ";
static const char* STR_DOT = ".";
static const char* STR_S = "s";

static const char* ROW_2_BUFFER = "uhr";

static const int UHR_ASC = 25;
static const int MINUTES_ASC = 23;
static const int MINUTES2_ASC = 25;
static const int DATE_ASC = 34;

static const int MINUTES_X = 3;
static const int MINUTES2_X = 3;

void setup_text_layer(TextLayer* row, PropertyAnimation *this_anim, int x, int y, int oldx, int oldy, GFont font, int magic){
    text_layer_set_text_color(row, GColorWhite);
    text_layer_set_background_color(row, GColorClear);
    layer_add_child(&window.layer, &row->layer);
    text_layer_set_font(row,font);
    
    int speed = 1000;
    int distance = oldy - y;
    
    if (distance < 0) { distance *= -1; }
    
    if (firstblood) {
        speed = 600;
    } else {
        speed = distance * 90;
    }
    
    GRect start_rect = GRect(oldx,oldy,144-oldx-1,50);
    GRect target_rect = GRect(x,y,144-x-1,50);
    
    if (magic == 1) { // disappear
        start_rect = GRect(oldx,oldy,144-oldx-1,50);
        target_rect = GRect(oldx,oldy,144-x-1,0);
        speed = 300;
    } else if (magic == 2) { // reappear
        start_rect = GRect(x,y,144-oldx-1,0);
        target_rect = GRect(x,y,144-x-1,50);
    } else if (magic == 3) { // and stay down
        start_rect = GRect(oldx,oldy,144-oldx-1,0);
        target_rect = GRect(x,y,144-x-1,0);
        speed = 100;
    } else {
    }
    
    layer_set_frame(&row->layer, start_rect);
    property_animation_init_layer_frame(this_anim, &row->layer, NULL, &target_rect);
    
    animation_set_duration(&this_anim->animation, speed);  
    animation_set_curve(&this_anim->animation, AnimationCurveEaseInOut);
    
    animation_schedule(&this_anim->animation);
}

void GetTime(int hours, int minutes, int day, int month, int weekday){
    
    has_row_3 = false;
    has_row_4 = false;
    
    int hours12h = hours % 12;
    if (hours == 12) { hours12h = 12; }
    strcat(row_1_buffer, TEENS[hours12h]);
    
    if (minutes == 0) {
    } else if (minutes < 20) {
        strcat(row_3_buffer, TEENS[minutes]);
        if (minutes == 1) {
            strcat(row_3_buffer, STR_S);
        }
        has_row_3 = true;
        row_3_asc = TEENS_ASC[minutes];
    } else {
        int tenner = minutes/10;
        int oner = minutes % 10;
        if (oner == 0) {
            strcat(row_3_buffer, TENS[tenner-2]);
            row_3_asc = TENS_ASC[tenner-2];
            has_row_3 = true;
        } else {
            strcat(row_3_buffer, TEENS[oner]);
            strcat(row_3_buffer, STR_UND);
            row_3_asc = TEENS_ASC[oner];
            has_row_3 = true;
            strcat(row_4_buffer, TENS[tenner-2]);
            row_4_asc = TENS_ASC[tenner-2];
            has_row_4 = true;
        }
    }
    
    char daynum[] = "xx";
    itoa(day,daynum);
    strcat(row_5_buffer, WEEKDAYS[weekday]);
    strcat(row_5_buffer, STR_SPACE);
    strcat(row_5_buffer, daynum);
    strcat(row_5_buffer, STR_DOT);
    strcat(row_5_buffer, STR_SPACE);
    strcat(row_5_buffer, MONTHS[month]);
}

void update_time(PblTm* t){

    memset(row_3_oldbuf,0,20);
    memset(row_4_oldbuf,0,20);

    strcat(row_3_oldbuf,row_3_buffer);
    strcat(row_4_oldbuf,row_4_buffer);
    
    memset(row_1_buffer,0,20);
    memset(row_3_buffer,0,20);
    memset(row_4_buffer,0,20);
    memset(row_5_buffer,0,20);
    
    bool has_row_3_old = has_row_3;
    bool has_row_4_old = has_row_4;
        
    strcat(row_1_buffer, STR_SPACE); // workaround for weird "z" bug
    
    GetTime(t->tm_hour,t->tm_min,t->tm_mday,t->tm_mon,t->tm_wday);
    //GetTime(t->tm_hour,t->tm_sec,t->tm_mday,t->tm_mon,t->tm_wday);
        
    int spacing = 0;
    
    int row_1_oldx = row_1_x;
    int row_2_oldx = row_2_x;
    int row_3_oldx = row_3_x;
    int row_4_oldx = row_4_x;
    int row_5_oldx = row_5_x;
    
    int row_1_oldy = row_1_y;
    int row_2_oldy = row_2_y;
    int row_3_oldy = row_3_y;
    int row_4_oldy = row_4_y;
    int row_5_oldy = row_5_y;
    
    row_1_x = row_2_x = row_3_x = row_4_x = row_5_x = 26;
    
    row_1_y = spacing;
    spacing += UHR_ASC;
    row_2_y = spacing;
    if (has_row_3) {
        if (row_3_asc) { spacing += MINUTES_ASC; } else { spacing += MINUTES_ASC-MINUTES_X; }
    }
    row_3_y = spacing;
    if (has_row_4) {
        if (row_4_asc) { spacing += MINUTES2_ASC; } else { spacing += MINUTES2_ASC-MINUTES2_X; }
    }
    row_4_y = spacing;
    spacing += DATE_ASC;
    row_5_y = spacing;
    
    row_1_x -= row_1_y/5;
    row_2_x -= row_2_y/5;
    row_3_x -= row_3_y/5;
    row_4_x -= row_4_y/5;
    row_5_x -= row_5_y/5;
    row_1_x -= 7;
    row_5_x += 4;
    
    spacing += 22;
    
    int y_offset = (168-spacing)/2;
    
    row_1_y += y_offset;
    row_2_y += y_offset;
    row_3_y += y_offset;
    row_4_y += y_offset;
    row_5_y += y_offset;
    
    if (firstblood) {
        has_row_3_old = has_row_3;
        has_row_4_old = has_row_4;
        memset(row_3_oldbuf,0,20);
        memset(row_4_oldbuf,0,20);    
        strcat(row_3_oldbuf,row_3_buffer);
        strcat(row_4_oldbuf,row_4_buffer);
        
        int sidestoggle = -144;
        
        row_1_oldx = sidestoggle;
        sidestoggle *= -1;
        row_2_oldx = sidestoggle;
        if (has_row_3) { sidestoggle *= -1; };
        row_3_oldx = sidestoggle;
        if (has_row_4) { sidestoggle *= -1; };
        row_4_oldx = sidestoggle;
        sidestoggle *= -1;
        row_5_oldx = sidestoggle;
    
        row_1_oldy = row_1_y;
        row_2_oldy = row_2_y;
        row_3_oldy = row_3_y;
        row_4_oldy = row_4_y;
        row_5_oldy = row_5_y;
    }
    
    
    setup_text_layer(&row_1,&anim_1,row_1_x,row_1_y,row_1_oldx,row_1_oldy,fontHour,0);
    text_layer_set_text(&row_1,row_1_buffer);
    
    setup_text_layer(&row_2,&anim_2,row_2_x,row_2_y,row_2_oldx,row_2_oldy,fontUhr,0);
    text_layer_set_text(&row_2,ROW_2_BUFFER);
    
    int magic = 0;
    
    if (has_row_3 == has_row_3_old && has_row_3 == true) {
        magic = 0;
    } else if (has_row_3 == has_row_3_old && has_row_3 == false) {
        magic = 3; // stay down
    } else if (has_row_3 != has_row_3_old && has_row_3 == false) {
        magic = 1; // disappear
    } else if (has_row_3 != has_row_3_old && has_row_3 == true) {
        magic = 2; // reappear
    }
    setup_text_layer(&row_3,&anim_3,row_3_x,row_3_y,row_3_oldx,row_3_oldy,fontMinutes,magic);
    if (magic == 1) {
        text_layer_set_text(&row_3,row_3_oldbuf);
    } else {
        text_layer_set_text(&row_3,row_3_buffer);
    }
        
    if (has_row_4 == has_row_4_old && has_row_4 == true) {
        magic = 0;
    } else if (has_row_4 == has_row_4_old && has_row_4 == false) {
        magic = 3; // stay down
    } else if (has_row_4 != has_row_4_old && has_row_4 == false) {
        magic = 1; // disappear
    } else if (has_row_4 != has_row_4_old && has_row_4 == true) {
        magic = 2; // reappear
    }
    setup_text_layer(&row_4,&anim_4,row_4_x,row_4_y,row_4_oldx,row_4_oldy,fontMinutes,magic);
    if (magic == 1) {
        text_layer_set_text(&row_4,row_4_oldbuf);
    } else {
        text_layer_set_text(&row_4,row_4_buffer);
    }
    
    setup_text_layer(&row_5,&anim_5,row_5_x,row_5_y,row_5_oldx,row_5_oldy,fontDate,0);
    text_layer_set_text(&row_5,row_5_buffer);
    
    if (firstblood) {
        firstblood = false;
    }
}


static void handle_minute_tick(AppContextRef app_ctx, PebbleTickEvent* e) {
    update_time(e->tick_time);
}

void handle_init(AppContextRef ctx) {
    (void)ctx;
    
    firstblood = true;
    
    window_init(&window, "Clock_Window");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);
    resource_init_current_app(&APP_RESOURCES);
    
    fontHour = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLDITALIC_30));
    fontUhr = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_LIGHTITALIC_28));
    fontMinutes = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_ITALIC_30));
    fontDate = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_ITALIC_12));
    
    memset(row_3_buffer,0,20);
    memset(row_4_buffer,0,20);
    
    row_1_x = row_2_x = row_3_x = row_4_x = row_5_x = 13;
    row_1_y = row_2_y = row_3_y = row_4_y = row_5_y = 168/2;
    row_1_y -= 30;
    row_2_y -= 10;
    row_3_y += 10;
    row_4_y += 30;
    row_5_y += 50;
    
    text_layer_init(&row_5, window.layer.frame);
    text_layer_init(&row_4, window.layer.frame);
    text_layer_init(&row_3, window.layer.frame);
    text_layer_init(&row_2, window.layer.frame);
    text_layer_init(&row_1, window.layer.frame);
    
    get_time(&t);
}


void pbl_main(void *params) {
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .tick_info = {
            .tick_handler = &handle_minute_tick,
            .tick_units = MINUTE_UNIT
        }
    };
    app_event_loop(params, &handlers);
}