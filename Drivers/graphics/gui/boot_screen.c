/*
 * boot_screen.c
 *
 *  Created on: Jan 12, 2021
 *      Author: David		Original work by Jose (PTDreamer), 2017
 */

#include "boot_screen.h"
#include "oled.h"
#include "gui.h"
#define SPLASH_TIMEOUT 1000


//-------------------------------------------------------------------------------------------------------------------------------
// Boot Screen variables
//-------------------------------------------------------------------------------------------------------------------------------
int32_t profile;
uint8_t status;
uint32_t splash_time;
//-------------------------------------------------------------------------------------------------------------------------------
// Boot Screen widgets
//-------------------------------------------------------------------------------------------------------------------------------
screen_t Screen_boot;

static widget_t Widget_profile_edit;
static editable_widget_t editable_Profile_edit;

static widget_t Widget_profile_OK;
static button_widget_t button_Profile_OK;

static uint8_t boot_step=0;
//-------------------------------------------------------------------------------------------------------------------------------
// Boot Screen widget functions
//-------------------------------------------------------------------------------------------------------------------------------
static void * getProfile() {
	return &profile;
}

static void setProfile(int32_t *val) {
	profile = *val;
}
static int profile_OK(widget_t *w) {
	loadProfile((uint8_t)profile);									// Load profile
	saveSettings(saveKeepingProfiles);								// Save
	systemSettings.setupMode=setup_Off;								// Reset setup mode
	SetFailState(noError);											// Enable normal operation
	return screen_main;
}


// Credits: Jesus Vallejo  https://github.com/jesusvallejo/
const uint8_t splashXBM[] = {
	128, 64,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x54, 0x55, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0x0F, 0x00, 0x00, 0xF2, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xFF,
	0x00, 0xC0, 0xF3, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xFE, 0xFF, 0x0F, 0x00, 0xF0, 0xF3, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xC0, 0x83, 0x20, 0x39, 0x00, 0xFC, 0xF3, 0xFF,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x3B, 0x09,
	0x00, 0xCC, 0xF3, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xF8, 0x83, 0xBB, 0xFA, 0x00, 0x87, 0xF3, 0xFF, 0x0F, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xBE, 0xBB, 0x0B, 0x80, 0x03, 0x00, 0xF8,
	0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x83, 0xBB, 0x1B,
	0xC0, 0x07, 0x00, 0xF0, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xFE, 0xFF, 0x0F, 0xC0, 0xCF, 0xFF, 0xE3, 0x3F, 0xF0, 0x33, 0xF3,
	0x03, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0x3F, 0xE0, 0xFF, 0xFF, 0x07,
	0x00, 0xF0, 0x33, 0xF3, 0x03, 0x00, 0x00, 0x00, 0x00, 0x7E, 0xCC, 0x0F,
	0xF0, 0xFF, 0x07, 0x0C, 0x00, 0xC0, 0x30, 0x33, 0x00, 0x00, 0x00, 0x00,
	0xE0, 0xFF, 0xBB, 0xFF, 0xF0, 0xFF, 0x00, 0xF8, 0xFF, 0xC0, 0xF0, 0xF3,
	0x01, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xDC, 0x0F, 0xF0, 0x7F, 0x00, 0xE0,
	0xF9, 0xC0, 0xF0, 0xF3, 0x01, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xEB, 0xFF,
	0xF8, 0x3F, 0x00, 0xC0, 0xF0, 0xC1, 0x30, 0x33, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x7E, 0x8C, 0x0F, 0xF8, 0x3F, 0x00, 0x40, 0xE0, 0xC1, 0x30, 0xF3,
	0x03, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0x7F, 0x00, 0x1E, 0x60, 0x80,
	0xF0, 0xC1, 0x30, 0xF3, 0x03, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0x0F,
	0x00, 0x1E, 0xF8, 0x81, 0xF9, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x54, 0x55, 0x05, 0x7C, 0x0E, 0xF8, 0x01, 0xF9, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x54, 0x01, 0x7C, 0x0E, 0xFC, 0x03,
	0xF9, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01,
	0x7C, 0x0E, 0xFC, 0x03, 0xF9, 0xF3, 0xF3, 0x33, 0xF0, 0xF3, 0xF3, 0xF3,
	0x33, 0xF3, 0x43, 0x00, 0x7C, 0x0E, 0xF8, 0x01, 0xF9, 0xF3, 0xF3, 0x33,
	0x70, 0xF2, 0xF3, 0xF3, 0x73, 0xF3, 0x43, 0x00, 0x7C, 0x1E, 0xF8, 0x81,
	0x19, 0x33, 0x30, 0x33, 0x70, 0x32, 0x30, 0xC2, 0xF0, 0x33, 0x00, 0x00,
	0x7C, 0x1E, 0x60, 0x80, 0x09, 0xF0, 0x33, 0x33, 0x70, 0xF2, 0xF1, 0xC3,
	0xF0, 0xB3, 0x03, 0x00, 0x7C, 0x1E, 0x00, 0x80, 0x09, 0xF0, 0x33, 0x33,
	0x70, 0xF2, 0xB1, 0xC0, 0xB0, 0xB3, 0x03, 0x00, 0x7C, 0x1E, 0x60, 0x80,
	0x19, 0x03, 0x33, 0x33, 0x70, 0x32, 0xB0, 0xC1, 0x30, 0x33, 0x03, 0x00,
	0x3C, 0x3C, 0x60, 0xC0, 0xF9, 0xF3, 0xF3, 0xF3, 0x73, 0xF2, 0x33, 0xF3,
	0x33, 0xF3, 0x03, 0x00, 0x18, 0x78, 0x60, 0x00, 0xF8, 0xF1, 0xF3, 0xF3,
	0xF3, 0xF3, 0x33, 0xF3, 0x33, 0xF3, 0x03, 0x00, 0x18, 0xF8, 0x60, 0x00,
	0xF8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x30, 0xFC, 0x60, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x70, 0xFE, 0x60, 0xF8, 0xFF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x7F, 0x60, 0xF0,
	0x7F, 0xF0, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0x33, 0x03, 0x00, 0x00, 0x00,
	0xC0, 0x3F, 0x60, 0xE0, 0x3F, 0xF0, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0x73,
	0x03, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x60, 0xC0, 0x1F, 0x30, 0xC0, 0x30,
	0xC3, 0xC0, 0x30, 0xF3, 0x03, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x60, 0xC0,
	0x0F, 0xF0, 0xC3, 0xF0, 0xC3, 0xC0, 0x30, 0xB3, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x1E, 0x60, 0x80, 0x07, 0xF0, 0xC3, 0xF0, 0xC3, 0xC0, 0x30, 0x33,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x60, 0x00, 0x03, 0x00, 0xC3, 0x30,
	0xC3, 0xC0, 0x30, 0x33, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
	0x00, 0xF0, 0xC3, 0x30, 0xC3, 0xF0, 0xF3, 0x33, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0xF0, 0xC3, 0x30, 0xC3, 0xF0, 0xF3, 0x33,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01,
	0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0x1D, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x80, 0x03, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1D, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xEF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xDD, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E,
	0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDD, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1D, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0xFF,
	0xFF, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, };


void boot_screen_draw(screen_t *scr){
	if(boot_step==1){
		boot_step=2;
		default_screenDraw(scr);

		u8g2_SetFont(&u8g2,default_font );
		u8g2_SetDrawColor(&u8g2, WHITE);
		putStrAligned("First boot!", 0, align_center);
		putStrAligned("Select profile", 16, align_center);
		u8g2_DrawHLine(&u8g2, 0, 32, OledWidth);
		return;
	}
	default_screenDraw(scr);
}
int boot_screen_processInput(screen_t * scr, RE_Rotation_t input, RE_State_t *state) {

	if(HAL_GetTick() - splash_time > SPLASH_TIMEOUT){		// After splash timeout

		if(!systemSettings.setupMode){
			return screen_main;
		}
		else if(boot_step==0){
			boot_step=1;
			widgetEnable(&Widget_profile_edit);
			widgetEnable(&Widget_profile_OK);
			scr->refresh = screen_eraseAndRefresh;
		}
	}
	else{
		return -1;
	}
	return default_screenProcessInput(scr, input, state);
}


void boot_screen_init(screen_t * scr){
	profile=systemSettings.settings.currentProfile;

	if( (systemSettings.settings.NotInitialized!=initialized) || (systemSettings.settings.currentProfile>profile_C210) ){

		profile=profile_C210;							// For safety, set C210 profile by default, has the lowest Output TC
		systemSettings.setupMode=setup_On;				// (Failure state is set in the iron routine when unknown iron profile is detected)
	}
	default_init(scr);

	splash_time = HAL_GetTick();
	u8g2_SetDrawColor(&u8g2,WHITE);
	u8g2_DrawXBMP(&u8g2, 0, 0, splashXBM[0], splashXBM[1], &splashXBM[2]);
}

void boot_screen_setup(screen_t *scr) {
	widget_t* w;
	displayOnly_widget_t* dis;
  editable_widget_t* edit;
	screen_setDefaults(scr);
	scr->draw = &boot_screen_draw;
	scr->processInput = &boot_screen_processInput;
	scr->init = &boot_screen_init;

	// Profile select
	//
	w = &Widget_profile_edit;
	screen_addWidget(w,scr);
	widgetDefaultsInit(w, widget_multi_option, &editable_Profile_edit);
	dis=extractDisplayPartFromWidget(w);
	edit=extractEditablePartFromWidget(w);
	dis->reservedChars=4;
	w->posX = 12;
	w->posY = 40;
	w->width = 48;
	dis->getData = &getProfile;
	edit->big_step = 1;
	edit->step = 1;
	edit->selectable.tab = 0;
	edit->setData = (void (*)(void *))&setProfile;
	edit->max_value = ProfileSize-1;
	edit->options = profileStr;
	edit->numberOfOptions = 3;
	w->enabled=0;

	// OK Button
	//
	w = &Widget_profile_OK;
	screen_addWidget(w,scr);
	widgetDefaultsInit(w, widget_button, &button_Profile_OK);
	button_Profile_OK.displayString="OK";
	button_Profile_OK.selectable.tab = 1;
	button_Profile_OK.action = &profile_OK;
	w->posX = 95;
	w->posY = 40;
	w->width = 32;
	w->enabled=0;

}
