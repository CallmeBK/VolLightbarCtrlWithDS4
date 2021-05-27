#include <string.h>
#include <taihen.h>
#include <stdio.h>
#include <psp2kern/kernel/types.h>
#include <psp2kern/ctrl.h>
#include <psp2kern/sblaimgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/sysclib.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/registrymgr.h>
#include <psp2kern/display.h>
#include <psp2kern/kernel/iofilemgr.h>


#define R_SUCCEEDED(res)   ((res)>=0)
                      /*    BBGGRR <- B=BLUE, G=GREEN, R=RED*/
#define CYAN            0x00FFFF00
#define MAGENTA         0x00FF00FF
#define YELLOW          0x0000FFFF
#define RED             0x000000FF 
#define WHITE           0x00FFFFFF
#define BLANK           0x00000000
#define LIMEGREEN       0x8000FF00
#define GREEN           0x0000FF00
#define SALMON          0xC00000FF
#define SKYBLUE         0x00EE9500
#define BLACK           0xFF000000
#define GRAY            0x00BEBEBE

#define APP_SYSTEM      0
#define APP_SHELL       1
#define APP_GAME        2
#define APP_PSPEMU      3

static SceUID tai_uid;
static SceUID tai_uid1;
static tai_hook_ref_t hook;
static tai_hook_ref_t hook1;
static int batteryinfo;
static int lightbarinfo;
int volbeforemute = 100;
SceUInt64 tick_prev_change;
int recentmute = 0;
uint32_t* fb_base;
uint32_t fb_width;
uint32_t fb_height;
uint32_t fb_pitch;
int X1_background;
int X2_background;
int Y1_background;
int Y2_background;
int X1_bar;
int X2_bar;
int Y1_bar;
int Y2_bar;
int Indicator_width;
int X1_indicator;
int X2_indicator;
int Y1_indicator;
int Y2_indicator;
static uint32_t p1multiplier;
static uint32_t p2multiplier;
static uint32_t p3multiplier;
static uint32_t p4multiplier;
int toggle_port1 = 0;
int toggle_port2 = 0;
int toggle_port3 = 0;
int toggle_port4 = 0;
char buffer[3];

SceBool ksceAppMgrIsExclusiveProcessRunning();  /*delcare function for determining if current process is exclusive*/
int ksceSblACMgrIsShell(SceUID pid);  /*delcare function for determining if current process is shell (main)*/
int ksceSblACMgrIsPspEmu(SceUID pid); /*delcare function for determining if current process is PSP Emulation (Adrenaline)*/

/*declare function pointers to SceCtrl userland functions*/
int (*sceCtrlGetBatteryInfo)(int port, SceUInt8 *batt) = NULL;
int (*sceCtrlSetLightBar)(int port, SceUInt8 r, SceUInt8 g, SceUInt8 b) = NULL;

/*declare function pointer to SceAVConfig global variable which controls master volume*/
int *mastervol = NULL;
/*declare function pointer to SceAVConfig global variable which holds master volume change event id*/
int *mastervolchange = NULL;

/*module_get_offset declaration required because not included in any headers. Part of taihenModuleUtils_stub.*/
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

/*Define function for drawing an on-screen volume indicator for PSTV*/
int showvolumebar(const SceDisplayFrameBuf *framebuffer){
	
	fb_base = framebuffer->base;
	fb_width = framebuffer->width;
	fb_height = framebuffer->height;
	fb_pitch = framebuffer->pitch;
	
	uint32_t color1 = SKYBLUE;
	uint32_t color2 = WHITE;
	uint32_t color3 = GRAY;
	
	/*Calculate dimensions of volume bar.  Dimensions are relative to size of framebuffer to account for all possible resolutions*/
	X1_background = fb_width / 10;
	X2_background = fb_width - X1_background;
	Y1_background = 84 * fb_height / 100;
	Y2_background = 99 * fb_height / 100;
	X1_bar = X1_background + ((X2_background - X1_background) / 10);
	X2_bar = X2_background - ((X2_background - X1_background) / 10); 
	Y1_bar = Y1_background + ((Y2_background - Y1_background) / 3);
	Y2_bar = Y2_background - ((Y2_background - Y1_background) / 3);
	Indicator_width = 2 * (X2_bar - X1_bar) / 100;
	Y1_indicator = Y1_background + (2 * (Y2_background - Y1_background) / 15);
	Y2_indicator = Y2_background - (2 * (Y2_background - Y1_background) / 15);
	
	/*Calculate X1_indicator and X2_indicator based on *mastervol*/
	X1_indicator = X1_bar + ((X2_bar - X1_bar) / 30 * (*mastervol));
	X2_indicator = X1_indicator + Indicator_width;
	
	/*define arrays to store color value for multiple pixels.  This allows for the copying of a color to a row of pixels all at once and drastically improves performance over copying a color to one pixel at a time.*/
	uint32_t line[Indicator_width];
	uint32_t line1[X1_indicator - X1_bar];
	uint32_t line2[X2_bar - X2_indicator];
	
	/*Fill first array with WHITE color*/
	for (int i = 0; i < Indicator_width; i++){
		line[i] = color2;
	}
	
	/*Fill second array with SKYBLUE color*/
	for (int j = 0; j < X1_indicator - X1_bar; j++){
		line1[j] = color1;
	}
	
	/*Fill third array with GRAY color*/
	for (int k = 0; k < X2_bar - X2_indicator; k++){
		line2[k] = color3;
	}
	
	/*Draw volume bar one row at a time*/
	for (int y = Y1_indicator; y < Y2_indicator; y++){
		if (y < Y1_bar || y > Y2_bar) {
			ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + X1_indicator], &line[0], sizeof(line));
		}
		else {
			ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + X1_bar], &line1[0], sizeof(line1));
			ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + X1_indicator], &line[0], sizeof(line));
			ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + X2_indicator], &line2[0], sizeof(line2));
		}
	}
	
	/*Draw volume bar one pixel at a time.  Leaving this section as a comment for future reference.
	for (int y = Y1_indicator; y < Y2_indicator; y++){
		for (int x = X1_bar; x < X2_bar; x++){
			if (x >= X1_bar && x < X2_bar && y >= Y1_bar && y < Y2_bar){
				if (x >= X1_indicator && x < X2_indicator && y >= Y1_indicator && y < Y2_indicator){
					draw white pixel
					ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + x], &color2, sizeof(color2));
				}
				else if (x >= X2_indicator && x < X2_bar && y >= Y1_bar && y < Y2_bar){
					draw gray pixel
					ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + x], &color3, sizeof(color3));
				}
				else {
					draw green pixel
					ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + x], &color1, sizeof(color1));
				}
				don't draw black pixel over green pixels
			}
			else if (x >= X1_indicator && x < X2_indicator && y >= Y1_indicator && y < Y2_indicator){
				draw white pixel
				ksceKernelMemcpyKernelToUser((uintptr_t)&fb_base[y * fb_pitch + x], &color2, sizeof(color2));
			}
		}
	}*/
	return 0;
}

/*Reverse Engineered kernel equivalent of sceAVConfigSetMasterVol*/
void setMasterVolume(int volume){
	*mastervol = volume;
	ksceKernelSetEventFlag(*mastervolchange,0x20);
	ksceRegMgrSetKeyInt("/CONFIG/SOUND","master_volume",*mastervol);
}

int ksceDisplaySetFrameBufInternal_patched(int head, int index, const SceDisplayFrameBuf *pParam, int sync) {
	
	SceUID pid;
	char title_id[32] = "";
	int processtype;
	
	/*Determine process type*/
	pid = ksceKernelGetProcessId();
	ksceKernelGetProcessTitleId(pid, title_id, sizeof(title_id));
	
	if (ksceSblACMgrIsPspEmu(pid)){
		processtype = APP_PSPEMU;
	}
	else if (strncmp(title_id, "NPXS", strlen("NPXS")) == 0){
		processtype = APP_SYSTEM;
	}
	else if (ksceSblACMgrIsShell(pid)){
		processtype = APP_SHELL;
	}
	else {
		processtype = APP_GAME;
	}
	
	
	
	if (head != ksceDisplayGetPrimaryHead() || !pParam || !pParam->base){             /*Reduces screen lag quite a bit.  Copied from MERLev's reVita plugin.*/
		goto DISPLAY_HOOK_RET;
	}
	
	if (!index && processtype == APP_SHELL){                                          /*Do not draw on i0 in SceShell.  Copied from MERLev's reVita plugin.*/
		goto DISPLAY_HOOK_RET;
	}
	
	if (index && (ksceAppMgrIsExclusiveProcessRunning() || processtype == APP_GAME)){ /*Do not draw over SceShell overlay.  Copied from MERLev's reVita plugin.*/
		goto DISPLAY_HOOK_RET;
	}
	
	if (ksceKernelGetSystemTimeWide() - tick_prev_change < 2000000){                  /*if less than 2 seconds have gone by since last change in volume, draw volume indicator.*/
		showvolumebar(pParam);
	}
DISPLAY_HOOK_RET:
	return TAI_CONTINUE(int, hook1, head, index, pParam, sync);
}

/*sceCtrlPeekBufferPositive2 hook function*/
int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) 
{
	int ret;
	int genuineVITA;
	int max_volume = 30;
	int min_volume = 0;

	/*Should always call TAI_CONTINUE at beginning of hook function */
    ret = TAI_CONTINUE(int, hook, port, pad_data, count);
	
	/*Make sure TAI_CONTINUE return value is not out of bounds*/
    if (ret < 1 || ret > 64){
        return ret;
	}
	
	/*Check if device is VITA and not PSTV*/
	genuineVITA = ksceSblAimgrIsGenuineVITA();
	
	
	
	/*VITA behavior*/
    /*If L1 and Select are the only buttons pressed, add volume down as a pressed button
	If R1 and Select are the only buttons pressed, add volume up as a pressed button
	Separate if statements allow for both volume down and volume up to be added as pressed buttons.
	Holding volume down and volume up buttons will mute the volume.  If L1,L2,R1,R2 are all pressed
	and the lightbar brightness is at 100%, lightbar brightness will change to 5%.  And vice versa.
	This brightness toggle is controller independent, so only the controller that has L1,L2,R1,R2
	held down will have its lightbar brightness change.*/
	if (genuineVITA == 1) {
		if (pad_data->buttons == (SCE_CTRL_L1 | SCE_CTRL_R1 | SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER)){
			if (port == 1){
				if (toggle_port1 == 0){
					if (p1multiplier == 100){
						p1multiplier = 5;
						SceUID fd = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p1config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd, "5", 3);
						ksceIoClose(fd);
					}
					else if (p1multiplier == 5){
						p1multiplier = 100;
						SceUID fd = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p1config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd, "100", 3);
						ksceIoClose(fd);
					}
					toggle_port1 = 1;
				}
			}
			else if (port == 2){
				if (toggle_port2 == 0){
					if (p2multiplier == 100){
						p2multiplier = 5;
						SceUID fd1 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p2config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd1, "5", 3);
						ksceIoClose(fd1);
					}
					else if (p2multiplier == 5){
						p2multiplier = 100;
						SceUID fd1 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p2config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd1, "100", 3);
						ksceIoClose(fd1);
					}
					toggle_port2 = 1;
				}
			}
			else if (port == 3){
				if (toggle_port3 == 0){
					if (p3multiplier == 100){
						p3multiplier = 5;
						SceUID fd2 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p3config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd2, "5", 3);
						ksceIoClose(fd2);
					}
					else if (p3multiplier == 5){
						p3multiplier = 100;
						SceUID fd2 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p3config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd2, "100", 3);
						ksceIoClose(fd2);
					}
					toggle_port3 = 1;
				}
			}
			else if (port == 4){
				if (toggle_port4 == 0){
					if (p4multiplier == 100){
						p4multiplier = 5;
						SceUID fd3 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p4config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd3, "5", 3);
						ksceIoClose(fd3);
					}
					else if (p4multiplier == 5){
						p4multiplier = 100;
						SceUID fd3 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p4config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd3, "100", 3);
						ksceIoClose(fd3);
					}
					toggle_port4 = 1;
				}
			}
		}
		else if (pad_data->buttons == (SCE_CTRL_L1 | SCE_CTRL_R1 | SCE_CTRL_SELECT)){
			pad_data->buttons = (SCE_CTRL_VOLDOWN | SCE_CTRL_VOLUP);
		}
		else if (pad_data->buttons == (SCE_CTRL_L1 | SCE_CTRL_SELECT)){
			pad_data->buttons = SCE_CTRL_VOLDOWN;
		}
		else if (pad_data->buttons == (SCE_CTRL_R1 | SCE_CTRL_SELECT)){
			pad_data->buttons = SCE_CTRL_VOLUP;
		}
		else if (pad_data->buttons == 0){
			if (port == 1){
				toggle_port1 = 0;
			}
			else if (port == 2){
				toggle_port2 = 0;
			}
			else if (port == 3){
				toggle_port3 = 0;
			}
			else if (port == 4){
				toggle_port4 = 0;
			}
		}
	}
	/*PSTV behavior*/
	/*Volume up and volume down buttons do not work on PSTV.  PSTV volume is
	controlled by the master volume variable and not the system volume variable.
	The code below recreates the volume up and volume down button functionality
	using the master volume variable rather than the system volume variable.
	If L1,L2,R1,R2 are all pressed and the lightbar brightness is at 100%, lightbar
	brightness will change to 5%.  And vice versa.  This brightness toggle is 
	controller independent, so only the controller that has L1,L2,R1,R2
	held down will have its lightbar brightness change.*/*/
	else {
		if (pad_data->buttons == (SCE_CTRL_L1 | SCE_CTRL_R1 | SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER)){
			if (port == 1){
				if (toggle_port1 == 0){
					if (p1multiplier == 100){
						p1multiplier = 5;
						SceUID fd = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p1config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd, "5", 3);
						ksceIoClose(fd);
					}
					else if (p1multiplier == 5){
						p1multiplier = 100;
						SceUID fd = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p1config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd, "100", 3);
						ksceIoClose(fd);
					}
					toggle_port1 = 1;
				}
			}
			else if (port == 2){
				if (toggle_port2 == 0){
					if (p2multiplier == 100){
						p2multiplier = 5;
						SceUID fd1 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p2config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd1, "5", 3);
						ksceIoClose(fd1);
					}
					else if (p2multiplier == 5){
						p2multiplier = 100;
						SceUID fd1 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p2config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd1, "100", 3);
						ksceIoClose(fd1);
					}
					toggle_port2 = 1;
				}
			}
			else if (port == 3){
				if (toggle_port3 == 0){
					if (p3multiplier == 100){
						p3multiplier = 5;
						SceUID fd2 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p3config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd2, "5", 3);
						ksceIoClose(fd2);
					}
					else if (p3multiplier == 5){
						p3multiplier = 100;
						SceUID fd2 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p3config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd2, "100", 3);
						ksceIoClose(fd2);
					}
					toggle_port3 = 1;
				}
			}
			else if (port == 4){
				if (toggle_port4 == 0){
					if (p4multiplier == 100){
						p4multiplier = 5;
						SceUID fd3 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p4config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd3, "5", 3);
						ksceIoClose(fd3);
					}
					else if (p4multiplier == 5){
						p4multiplier = 100;
						SceUID fd3 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p4config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
						ksceIoWrite(fd3, "100", 3);
						ksceIoClose(fd3);
					}
					toggle_port4 = 1;
				}
			}
		}
		else if (pad_data->buttons == (SCE_CTRL_L1 | SCE_CTRL_R1 | SCE_CTRL_SELECT)){
			if (*mastervol != min_volume) {                                         /*If master volume level is not 0*/
				volbeforemute = *mastervol;                                         /*Store the master volume level into volbeforemute variable*/
			}
			setMasterVolume(min_volume);                                            /*Set master volume level to 0*/
			recentmute = 1;                                                         /*Let PSTV know that it was recently muted.  This prevents the PSTV from unmuting itself if you accidentally release L1 and R1 at different times*/
			tick_prev_change = ksceKernelGetSystemTimeWide();                       /*Store the precise time that master volume was changed into tick_prev_change variable*/
			pad_data->buttons = 0;                                                  /*Limits L1,R1,Select functionality to only muting volume when they are all pressed simultaneously.  Example: L1 and R1 will not cause PSTV to switch apps when you are just trying to mute the system.*/
		}
		else if (pad_data->buttons == (SCE_CTRL_L1 | SCE_CTRL_SELECT)){
			if (recentmute == 0) {                                                  /*If L1,R1,Select buttons have been released since they were last all held down together to mute the PSTV*/
				if (ksceKernelGetSystemTimeWide() - tick_prev_change > 100000) {    /*If more than 1/10 of a second has elapsed since the volume was last changed*/
					if (*mastervol != min_volume) {                                 /*If master volume level does not equal 0*/
						if (*mastervol == 1) {                                      /*If master volume level equals 1*/
							volbeforemute = 100;                                    /*This tells PSTV that the volume is 0 because it was lowered rather than muted*/
						}
						setMasterVolume(*mastervol - 1);                            /*decrease the master volume level by 1*/
						tick_prev_change = ksceKernelGetSystemTimeWide();           /*Store the precise time that master volume was changed into tick_prev_change variable*/
					}
					else if (*mastervol == min_volume) {                            /*If volume level is 0*/
						if (volbeforemute != 100) {                                 /*If volume level equals 0 because it was muted*/
							setMasterVolume(volbeforemute);                         /*Set volume level back to what it was before it was muted*/
							tick_prev_change = ksceKernelGetSystemTimeWide();       /*Store the precise time that master volume was changed into tick_prev_change variable*/
						}
						else if (volbeforemute == 100) {                            /*If volume equals 0 because it was lowered from 1*/
							                                                        /*do not decrease volume any further*/
							tick_prev_change = ksceKernelGetSystemTimeWide();       /*Storing current tick into tick_prev_change variable here will allow the volume indicator to keep displaying while Select+L1 are held*/
						}
					}
				}
			}
			pad_data->buttons = 0;                                                  /*Limits L1,Select functionality to only lowering volume when they are both pressed simultaneously.  Example: L1 will not cause PSTV to switch apps when you are just trying to lower the volume.*/
		}
		else if (pad_data->buttons == (SCE_CTRL_R1 | SCE_CTRL_SELECT)){
			if (recentmute == 0) {                                                  /*If L1,R1,Select buttons have been released since they were last all held down together to mute the PSTV*/
				if (ksceKernelGetSystemTimeWide() - tick_prev_change > 100000) {    /*If more than 1/10 of a second has elapsed since the volume was last changed*/
					if (*mastervol == max_volume) {                                 /*If master volume level is 30*/
						                                                            /*do not increase volume any further*/
						tick_prev_change = ksceKernelGetSystemTimeWide();           /*Storing current tick into tick_prev_change variable here will allow the volume indicator to keep displaying while Select+R1 are held*/
					}
					else if (*mastervol != min_volume) {                            /*If master volume level is not 0*/
						setMasterVolume(*mastervol + 1);                            /*Increase master volume level by 1*/
						tick_prev_change = ksceKernelGetSystemTimeWide();           /*Store the precise time that master volume was changed into tick_prev_change variable*/
					}
					else if (*mastervol == min_volume) {                            /*If volume level is 0*/
						if (volbeforemute != 100) {                                 /*If volume level equals 0 because it was muted*/
							setMasterVolume(volbeforemute);                         /*Set volume level back to what it was before it was muted*/
							tick_prev_change = ksceKernelGetSystemTimeWide();       /*Store the precise time that master volume was changed into tick_prev_change variable*/
						}
						else if (volbeforemute == 100) {                            /*If volume equals 0 because it was lowered from 1*/
							setMasterVolume(*mastervol + 1);                        /*Increase master volume level by 1*/
							tick_prev_change = ksceKernelGetSystemTimeWide();       /*Store the precise time that master volume was changed into tick_prev_change variable*/
						}
					}
				}
			}
			pad_data->buttons = 0;                                                  /*Limits R1,Select functionality to only raising volume when they are both pressed simultaneously.  Example: R1 will not cause PSTV to switch apps when you are just trying to raise the volume.*/
		}
		else if (pad_data->buttons == 0){
			if (port == 1){
				recentmute = 0;
				toggle_port1 = 0;
			}
			else if (port == 2){
				toggle_port2 = 0;
			}
			else if (port == 3){
				toggle_port3 = 0;
			}
			else if (port == 4){
				toggle_port4 = 0;
			}
		}
	}
    return ret;
}

/*Check config files for lightbar settings.  If none exist, create them.*/
void loadconfig(void){

	/*Make folder if it does not exist*/
	ksceIoMkdir("ux0:data/VolLightbarCtrlWithDS4", 0777);
	
	/*Attempt to open config files for each controller*/
	SceUID fd = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p1config.txt", SCE_O_RDONLY, 0777);
	SceUID fd1 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p2config.txt", SCE_O_RDONLY, 0777);
	SceUID fd2 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p3config.txt", SCE_O_RDONLY, 0777);
	SceUID fd3 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p4config.txt", SCE_O_RDONLY, 0777);
	
	if (fd >= 0) {
		
		ksceIoRead(fd, buffer, 3);
		ksceIoClose(fd);
		p1multiplier = strtol(buffer, NULL, 0);
		
	}
	else {
		
		fd = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p1config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
		ksceIoWrite(fd, "100", 3);
		ksceIoClose(fd);
		p1multiplier = 100;
		
	}
	if (fd1 >= 0) {
		
		ksceIoRead(fd1, buffer, 3);
		ksceIoClose(fd1);
		p2multiplier = strtol(buffer, NULL, 0);
		
	}
	else {
		
		fd1 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p2config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
		ksceIoWrite(fd1, "100", 3);
		ksceIoClose(fd1);
		p2multiplier = 100;
		
	}
	if (fd2 >= 0) {
		
		ksceIoRead(fd2, buffer, 3);
		ksceIoClose(fd2);
		p3multiplier = strtol(buffer, NULL, 0);
		
	}
	else {
		
		fd2 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p3config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
		ksceIoWrite(fd2, "100", 3);
		ksceIoClose(fd2);
		p3multiplier = 100;
		
	}
	if (fd3 >= 0) {
		
		ksceIoRead(fd3, buffer, 3);
		ksceIoClose(fd3);
		p4multiplier = strtol(buffer, NULL, 0);
		
	}
	else {
		
		fd3 = ksceIoOpen("ux0:/data/VolLightbarCtrlWithDS4/p4config.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
		ksceIoWrite(fd3, "100", 3);
		ksceIoClose(fd3);
		p4multiplier = 100;
		
	}
	
}

/*Thread function to check DS4 battery level and set DS4 lightbar color
to indicate the DS4 battery level.  Works with up to four DS4 controllers.
Not sure if this will be the case for everyone... but my DS4 controller dies
at a battery level of 0x2*/
int lightbar_thread(SceSize arglen, void *arg) {
    (void)arglen;
    (void)arg;
	SceUInt8 battery_level = 0x0;
	SceCtrlPortInfo portinfo;
	uint32_t lightbarbrightness;

	/*loop runs until vita shuts down*/
    for (;;) {

		/*Check to see how many DS4 controllers are connected*/
		ksceCtrlGetControllerPortInfo(&portinfo);

		/*Check each port.  If DS4 is connected to port, check DS4 battery level*/
		for (int c = 1; c < 5; c++) {
			if (portinfo.port[c] == 8) {
				sceCtrlGetBatteryInfo(c, &battery_level);
				if (c == 1) {
					lightbarbrightness = p1multiplier;
				}
				else if (c == 2) {
					lightbarbrightness = p2multiplier;
				}
				else if (c == 3) {
					lightbarbrightness = p3multiplier;
				}
				else if (c == 4) {
					lightbarbrightness = p4multiplier;
				}
				/*If battery level is 0, turn off lightbar*/
				if (battery_level == 0x0) {
					sceCtrlSetLightBar(c, 0, 0, 0);
				}
				/*If battery level is 1, turn off lightbar*/
				else if (battery_level == 0x1) {
					sceCtrlSetLightBar(c, 0, 0, 0);
				}
				/*If battery level is 2, pulse lightbar with red color*/
				else if (battery_level == 0x2) {
					for (int i = 0; i<=255; i++) {
						sceCtrlSetLightBar(c, i, 0, 0);
						ksceKernelDelayThread(3000);
					}
					for (int i = 255; i>=0; i--) {
						sceCtrlSetLightBar(c, i, 0, 0);
						ksceKernelDelayThread(3000);
					}
				}
				/*If battery level is 3, turn lightbar red*/
				else if (battery_level == 0x3) {
					sceCtrlSetLightBar(c, 255 * lightbarbrightness / 100, 0, 0);
				}
				/*If battery level is 4, turn lightbar yellow*/
				else if (battery_level == 0x4) {
					sceCtrlSetLightBar(c, 255 * lightbarbrightness / 100, 255 * lightbarbrightness / 100, 0);
				}
				/*If battery level is 5, turn lightbar green*/
				else if (battery_level == 0x5) {
					sceCtrlSetLightBar(c, 0, 255 * lightbarbrightness / 100, 0);
				}
				/*If battery is charging, turn lightbar magenta*/
				else if (battery_level == 0xEE) {
					sceCtrlSetLightBar(c, 200 * lightbarbrightness / 100, 0, 255 * lightbarbrightness / 100);
				}
				/*If battery level is fully charged, turn lightbar turquoise*/
				else if (battery_level == 0xEF) {
					sceCtrlSetLightBar(c, 0, 255 * lightbarbrightness / 100, 255 * lightbarbrightness / 100);
				}
			}
		}
		/*Add delay in between iterations for stability*/
		ksceKernelDelayThread(100000);
    }
    return 0;
}

/*Code execution starts from this point. Loaded by kernel.*/
int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { 
	
	(void)argc; 
	(void)argv;
	
	/*Define modInfo for SceCtrl*/
	tai_module_info_t modInfo;
	
	/*Define modInfo for SceAVConfig*/
	tai_module_info_t modInfo1;
	
	SceUID thread_id;
	
    modInfo.size = sizeof(modInfo);
	modInfo1.size = sizeof(modInfo1);
	
	/*Get SceCtrl module info*/
    taiGetModuleInfoForKernel(KERNEL_PID, "SceCtrl", &modInfo);
	
	/*Get SceAVConfig module info*/
    taiGetModuleInfoForKernel(KERNEL_PID, "SceAVConfig", &modInfo1);
	
	/*Hook sceCtrlPeekBufferPositive2 function offset (from base address) because kernel does not export sceCtrlPeekBufferPositive2*/
    tai_uid = taiHookFunctionOffsetForKernel(KERNEL_PID, &hook, modInfo.modid, 0, 0x3EF8, 1, sceCtrlPeekBufferPositive2_patched);
	
	/*Hook ksceDisplaySetFrameBufInternal if device is PSTV*/
	if (!ksceSblAimgrIsGenuineVITA()){
		tai_uid1 = taiHookFunctionExportForKernel(KERNEL_PID, &hook1, "SceDisplay", 0x9FED47AC, 0x16466675, ksceDisplaySetFrameBufInternal_patched);
	}
	
	/*sceCtrlGetBatteryInfo and sceCtrlSetLightBar userland functions are not listed in psp2kern/ctrl.h header.
	module_get_offset is called to add the offsets of these functions (0x5E95 and 0x5D81 in this case) to the base address
	of the module that they belong to. These resulting addresses belong to the sceCtrlGetBatteryInfo and sceCtrlSetLightBar
	userland functions and they are stored in the function pointers so the functions themselves can be used in this kernel plugin.*/
	batteryinfo = module_get_offset(KERNEL_PID, modInfo.modid, 0, 0x5E95, (uintptr_t*)&sceCtrlGetBatteryInfo);
	lightbarinfo = module_get_offset(KERNEL_PID, modInfo.modid, 0, 0x5D81, (uintptr_t*)&sceCtrlSetLightBar);
	
	/*Store address of global variable that controls master volume level in mastervol pointer.  Global variables are stored in segidx 1*/
	module_get_offset(KERNEL_PID, modInfo1.modid, 1, 0x280, (uintptr_t*)&mastervol);
	/*Store address of global variable that holds master volume change event id in mastervolchange pointer.  Global variables are stored in segidx 1*/
	module_get_offset(KERNEL_PID, modInfo1.modid, 1, 0xF4, (uintptr_t*)&mastervolchange);

	/*Create thread to control DS4 lightbar.  Highest thread priority is 64.  Lowest thread priority is 191.*/
	thread_id = ksceKernelCreateThread("LightbarThread", lightbar_thread, 191, 0x10000, 0, 0, NULL);

	/*Check lightbar settings*/
	loadconfig();

	/*Start the created thread*/
	ksceKernelStartThread(thread_id, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;

	/*If tai_uid was set to a number greater than or equal to 0, that means the hook
	was a success and needs to be released during module_stop*/
	if (R_SUCCEEDED(tai_uid))
		taiHookReleaseForKernel(tai_uid, hook);
	
	/*If tai_uid1 was set to a number greater than or equal to 0, that means the device is a PSTV and the hook
	was a success... so it needs to be released during module_stop*/
	if (R_SUCCEEDED(tai_uid1))
		taiHookReleaseForKernel(tai_uid1, hook1);

    return SCE_KERNEL_STOP_SUCCESS;
}
