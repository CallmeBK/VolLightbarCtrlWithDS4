#include <string.h>
#include <taihen.h>
#include <psp2kern/kernel/types.h>
#include <psp2kern/ctrl.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>

#define R_SUCCEEDED(res)   ((res)>=0)

static SceUID tai_uid;
static tai_hook_ref_t hook;
static int batteryinfo;
static int lightbarinfo;

/*declare function pointers to SceCtrl userland functions*/
int (*sceCtrlGetBatteryInfo)(int port, SceUInt8 *batt) = NULL;
int (*sceCtrlSetLightBar)(int port, SceUInt8 r, SceUInt8 g, SceUInt8 b) = NULL;

/*module_get_offset declaration required because not included in any headers. Part of taihenModuleUtils_stub.*/
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);


/*sceCtrlPeekBufferPositive2 hook function*/
int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) 
{
	int ret;

	/*Should always call TAI_CONTINUE at beginning of hook function */
    ret = TAI_CONTINUE(int, hook, port, pad_data, count);
	
	/*Make sure TAI_CONTINUE return value is not out of bounds*/
    if (ret < 1 || ret > 64){
        return ret;
	}

    /*If L1 and Select are the only buttons pressed, add volume down as a pressed button
	If R1 and Select are the only buttons pressed, add volume up as a pressed button
	Separate if statements allow for both volume down and volume up to be added as pressed buttons.
	Holding volume down and volume up buttons will mute the volume.*/
    if (pad_data->buttons == (SCE_CTRL_L1 | SCE_CTRL_SELECT))
        pad_data->buttons |= SCE_CTRL_VOLDOWN;
    if (pad_data->buttons == (SCE_CTRL_R1 | SCE_CTRL_SELECT))
        pad_data->buttons |= SCE_CTRL_VOLUP;
    return ret;
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

	/*loop runs until vita shuts down*/
    for (;;) {

		/*Check to see how many DS4 controllers are connected*/
		ksceCtrlGetControllerPortInfo(&portinfo);

		/*If DS4 is connected to port 1, check DS4 battery level*/
		if (portinfo.port[1] == 8) {
			sceCtrlGetBatteryInfo(1, &battery_level);
			/*If battery level is 0, turn off lightbar*/
			if (battery_level == 0x0) {
				sceCtrlSetLightBar(1, 0, 0, 0);
			}
			/*If battery level is 1, turn off lightbar*/
			else if (battery_level == 0x1) {
				sceCtrlSetLightBar(1, 0, 0, 0);
			}
			/*If battery level is 2, pulse lightbar with red color*/
			else if (battery_level == 0x2) {
				for (int i = 0; i<=255; i++) {
					sceCtrlSetLightBar(1, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
				for (int i = 255; i>=0; i--) {
					sceCtrlSetLightBar(1, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
			}
			/*If battery level is 3, turn lightbar red*/
			else if (battery_level == 0x3) {
				sceCtrlSetLightBar(1, 255, 0, 0);
			}
			/*If battery level is 4, turn lightbar yellow*/
			else if (battery_level == 0x4) {
				sceCtrlSetLightBar(1, 255, 255, 0);
			}
			/*If battery level is 5, turn lightbar green*/
			else if (battery_level == 0x5) {
				sceCtrlSetLightBar(1, 0, 255, 0);
			}
			/*If battery is charging, turn lightbar magenta*/
			else if (battery_level == 0xEE) {
				sceCtrlSetLightBar(1, 200, 0, 255);
			}
			/*If battery level is fully charged, turn lightbar turquoise*/
			else if (battery_level == 0xEF) {
				sceCtrlSetLightBar(1, 0, 255, 255);
			}
		}
		/*If DS4 is connected to port 2, check DS4 battery level*/
		if (portinfo.port[2] == 8) {
			sceCtrlGetBatteryInfo(2, &battery_level);
			/*If battery level is 0, turn off lightbar*/
			if (battery_level == 0x0) {
				sceCtrlSetLightBar(2, 0, 0, 0);
			}
			/*If battery level is 1, turn off lightbar*/
			else if (battery_level == 0x1) {
				sceCtrlSetLightBar(2, 0, 0, 0);
			}
			/*If battery level is 2, pulse lightbar with red color*/
			else if (battery_level == 0x2) {
				for (int i = 0; i<=255; i++) {
					sceCtrlSetLightBar(2, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
				for (int i = 255; i>=0; i--) {
					sceCtrlSetLightBar(2, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
			}
			/*If battery level is 3, turn lightbar red*/
			else if (battery_level == 0x3) {
				sceCtrlSetLightBar(2, 255, 0, 0);
			}
			/*If battery level is 4, turn lightbar yellow*/
			else if (battery_level == 0x4) {
				sceCtrlSetLightBar(2, 255, 255, 0);
			}
			/*If battery level is 5, turn lightbar green*/
			else if (battery_level == 0x5) {
				sceCtrlSetLightBar(2, 0, 255, 0);
			}
			/*If battery is charging, turn lightbar magenta*/
			else if (battery_level == 0xEE) {
				sceCtrlSetLightBar(2, 200, 0, 255);
			}
			/*If battery level is fully charged, turn lightbar turquoise*/
			else if (battery_level == 0xEF) {
				sceCtrlSetLightBar(2, 0, 255, 255);
			}
		}
		/*If DS4 is connected to port 3, check DS4 battery level*/
		if (portinfo.port[3] == 8) {
			sceCtrlGetBatteryInfo(3, &battery_level);
			/*If battery level is 0, turn off lightbar*/
			if (battery_level == 0x0) {
				sceCtrlSetLightBar(3, 0, 0, 0);
			}
			/*If battery level is 1, turn off lightbar*/
			else if (battery_level == 0x1) {
				sceCtrlSetLightBar(3, 0, 0, 0);
			}
			/*If battery level is 2, pulse lightbar with red color*/
			else if (battery_level == 0x2) {
				for (int i = 0; i<=255; i++) {
					sceCtrlSetLightBar(3, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
				for (int i = 255; i>=0; i--) {
					sceCtrlSetLightBar(3, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
			}
			/*If battery level is 3, turn lightbar red*/
			else if (battery_level == 0x3) {
				sceCtrlSetLightBar(3, 255, 0, 0);
			}
			/*If battery level is 4, turn lightbar yellow*/
			else if (battery_level == 0x4) {
				sceCtrlSetLightBar(3, 255, 255, 0);
			}
			/*If battery level is 5, turn lightbar green*/
			else if (battery_level == 0x5) {
				sceCtrlSetLightBar(3, 0, 255, 0);
			}
			/*If battery is charging, turn lightbar magenta*/
			else if (battery_level == 0xEE) {
				sceCtrlSetLightBar(3, 200, 0, 255);
			}
			/*If battery level is fully charged, turn lightbar turquoise*/
			else if (battery_level == 0xEF) {
				sceCtrlSetLightBar(3, 0, 255, 255);
			}
		}
		/*If DS4 is connected to port 4, check DS4 battery level*/
		if (portinfo.port[4] == 8) {
			sceCtrlGetBatteryInfo(4, &battery_level);
			/*If battery level is 0, turn off lightbar*/
			if (battery_level == 0x0) {
				sceCtrlSetLightBar(4, 0, 0, 0);
			}
			/*If battery level is 1, turn off lightbar*/
			else if (battery_level == 0x1) {
				sceCtrlSetLightBar(4, 0, 0, 0);
			}
			/*If battery level is 2, pulse lightbar with red color*/
			else if (battery_level == 0x2) {
				for (int i = 0; i<=255; i++) {
					sceCtrlSetLightBar(4, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
				for (int i = 255; i>=0; i--) {
					sceCtrlSetLightBar(4, i, 0, 0);
					ksceKernelDelayThread(3000);
				}
			}
			/*If battery level is 3, turn lightbar red*/
			else if (battery_level == 0x3) {
				sceCtrlSetLightBar(4, 255, 0, 0);
			}
			/*If battery level is 4, turn lightbar yellow*/
			else if (battery_level == 0x4) {
				sceCtrlSetLightBar(4, 255, 255, 0);
			}
			/*If battery level is 5, turn lightbar green*/
			else if (battery_level == 0x5) {
				sceCtrlSetLightBar(4, 0, 255, 0);
			}
			/*If battery is charging, turn lightbar magenta*/
			else if (battery_level == 0xEE) {
				sceCtrlSetLightBar(4, 200, 0, 255);
			}
			/*If battery level is fully charged, turn lightbar turquoise*/
			else if (battery_level == 0xEF) {
				sceCtrlSetLightBar(4, 0, 255, 255);
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
	
	tai_module_info_t modInfo;
	SceUID thread_id;
	
    modInfo.size = sizeof(modInfo);
	/*Get SceCtrl module info*/
    taiGetModuleInfoForKernel(KERNEL_PID, "SceCtrl", &modInfo);
	/*Hook sceCtrlPeekBufferPositive2 function offset (from base address) because kernel does not export sceCtrlPeekBufferPositive2*/
    tai_uid = taiHookFunctionOffsetForKernel(KERNEL_PID, &hook, modInfo.modid, 0, 0x3EF8, 1, sceCtrlPeekBufferPositive2_patched);
	
	/*sceCtrlGetBatteryInfo and sceCtrlSetLightBar userland functions are not listed in psp2kern/ctrl.h header.
	module_get_offset is called to add the offsets of these functions (0x5E95 and 0x5D81 in this case) to the base address
	of the module that they belong to. These resulting addresses belong to the sceCtrlGetBatteryInfo and sceCtrlSetLightBar
	userland functions and they are stored in the function pointers so the functions themselves can be used in this kernel plugin.*/
	batteryinfo = module_get_offset(KERNEL_PID, modInfo.modid, 0, 0x5E95, (uintptr_t*)&sceCtrlGetBatteryInfo);
	lightbarinfo = module_get_offset(KERNEL_PID, modInfo.modid, 0, 0x5D81, (uintptr_t*)&sceCtrlSetLightBar);

	/*Create thread to control DS4 lightbar.  Highest thread priority is 64.  Lowest thread priority is 191.*/
	thread_id = ksceKernelCreateThread("LightbarThread", lightbar_thread, 191, 0x10000, 0, 0, NULL);

	/*Start the created thread*/
	ksceKernelStartThread(thread_id, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;

	/*If tai_uid was set to a number greater than or equal to 0, that means the hook
	was a success and needs to be released during module_stop*/
	if (R_SUCCEEDED(tai_uid))
		taiHookReleaseForKernel(tai_uid, hook);

    return SCE_KERNEL_STOP_SUCCESS;
}