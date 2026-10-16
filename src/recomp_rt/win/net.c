/* WSOCK32 (a machine with no network) and printing (GDI32, WINSPOOL, comdlg32: no printer). */
#include "win.h"

/* WSOCK32 by ordinal. WSAStartup fails with WSASYSNOTREADY, as on a machine whose network is not
 * ready; the network game then takes its own failure path. Every other call fails as it does before a
 * successful WSAStartup. */
#define WSASYSNOTREADY 10091u
#define WSANOTINITIALISED 10093u
#define SOCKET_ERROR 0xffffffffu

RT_IMPORT(WSOCK32_ord115) { rt_ret_stdcall(WSASYSNOTREADY, 2); }                  /* WSAStartup */
RT_IMPORT(WSOCK32_ord116) { rt_ret_stdcall(SOCKET_ERROR, 0); }                    /* WSACleanup */
RT_IMPORT(WSOCK32_ord111) { rt_ret_stdcall(WSANOTINITIALISED, 0); }               /* WSAGetLastError */
RT_IMPORT(WSOCK32_ord57) { rt_ret_stdcall(SOCKET_ERROR, 2); }                     /* gethostname */
RT_IMPORT(WSOCK32_ord51) { rt_ret_stdcall(0, 1); }                                /* gethostbyname */
RT_IMPORT(WSOCK32_ord23) { rt_ret_stdcall(0xffffffffu, 3); }                      /* socket: INVALID_SOCKET */
RT_IMPORT(WSOCK32_ord3) { rt_ret_stdcall(SOCKET_ERROR, 1); }                      /* closesocket */
RT_IMPORT(WSOCK32_ord1) { rt_ret_stdcall(0xffffffffu, 3); }                       /* accept */
RT_IMPORT(WSOCK32_ord2) { rt_ret_stdcall(SOCKET_ERROR, 3); }                      /* bind */
RT_IMPORT(WSOCK32_ord4) { rt_ret_stdcall(SOCKET_ERROR, 3); }                      /* connect */
RT_IMPORT(WSOCK32_ord13) { rt_ret_stdcall(SOCKET_ERROR, 2); }                     /* listen */
RT_IMPORT(WSOCK32_ord16) { rt_ret_stdcall(SOCKET_ERROR, 4); }                     /* recv */
RT_IMPORT(WSOCK32_ord17) { rt_ret_stdcall(SOCKET_ERROR, 6); }                     /* recvfrom */
RT_IMPORT(WSOCK32_ord19) { rt_ret_stdcall(SOCKET_ERROR, 4); }                     /* send */
RT_IMPORT(WSOCK32_ord20) { rt_ret_stdcall(SOCKET_ERROR, 6); }                     /* sendto */
RT_IMPORT(WSOCK32_ord21) { rt_ret_stdcall(SOCKET_ERROR, 5); }                     /* setsockopt */
RT_IMPORT(WSOCK32_ord9)                                                           /* htons */
{
    uint32_t v = rt_arg(0) & 0xffffu;
    rt_ret_stdcall(((v & 0xff) << 8) | (v >> 8), 1);
}
RT_IMPORT(WSOCK32_ord12)                                                          /* inet_ntoa */
{
    static uint32_t text;
    uint32_t a = rt_arg(0);
    char s[16];
    snprintf(s, sizeof s, "%u.%u.%u.%u", a & 0xff, (a >> 8) & 0xff, (a >> 16) & 0xff, a >> 24);
    if (!text) text = win_static_alloc(16);
    win_put_str(text, s, 16);
    rt_ret_stdcall(text, 1);
}

/* Printing: the print function 0x84c830 is replaced by the PDF writer (print_pdf.c), so PrintDlgA and
 * the GDI32 calls are not reached; they fail. EnumPrintersA answers that one printer exists (0x84c7f0
 * returns "returned != 0"), so the default of Print To is Printer, which the game shows as PDF. */
RT_IMPORT(EnumPrintersA)
{
    if (rt_arg(6)) wr32(g_mem, rt_arg(6), 1);       /* returned: the number of printers */
    rt_ret_stdcall(1, 7);
}
RT_IMPORT(PrintDlgA) { rt_ret_stdcall(0, 1); }
RT_IMPORT(GetDeviceCaps) { rt_ret_stdcall(0, 2); }
RT_IMPORT(CreateFontA) { rt_ret_stdcall(0, 14); }
RT_IMPORT(GetTextMetricsA) { rt_ret_stdcall(0, 2); }
RT_IMPORT(StartDocA) { rt_ret_stdcall(0xffffffffu, 2); }
RT_IMPORT(StartPage) { rt_ret_stdcall(0xffffffffu, 1); }
RT_IMPORT(EndPage) { rt_ret_stdcall(0xffffffffu, 1); }
RT_IMPORT(EndDoc) { rt_ret_stdcall(0xffffffffu, 1); }
RT_IMPORT(SelectObject) { rt_ret_stdcall(0, 2); }
RT_IMPORT(TextOutA) { rt_ret_stdcall(0, 5); }
RT_IMPORT(DeleteObject) { rt_ret_stdcall(0, 1); }
