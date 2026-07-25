#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    // #include <winsock2.h>
    #include <windows.h>
    #include <direct.h>
    #include <stdlib.h>

    #define __unused

    #define usleep(x) Sleep((x)/1000)
    #define mkdir(path, mode) _mkdir(path)

    #define __builtin_bswap16(x) _byteswap_ushort(x)
    #define __builtin_bswap32(x) _byteswap_ulong(x)

    typedef SSIZE_T ssize_t;

    static inline void GetHomeDirectory(char *out, size_t size_out) {
        char *home = NULL;
        size_t length = 0;

        if (_dupenv_s(&home, &length, "USERPROFILE") == 0 && home) {
            strncpy(out, home, size_out - 1);
            out[size_out - 1] = '\0';
            free(home);
            return;
        }

        strncpy(out, "C:\\Users", size_out - 1);
        out[size_out - 1] = '\0'; 
    }
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <stdlib.h>

    #ifndef __unused
        #define __unused __attribute__((unused))
    #endif

    static inline void GetHomeDirectory(char *out, size_t size_out) {
        const char *home = getenv("HOME");
        strncpy(out, home ? home : "/tmp", size_out - 1);
        out[size_out - 1] = '\0';
    }
#endif

#endif // PLATFORM_H
