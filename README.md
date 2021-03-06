# Preface
This was the first project I've ever written in c.  Most of my programming experience comes from VBA so I had a lot of help on this project and learned a lot (see credit section at bottom).

# VolLightbarCtrlWithDS4
Kernel plugin for PS Vita.  Requires vita firmware 3.60, Henkaku Enso, and the Minivitatv plugin to work.
Also supports PSTV.

# Installation Instructions
1) Download zip from latest release
2) Copy VolLightbarCtrlWithDS4.skprx to ur0:/tai folder on ps vita
3) Put "ur0:/tai/VolLightbarCtrlWithDS4.skprx" (without quotes) under KERNEL section in Config.txt file
4) Reboot vita

# What it does
Control volume using DS4 controller.  
DS4 lightbar used as DS4 battery indicator.  
Adjust DS4 lightbar brightness.  
Supports up to 4 connected controllers.  

# How to use
Press Select and L1 simultaneously to lower volume  
Press Select and R1 simultaneously to raise volume  
Press Select, L1, and R1 simultaneously to mute volume  
Press L1, L2, R1, and R2 simultaneously to toggle DS4 lightbar brightess  

Magenta lightbar means DS4 is charging  
Turquoise lightbar means DS4 is fully charged  
Green lightbar means DS4 is high charge  
Yellow lightbar means DS4 is medium charge  
Red lightbar means DS4 is low charge  
Red pulsing lightbar means DS4 will run out of charge soon  

# Credits
xerpi  
-for directing me to the discord henkaku server

marburg  
-for providing lightbar mod example code  
-letting me know to use SceLibKernel_stub in my cmakelists.txt  
-for letting me know that a function in my user thread could be interrupting the reboot sequence  

SKGleba  
-for confirming that the basic difference between user and kernel is privilege.  
-for teaching me that *ALL in config.txt includes *main  
-for teaching me that the format for checking if titleid string belongs to main --> titleid[0]=='\0'  
-for informing me of ksceDebugPrintf (kernel printf function)  
-for giving me the idea to use db.yml to find required stubs  
-for informing me that I need to have taimoduleutils_stub for module_get_offset to work in kernel plugin  
-for suggesting to reverse engineer sceAVConfigGetSystemVol, sceAVConfigSetSystemVol, sceAVConfigMuteOn functions  
-for suggesting tools/programs to use for reverse engineering functions  
-for informing me that bootimage is just a bunch of elfs appended to each other and then encrypted to one self  

davee  
-for teaching me that you should choose user over kernel everytime you get the option because with less privileges, you're less likely to cause system crashes if something goes wrong.  
-for teaching me what "nostdlib" link flag does in cmakelists.txt  
-for teaching me that plugins are injected into whatever process is described in the config.txt.  It's lifetime is determined by whatever process it lives in.  
-for teaching me that *ALL in config.txt includes *main  
-for teaching me that adding a delay in an infinite loop provides cooperative scheduling to prevent my plugin from hogging the cpu  
-for informing me that using sceKernelDelayThread in a hook is bad form  

Bythos  
-for letting me know minivitatv plugin supports lightbar and DS4vita doesn't  
-for supplying me with the user function used to print to PrincessLog and how to use it  
-for helping me setup PrincessLog and getting it to work  
-for the suggestion of using |= instead of = to assign volume buttons to pad_data->buttons  
-for suggesting that I need to hook sceCtrlPeekBufferPositive2 for LiveArea to recognize my inputs because SceShell imports sceCtrlPeekBufferPositive2  
-for suggesting one can use userspace functions in kernel if they get the address of the module in memory and use offsets.  Then hook the function offsets (taihen).  
-for suggesting how to simplify my if statements in the sceCtrlPeekBufferPositive2 hook function  
-for informing me that all functions in same module have same base address  
-for informing me that known kernel functions in db.yml can be hooked by export and don't require offset.  All libraries in db.yml that have kernel set to true, have the named functions exported from the kernel.  
-for informing me that hooking imports requires NID hooking  
-for informing me that the kernel exports functions so that other apps can import them.  So when you hook a function export, the code gets run every single time an application uses that same function as an import.  
-for informing me that Module = ELF on vita  
-for informing me that dynamic libraries and SceShell export functions.  Applications and dynamic libraries import functions.  Executables are modules.  
-for informing me that plugins are basically dynamic libraries that usually don't export functions  
-for informing me that SceShell calls sceCtrlPeekBufferPositive2.  Different apps use different control functions.  
-for informing me that SceShell is always running in the background  
-for informing me to use module_get_offset to use kernel function equivalents to user functions that aren't exported by the kernel (in other words, user functions that aren't listed in the kernel header)  
-for informing me that module_get_offset gets the base address of the module and adds the offset of the function to the base address.  This result is the address of the function you want to use and you would call it using a function pointer.  
-for giving me an example of declaring a function pointer in c. int (*ksceCtrlPeekBufferPositve2)(int, SceCtrlData*, int) = NULL; Pass it to module_get_offset last argument as &ksceCtrlPeekBufferPositive2  
-for informing me that sceKernelDelayThread works in function hooks too (but it's bad form and could hurt a tight loop)  
-for informing me that segidx argument in module_get_offset is the index of the segment in which the function I'm looking for is.  There are 4 segments and usually functions are found in segment 0  
-for teaching me how to use callback functions  
-for informing me that putting the vita to sleep mode does not run module_stop function  
-for informing me that event flags are set by my own code  
-for supplying me with sceAVConfigGetSystemVol, sceAVConfigSetSystemVol, sceAVConfigMuteOn offsets  
-for directing me to KuromeSan github page for psvita-elfs  
-for informing me that AVConfig elf is in bootimage  
-for informing me that a module can have multiple stubs  
-for teaching me how to understand the code that is decompiled by Ghidra  
-for introducing me to a function that determines if the current device is VITA or PSTV  
-for informing me that kernel exports and user space syscalls are exported from the same module for most kernel space stuff  
-for teaching me about what the segidx parameter is for in module_get_offset function.  Functions are put into segment 0.  Global variables are put into segment 1.  
-for informing me that kernel space and user space share the same global variables  
-for informing me that arguments from the function call map linearly in Ghidra  
-for informing me that to get SceShell module info for Kernel, you have to wait until SceShell is loaded before calling taiGetModuleInfoForKernel  
-for informing me that a function that exists in a stub, but is not in any headers can be declared and used all the same.  Stubs are not incomplete and are not missing functions from modules.  

NOTxCorra  
-for informing me of header/stub needed for sceClibPrintf function  
-for letting me know that caps actually matter  
-for helping me setup PrincessLog and getting it to work  
-for teaching me that shell/main title id can be referred to as \0  
-for teaching me that adding a sceKernelDelayThread in my infinite loop is good practice  
-for teaching me that arrays are pointers  
-for teaching me to call TAI_CONTINUE first inside function hooks  
-for informing me that function export is exported from module.  Function import is imported from module.  Offset is from base address.  

teakhanirons  
-for pointing me to PrincessLog debugging solution  
-for teaching me that *ALL in config.txt includes *main  
-for teaching me that headers in psp2 are user and psp2kern are kernel  
-for pointing me to documentation on thread priority  
-for pointing me to examples of kernel and user forms of blit to print to the screen as an overlay  
-for confirming that one can use userspace functions in kernel if they get the address of the module in memory and use offsets  
-for providing the offset for sceCtrlPeekBufferPositive2 and providing an example using taiHookFunctionOffsetForKernel  
-for helping Mer1e with hooking the function offset for sceCtrlPeekBufferPositive2  
-for informing me that SceShell calls sceCtrlPeekBufferPositive2  
-for suggesting to use a for loop to cycle through each controller in the lightbar_thread code

Princess of Sleeping  
-for teaching me that shell/main title id is referred to as \0, main, or NPXS19999  
-for correcting the offsets that Mer1e supplied for sceCtrlGetBatteryInfo and sceCtrlSetLightBar.  ghidra returns even addresses when thumb functions are odd so you need to increase the ghidra offset by one byte for thumb functions.  
-for informing me that if ksceKernelExitThread is called and then vita goes to sleep, waking from sleep does not auto start threads again.  You need to call the startthread function.  
-for informing me about waiting with event flags and giving me an example of how it is done  

Mer1e  
-for the idea to hook a ksceCtrl function and add SCE_CTRL_VOLUP and SCE_CTRL_VOLDOWN to pad_data->buttons  
-for letting me know I needed SceDebugForDriver_stub and sysmem.h header to call ksceDebugPrintf  
-for informing me to always call TAI_CONTINUE before any returns  
-for informing me to check that TAI_CONTINUE returns a value between 1 and 64 before any code runs inside the hook function  
-for providing a working sample of of hooking the function offset for sceCtrlPeekBufferPositive2  
-for supplying me with the offsets for sceCtrlGetBatteryInfo and sceCtrlSetLightBar  
-for informing me that Kernel plugins are only loaded in one instance by the kernel.  User plugins under *MAIN are loaded in one instance under SceShell process.  User plugins under *ALL are loaded in separated instances for each of the processes running.  
-for informing me that the last argument in module_get_offset should be passed in the form of (uintptr_t*)&ksceCtrlPeekBufferPositive2  

Orangelampshade  
-for testing PSTV compatibility  

Rinnegatamante  
-for AnalogsEnhancer plugin.  I used this as reference for how to open, read, and write to a text file for storing configuration settings.  

cuevavirus  
-for Quick Menu Plus plugin.  I used this as reference for converting a char array to an integer data type.  

aliihsanasl  
-for the idea to give this plugin the ability to change lightbar brightness.  
