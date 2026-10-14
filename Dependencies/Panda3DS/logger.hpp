#pragma once
#include <cstdarg>
#include <fstream>


namespace Log {
// Our logger class
template <bool enabled>
class Logger {
public:
    Logger(std::string prefix = "") : prefix(prefix) {}

    std::string prefix;

    void log(const char* fmt, ...) {
        if constexpr (!enabled) return;

        std::fputs(prefix.c_str(), stdout);
        std::va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);
    }

    void logNoPrefix(const char* fmt, ...) {
        if constexpr (!enabled) return;

        std::va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);
    }
};

// Our loggers here. Enable/disable by toggling the template param

// Never redefine the C++ keywords 'true' or 'false'. The old macro
// silently broke CPU, memory and input logic in every including source.
// Use explicit per-logger template settings instead.

// Modules
static Logger cellAudioOut      = Logger<false> ("[Module ][cellAudioOut  ] ");
static Logger cellAudio         = Logger<false> ("[Module ][cellAudio     ] ");
static Logger cellMsgDialog     = Logger<false> ("[Module ][cellMsgDialog ] ");
static Logger cellSysCache      = Logger<false> ("[Module ][cellSysCache  ] ");
static Logger cellSsl           = Logger<false> ("[Module ][cellSsl       ] ");
static Logger cellKb            = Logger<false> ("[Module ][cellKb        ] ");
static Logger cellPad           = Logger<false> ("[Module ][cellPad       ] ");
static Logger cellSaveData      = Logger<false> ("[Module ][cellSaveData  ] ");
static Logger sceNp             = Logger<false> ("[Module ][sceNp         ] ");
static Logger sceNpTrophy       = Logger<false> ("[Module ][sceNpTrophy   ] ");
static Logger cellPngDec        = Logger<false> ("[Module ][cellPngDec    ] ");
static Logger cellFs            = Logger<false> ("[Module ][cellFs        ] ");
static Logger cellRtc           = Logger<false> ("[Module ][cellRtc       ] ");
static Logger cellSpurs         = Logger<false> ("[Module ][cellSpurs     ] ");
static Logger cellGame          = Logger<false> ("[Module ][cellGame      ] ");
static Logger cellResc          = Logger<false>("[Module ][cellResc      ] ");
static Logger cellSysmodule     = Logger<false> ("[Module ][cellSysmodule ] ");
static Logger cellSysutil       = Logger<false> ("[Module ][cellSysutil   ] ");
static Logger cellGcmSys        = Logger<false> ("[Module ][cellGcmSys    ] ");
static Logger cellVideoOut      = Logger<false> ("[Module ][cellVideoOut  ] ");
static Logger sysLwCond         = Logger<false> ("[Module ][sysLwCond     ] ");
static Logger sysLwMutex        = Logger<false> ("[Module ][sysLwMutex    ] ");
static Logger sysMMapper        = Logger<false> ("[Module ][sysMMapper    ] ");
static Logger sysPrxForUser     = Logger<false>("[Module ][sysPrxForUser ] ");
static Logger sysThread         = Logger<false> ("[Module ][sysThread     ] ");

// Syscalls
static Logger sys_spu           = Logger<false> ("[Syscall][sys_spu       ] ");
static Logger sys_semaphore     = Logger<false> ("[Syscall][sys_semaphore ] ");
static Logger sys_ppu_thread    = Logger<false> ("[Syscall][sys_ppu_thread] ");
static Logger sys_prx           = Logger<false> ("[Syscall][sys_prx       ] ");
static Logger sys_cond          = Logger<false> ("[Syscall][sys_cond      ] ");
static Logger sys_rwlock        = Logger<false> ("[Syscall][sys_rwlock    ] ");
static Logger sys_mutex         = Logger<false> ("[Syscall][sys_mutex     ] ");
static Logger sys_event         = Logger<false> ("[Syscall][sys_event     ] ");
static Logger sys_vm            = Logger<false> ("[Syscall][sys_vm        ] ");
static Logger sys_memory        = Logger<false> ("[Syscall][sys_memory    ] ");
static Logger sys_mmapper       = Logger<false> ("[Syscall][sys_mmapper   ] ");
static Logger sys_timer         = Logger<false>("[Syscall][sys_timer     ] ");
static Logger sys_fs            = Logger<false> ("[Syscall][sys_fs        ] ");
static Logger misc_sc           = Logger<false> ("[Syscall][Misc          ] ");
static Logger tty               = Logger<1>    ();

// RSX
static Logger rsx               = Logger<false>("[RSX    ][Command       ] ");
static Logger vertex_shader     = Logger<false> ("[Shader ][Vertex        ] ");
static Logger fragment_shader   = Logger<false> ("[Shader ][Fragment      ] ");
static Logger rsx_cache         = Logger<false> ("[RSX    ][Cache         ] ");

// Loader
static Logger loader_elf        = Logger<1>    ("[Loader ][ELF           ] ");
static Logger loader_spu        = Logger<1>    ("[Loader ][SPU           ] ");
static Logger loader_prx        = Logger<1>    ("[Loader ][PRX           ] ");
static Logger loader_sfo        = Logger<1>    ("[Loader ][SFO           ] ");
static Logger loader_game       = Logger<1>    ("[Loader ][Game          ] ");

// Manager
static Logger manager_prx       = Logger<1>    ("[Manager][PRX           ] ");

// Thread
static Logger thread            = Logger<false>("[Thread ][PPU           ] ");
static Logger thread_spu        = Logger<false> ("[Thread ][SPU           ] ");

// Other
static Logger memory            = Logger<false> ("[Other  ][Memory        ] ");
static Logger lle_module        = Logger<false> ("[LLE    ][Module        ] ");
static Logger filesystem        = Logger<false> ("[Other  ][Filesystem    ] ");
static Logger lv2_obj           = Logger<false> ("[Other  ][Lv2 Object    ] ");
static Logger unimplemented     = Logger<false> ("[Other  ][Unimplemented ] ");

#undef true
#undef false

// We have 2 ways to create a log function
// MAKE_LOG_FUNCTION: Creates a log function which is toggleable but always killed for user-facing builds
// MAKE_LOG_FUNCTION_USER: Creates a log function which is toggleable, may be on for user builds as well
// We need this because sadly due to the loggers taking variadic arguments, compilers will not properly
// Kill them fully even when they're disabled. The only way they will is if the function with varargs is totally empty

#define MAKE_LOG_FUNCTION_USER(functionName, logger)                    \
	template <typename... Args>                                         \
	void functionName(const char* fmt, Args&&... args) {                \
		Log::logger.log(fmt, args...);                             \
	}                                                                   \
    template <typename... Args>                                         \
    void functionName##NoPrefix(const char* fmt, Args&&... args) {      \
        Log::logger.logNoPrefix(fmt, args...);                     \
    }

#ifdef CHONKYSTATION3_USER_BUILD
#define MAKE_LOG_FUNCTION(functionName, logger)             \
	template <typename... Args>                             \
	void functionName(const char* fmt, Args&&... args) {}   \
    template <typename... Args>                             \
	void functionName##NoPrefix(const char* fmt, Args&&... args) {}
#else
#define MAKE_LOG_FUNCTION(functionName, logger) MAKE_LOG_FUNCTION_USER(functionName, logger)
#endif
}