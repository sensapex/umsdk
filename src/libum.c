/*
 * A software development kit for Sensapex 2015 series Micromanipulators,
 * Microscope stage and Pressure controller
 *
 * Copyright (c) 2015-2024, Sensapex Oy
 * All rights reserved.
 *
 * This file is part of 2015 series Sensapex uMx device SDK
 *
 * The Sensapex uMx SDK is free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * The Sensapex Micromanipulator SDK is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Sensapex micromanipulator SDK. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "libum.h"
#include "smcp1.h"

#define LIBUM_VERSION_STR    "v1.501"
#define LIBUM_COPYRIGHT      "Copyright (c) Sensapex 2017-2024. All rights reserved"

#define LIBUM_MAX_MESSAGE_SIZE   1502
#define LIBUM_ANY_IPV4_ADDR  "0.0.0.0"

// 169.254.0.0/16
#define LINK_LOCAL_IPV4_NET   0xA9FE0000

typedef unsigned char um_message[LIBUM_MAX_MESSAGE_SIZE];

#ifdef __WINDOWS
HRESULT __stdcall DllRegisterServer(void) { return S_OK; }
HRESULT __stdcall DllUnregisterServer(void) { return S_OK; }
#endif

#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __func__
#endif

#ifndef LIBUM_SHARED_DO_NOT_EXPORT
const char rcsid[] = "$Id: Sensapex uMx device SDK " LIBUM_VERSION_STR " " __DATE__ " " LIBUM_COPYRIGHT " Exp $";
#endif

// ftime has been depricated and replaced with gettimeofday
#ifdef _WINDOWS
#include <sys/timeb.h>

#pragma comment(lib, "ws2_32.lib")
static int gettimeofday(struct timeval* t, void *timezone)
{
    (void) timezone;
    struct _timeb timebuffer;
    _ftime(&timebuffer );
    t->tv_sec=(long)timebuffer.time;
    t->tv_usec=1000*timebuffer.millitm;
    return 0;
}
#else

#include <sys/time.h>

#endif

// Forward declaration
static int um_send_msg(um_state *hndl, const int dev, const int cmd, const int argc, const int *argv, const int argc2,
                       const int *argv2, // optional second sub block
                       const int respc, int *respv);

const char *um_get_version() { return LIBUM_VERSION_STR; }

unsigned long long um_get_timestamp_us() {
    struct timeval tv;
    if (gettimeofday (&tv, NULL) < 0) {
        return 0;
    }
    return (unsigned long long) tv.tv_sec * 1000000LL + (unsigned long long) tv.tv_usec;
}

unsigned long long um_get_timestamp_ms() {
    return um_get_timestamp_us () / 1000LL;
}

static unsigned long long get_elapsed(const unsigned long long ts_ms) {
    return um_get_timestamp_ms () - ts_ms;
}

static const char *get_errorstr(const int error_code, char *buf, size_t buf_size) {
#ifndef _WINDOWS
    if (strerror_r (error_code, buf, buf_size) < 0)
        snprintf(buf, buf_size, "error code %d", error_code);
#else
    if(error_code == timeoutError)
        strncpy(buf, "timeout", buf_size);
    else
    {
#ifdef _WINDOWS_TEXTUAL_ERRORS // Wrong language and charset combination for non English locales
        LPCWSTR wcBuffer = NULL;
        if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL, error_code, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                          (LPWSTR)&wcBuffer, 0, NULL) || !wcBuffer)
            snprintf(buf, buf_size, "error code %d", error_code);
        else
            WideCharToMultiByte(CP_UTF8, 0, wcBuffer, -1, buf, buf_size, NULL, NULL);
        if(wcBuffer)
            LocalFree((LPWSTR)wcBuffer);
#else
        // Better to print just the number
        snprintf(buf, buf_size, "error code %d", error_code);
#endif // _WINDOWS_TEXTUAL_ERRORS
    }
#endif // _WINDOWS
    return buf;
}

um_error um_last_error(const um_state *hndl) {
    if (!hndl) {
        return LIBUM_NOT_OPEN;
    }
    return hndl->last_error;
}

int um_last_os_errno(const um_state *hndl) {
    if (!hndl) {
        return LIBUM_NOT_OPEN;
    }
    return hndl->last_os_errno;
}

const char *um_last_os_errorstr(um_state *hndl) {
    if (!hndl) {
        return um_errorstr (LIBUM_NOT_OPEN);
    }
    return hndl->errorstr_buffer;
}

const char *um_last_errorstr(um_state *hndl) {
    static char open_errorstr[80];
    // Special handling for a NULL handle
    if (!hndl) {
#ifndef _WINDOWS
        int error_code = errno;
#else
        int error_code = GetLastError();
#endif
        if (!error_code) {
            return um_errorstr (LIBUM_NOT_OPEN);
        }
        return get_errorstr (error_code, open_errorstr, sizeof (open_errorstr));
    }
    if (strlen (hndl->errorstr_buffer)) {
        return hndl->errorstr_buffer;
    }
    return um_errorstr (hndl->last_error);
}

const char *um_errorstr(const int ret_code) {
    const char *errorstr;
    if (ret_code >= 0) {
        return "No error";
    }
    switch (ret_code) {
        case LIBUM_OS_ERROR:
            errorstr = "Operation system error";
            break;
        case LIBUM_NOT_OPEN:
            errorstr = "Not opened";
            break;
        case LIBUM_TIMEOUT:
            errorstr = "Timeout";
            break;
        case LIBUM_INVALID_ARG:
            errorstr = "Invalid argument";
            break;
        case LIBUM_INVALID_DEV:
            errorstr = "Invalid device id";
            break;
        case LIBUM_INVALID_RESP:
            errorstr = "Invalid response";
            break;
        case LIBUM_PEER_ERROR:
            errorstr = "Peer failure";
            break;
        default:
            errorstr = "Unknown error";
            break;
    }
    return errorstr;
}

static void um_log_print(um_state *hndl, const int verbose_level, const char *func, const char *fmt, ...) {
    if (hndl->verbose < verbose_level) {
        return;
    }
    va_list args;
    char message[LIBUM_MAX_LOG_LINE_LENGTH];
    va_start(args, fmt);
    vsnprintf(message, sizeof (message) - 1, fmt, args);
    va_end(args);

    if (hndl->log_func_ptr) {
        (*hndl->log_func_ptr) (verbose_level, hndl->log_print_arg, func, message);
    } else {
        fprintf (stderr, "%s: %s\n", func, message);
    }
}

static int um_resolve_dev_id(const int sno) {
    int prefix = sno / 100000;
    int offset_steps = prefix - SMCP1_UMP_SNO_PREFIX;
    if (offset_steps < 0 || offset_steps > 7) {
        return sno;
    }
    int sno_lsw = sno - prefix * 100000;
    return SMCP1_UMP_DEV_ID_OFFSET + offset_steps * SMCP1_DEV_ID_OFFSET_STEP + sno_lsw;
}

static bool is_valid_new_dev_id(const int dev) {
    return dev >= SMCP1_UMP_DEV_ID_OFFSET && dev < SMCP1_UMP_DEV_ID_OFFSET + 8 * SMCP1_DEV_ID_OFFSET_STEP;
}

static bool um_resolve_sno(const int dev_id, int *sno) {
    if (dev_id < SMCP1_UMP_DEV_ID_OFFSET) {
        return false;
    }
    int dev_id_without_base_offset = dev_id - SMCP1_UMP_DEV_ID_OFFSET;
    if (sno) {
        int sno_steps = dev_id_without_base_offset / SMCP1_DEV_ID_OFFSET_STEP;
        int sno_lsw = dev_id_without_base_offset - sno_steps * SMCP1_DEV_ID_OFFSET_STEP;
        *sno = (SMCP1_UMP_SNO_PREFIX + sno_steps) * 100000 + sno_lsw;
    }
    return true;
}

static bool is_valid_sno(const int dev) {
    int ret = um_resolve_dev_id (dev);
    return ret > 0 && dev > ret;
}

static bool is_valid_legacy_dev(const int dev) {
    return dev > 0 && dev <= SMCP1_ALL_DEVICES;
}

static bool is_valid_dev(const int dev) {
    return is_valid_legacy_dev (dev) || is_valid_sno (dev) || is_valid_new_dev_id (dev);
}

static int is_invalid_dev(const int dev) {
    if (is_valid_dev (dev)) {
        return 0;
    }
    return LIBUM_INVALID_DEV;
}

static int udp_select(um_state *hndl, int timeout) {
    fd_set fdSet;
    struct timeval timev;
    if (hndl->socket == INVALID_SOCKET) {
        return -1;
    }
    if (timeout < 0) {
        timeout = hndl->timeout;
    }
    timev.tv_sec = timeout / 1000;
    timev.tv_usec = (timeout % 1000) * 1000L;
    FD_ZERO(&fdSet);
    FD_SET(hndl->socket, &fdSet);
    return select ((int) (hndl->socket) + 1, &fdSet, NULL, NULL, &timev);
}

static bool udp_set_address(IPADDR *addr, const char *s) {
    if (!addr || !s) {
        return false;
    }
    addr->sin_family = AF_INET;
    return ((int) (addr->sin_addr.s_addr = inet_addr (s)) != -1);
    // return inet_aton(s, &addr->sin_addr) > 0;
}

static int
udp_recv(um_state *hndl, unsigned char *response, const size_t response_size, IPADDR *from, const int timeout) {
    int ret;
    if ((ret = udp_select (hndl, timeout)) < 0) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "select failed - %s", strerror (hndl->last_error));
        return ret;
    } else if (!ret) {
        hndl->last_os_errno = timeoutError;
        strcpy(hndl->errorstr_buffer, "timeout");
        return ret;
    }

    socklen_t len = sizeof (IPADDR);
    if ((ret = recvfrom (hndl->socket, (char *) response, response_size, 0, (struct sockaddr *) from, &len)) ==
        SOCKET_ERROR) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "recvfrom failed - %s", strerror (hndl->last_error));
    }
    return ret;
}

static bool udp_set_sock_opt_addr_reuse(um_state *hndl) {
#ifdef _WINDOWS
    char yes = 1;
#else
    int yes = 1;
#endif
    if (setsockopt (hndl->socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) < 0) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "address reuse setopt failed - %s", strerror (hndl->last_error));
        return false;
    }
    return true;
}

static bool udp_set_sock_opt_mcast_group(um_state *hndl, const IPADDR *addr) {
    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof (mreq));
    mreq.imr_multiaddr = addr->sin_addr;
    setsockopt (hndl->socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, SOCKOPT_CAST &mreq, sizeof (mreq));
    if (setsockopt (hndl->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, SOCKOPT_CAST &mreq, sizeof (mreq)) < 0) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "join to multicast group failed - %s", strerror (hndl->last_error));
        return false;
    }
    return true;
}

static bool udp_set_sock_opt_bcast(um_state *hndl) {
#ifdef _WINDOWS
    char yes = 1;
#else
    int yes = 1;
#endif
    if (setsockopt (hndl->socket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof (yes)) < 0) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "broadcast enable failed - %s", strerror (hndl->last_error));
        return false;
    }
    return true;
}

bool udp_get_local_address(um_state *hndl, IPADDR *addr) {
    if (!addr) {
        return false;
    }
    addr->sin_family = AF_INET;
    // Obtain the local address by connecting an UDP socket
    // TCP/IP stack will resolve the correct network interface
    SOCKET testSocket;

    if ((testSocket = socket (AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "socket create failed - %s", strerror (hndl->last_error));
        return false;
    }
    socklen_t addrLen = sizeof (IPADDR);
    if (connect (testSocket, (struct sockaddr *) &hndl->raddr, addrLen) == SOCKET_ERROR) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "connect failed - %s", strerror (hndl->last_error));
        closesocket (testSocket);
        return false;
    }
    if (getsockname (testSocket, (struct sockaddr *) addr, &addrLen) == SOCKET_ERROR) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "getsockname failed - %s", strerror (hndl->last_error));
        closesocket (testSocket);
        return false;
    }
    closesocket (testSocket);
    um_log_print (hndl, 2, __PRETTY_FUNCTION__, "%s:%d", inet_ntoa (addr->sin_addr), ntohs(addr->sin_port));
    return true;
}

static bool udp_is_multicast_address(IPADDR *addr) {
    return ntohl(addr->sin_addr.s_addr) >> 24 == 224;
}

static bool udp_is_loopback_address(IPADDR *addr) {
    return ntohl(addr->sin_addr.s_addr) >> 24 == 127;
}

static bool udp_is_broadcast_address(IPADDR *addr) {
    return (ntohl(addr->sin_addr.s_addr) & 0xff) == 0xff;
}

static bool udp_init(um_state *hndl, const char *broadcast_address) {
    bool ok = true;
#ifdef _WINDOWS
    WSADATA wsaData;
    // Initialize winsocket
    if(WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "WSAStartup failed (%d)\n", hndl->last_error);
        return 0;
    }
#endif
    if ((hndl->socket = socket (AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "socket create failed - %s", strerror (hndl->last_error));
        ok = false;
    }
    if (ok && !udp_set_address (&hndl->raddr, broadcast_address ? broadcast_address : LIBUM_DEF_BCAST_ADDRESS)) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "invalid remote address - %s\n", strerror (hndl->last_error));
        ok = false;
    }
    if (ok && !udp_set_address (&hndl->laddr, LIBUM_ANY_IPV4_ADDR)) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "invalid local address - %s\n", strerror (hndl->last_error));
        ok = false;
    }

    // Dynamic port used in windows by default
    if (!hndl->local_port) {
        hndl->laddr.sin_port = 0;
        // change local port to avoid conflict on localhost testing
    } else if (udp_is_loopback_address (&hndl->raddr)) {
        hndl->laddr.sin_port = htons(hndl->local_port - 2);
    } else {
        hndl->laddr.sin_port = htons(hndl->local_port);
    }

    hndl->raddr.sin_port = htons(hndl->udp_port);
    if (ok) {
        ok = udp_set_sock_opt_addr_reuse (hndl);
    }
#ifndef NO_UDP_MULTICAST
    if (ok && udp_is_multicast_address (&hndl->raddr)) {
        ok = udp_set_sock_opt_mcast_group (hndl, &hndl->raddr);
    }
#endif
    if (ok && udp_is_broadcast_address (&hndl->raddr)) {
        ok = udp_set_sock_opt_bcast (hndl);
    }

    int i, ret = 0;
    for (i = 0; ok && i < 2 &&
                (ret = bind (hndl->socket, (struct sockaddr *) &hndl->laddr, sizeof (IPADDR))) == SOCKET_ERROR; i++) {
        hndl->last_os_errno = getLastError();
        if (hndl->last_os_errno == EADDRINUSE) {
            // Attempt bind dynamic port
            hndl->laddr.sin_port = 0;
            continue;
        }
        sprintf(hndl->errorstr_buffer, "bind failed - %s\n", strerror (hndl->last_error));
        ok = false;
    }
#ifdef _WINDOWS
    if(!ok)
        WSACleanup();
#endif
    return ok && ret >= 0;
}

static int set_last_error(um_state *hndl, int code) {
    char *txt;
    if (hndl) {
        hndl->last_error = code;
        const char *txt = um_errorstr (code);
        if (txt) {
            strcpy(hndl->errorstr_buffer, txt);
        }
    }
    return code;
}

static bool um_resolve_dev_ip_address(um_state *hndl, const int dev, IPADDR *to) {
    if (dev < SMCP1_DIRECT_ADDRESS_LIMIT) {
        return false;
    }
    uint32_t addr = LINK_LOCAL_IPV4_NET | (dev & 0xffff);
    to->sin_family = AF_INET;
    to->sin_port = htons(hndl->udp_port);
    to->sin_addr.s_addr = htonl(addr);
    return true;
}

int um_has_unicast_address(um_state *hndl, const int dev) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    int dev_id = um_resolve_dev_id (dev);
    return hndl->addresses[dev_id].sin_addr.s_addr != 0;
}

static int um_send(um_state *hndl, const int dev, const unsigned char *data, int dataSize) {
    int ret;
    IPADDR to;

    if (dev > 0 && dev < LIBUM_MAX_DEVS && hndl->addresses[dev].sin_port && hndl->addresses[dev].sin_family)
        memcpy(&to, &hndl->addresses[dev], sizeof (IPADDR));
    else if (!um_resolve_dev_ip_address (hndl, dev, &to))
        memcpy(&to, &hndl->raddr, sizeof (IPADDR));

    if (hndl->verbose > 1) {
        int i;
        smcp1_frame *header = (smcp1_frame *) data;
        smcp1_subblock_header *sub_block = (smcp1_subblock_header *) (((unsigned char *) data) + SMCP1_FRAME_SIZE);
        int32_t *data_ptr = (int32_t *) (data) + (SMCP1_FRAME_SIZE + SMCP1_SUB_BLOCK_HEADER_SIZE) / sizeof (int32_t);

        um_log_print (hndl, 2, __PRETTY_FUNCTION__,
                      "type %d id %d sender %d receiver %d blocks %d options 0x%02X to %s:%d", ntohs(header->type),
                      ntohs(header->message_id), ntohs(header->sender_id), ntohs(header->receiver_id),
                      ntohs(header->sub_blocks), (int) ntohl(header->options), inet_ntoa (to.sin_addr),
                      ntohs(to.sin_port));
        if (ntohs(header->sub_blocks)) {
            int data_size = ntohs(sub_block->data_size);
            um_log_print (hndl, 3, __PRETTY_FUNCTION__, "sub block size %d type %d", data_size,
                          ntohs(sub_block->data_type));
            for (i = 0; i < data_size; i++, data_ptr++)
                um_log_print (hndl, 3, __PRETTY_FUNCTION__, " arg%d: %d (0x%02X)%c", i + 1, (int) ntohl(*data_ptr),
                              (int) ntohl(*data_ptr), i < data_size - 1 ? ',' : ' ');
        }
    }

    if ((ret = sendto (hndl->socket, (char *) data, dataSize, 0, (struct sockaddr *) &to, sizeof (IPADDR))) ==
        SOCKET_ERROR) {
        hndl->last_os_errno = getLastError();
        sprintf(hndl->errorstr_buffer, "sendto failed - %s\n", strerror (hndl->last_os_errno));
        return set_last_error (hndl, LIBUM_OS_ERROR);
    }
    hndl->last_msg_ts[dev] = um_get_timestamp_ms ();
    return ret;
}

um_state *um_open(const char *udp_target_address, const unsigned int timeout, const int group) {
    int i;
    um_state *hndl;
    if (group < SMCP1_DEF_UDP_PORT && (group < 0 || group > 10)) {
        // LIBUM_INVALID_ARG
        return NULL;
    }
    if (group > SMCP1_DEF_UDP_PORT + 10) {
        // LIBUM_INVALID_ARG
        return NULL;
    }
    if (timeout > LIBUM_MAX_TIMEOUT) {
        // LIBUM_INVALID_ARG);
        return NULL;
    }
    if (!(hndl = malloc (sizeof (um_state)))) {
        return NULL;
    }
    memset(hndl, 0, sizeof (um_state));
    hndl->socket = INVALID_SOCKET;
// Use dynamic local port 0 in windows unless explicitly requested with UDP port number as group
#ifdef _WINDOWS
    if(group >= SMCP1_DEF_UDP_PORT)
        hndl->udp_port = hndl->local_port = group;
    else
        hndl->udp_port = SMCP1_DEF_UDP_PORT + group;
#else
    // In linux ports need to symmetric. On the other hand multiple applications can share the same port.
    if (group >= SMCP1_DEF_UDP_PORT) {
        hndl->local_port = hndl->udp_port = group;
    } else {
        hndl->local_port = hndl->udp_port = SMCP1_DEF_UDP_PORT + group;
    }
#endif
    hndl->retransmit_count = 3;
    hndl->refresh_time_limit = LIBUM_DEF_REFRESH_TIME;
    hndl->timeout = timeout;
    for (i = 0; i < LIBUM_MAX_DEVS; i++) {
        hndl->last_positions[i].x = SMCP1_ARG_UNDEF;
        hndl->last_positions[i].y = SMCP1_ARG_UNDEF;
        hndl->last_positions[i].z = SMCP1_ARG_UNDEF;
        hndl->last_positions[i].d = SMCP1_ARG_UNDEF;
    }

    hndl->own_id = SMCP1_ALL_PCS - 100 - (um_get_timestamp_us () & 100);
    hndl->timeout = timeout;

    if (!udp_init (hndl, udp_target_address)) {
        free (hndl);
        return NULL;
    }
    return hndl;
}

void um_close(um_state *hndl) {
    if (!hndl) {
        return;
    }
    if (hndl->socket != INVALID_SOCKET) {
        closesocket (hndl->socket);
#ifdef _WINDOWS
        WSACleanup();
#endif
    }
    free (hndl);
}

int um_set_timeout(um_state *hndl, const int value) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (value < 0 || value > LIBUM_MAX_TIMEOUT) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    hndl->timeout = value;
    return 0;
}

int um_set_log_func(um_state *hndl, const int verbose, um_log_print_func func, const void *arg) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (verbose < 0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    hndl->verbose = verbose;
    hndl->log_func_ptr = func;
    hndl->log_print_arg = arg;
    return 0;
}

int um_set_refresh_time_limit(um_state *hndl, const int value) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (value < LIBUM_TIMELIMIT_DISABLED || value > 60000) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    hndl->refresh_time_limit = value;
    return 0;
}

int um_is_busy_status(const um_status status) {
    if (status < 0) {
        return status;
    }
    if (status & 0xfff1) {
        return 1;
    }
    return 0;
}

int um_get_status(um_state *hndl, const int dev) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    int dev_id = um_resolve_dev_id (dev);
    return hndl->last_status[dev_id];
}

int um_is_busy(um_state *hndl, const int dev) {
    int status = um_get_status (hndl, dev);
    return um_is_busy_status (status);
}

int um_get_drive_status(um_state *hndl, const int dev) {
    int drive_status, pwm_status;
    unsigned long long ts, now;

    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    int dev_id = um_resolve_dev_id (dev);
    drive_status = hndl->drive_status[dev_id];
    pwm_status = hndl->last_status[dev_id];
    ts = hndl->drive_status_ts[dev_id];
    now = um_get_timestamp_ms ();

    // Special handling for stuck drive status.
    // If drive status is busy, but pwm status not and 1s elapsed since it was last time,
    // assume drive status notification to be lost and set drive status to completed.
    if (ts && drive_status == LIBUM_POS_DRIVE_BUSY && !um_is_busy_status (pwm_status) && now - ts > 1000) {
        hndl->drive_status[dev_id] = LIBUM_POS_DRIVE_COMPLETED;
        um_log_print (hndl, 1, __PRETTY_FUNCTION__, "Stuck dev %d drive status, PWM was on %1.1fs ago", dev,
                      (float) (now - ts) / 1000.0);
    }
    // update last pwm busy time
    if (um_is_busy_status (pwm_status)) {
        hndl->drive_status_ts[dev_id] = now;
    }
    return hndl->drive_status[dev_id];
}

static int um_set_drive_status(um_state *hndl, const int dev, const int value) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    int dev_id = um_resolve_dev_id (dev);
    hndl->drive_status[dev_id] = value;
    hndl->drive_status_ts[dev_id] = um_get_timestamp_ms ();
    return 0;
}


#if defined(_WINDOWS) || (defined(__APPLE__) && defined(__MACH__))

static int isnanf(const float arg) { return isnan(arg); }

#endif

static bool um_arg_undef(const float arg) { return isnanf (arg) || arg == SMCP1_ARG_UNDEF || arg == INT32_MIN; }

static bool um_invalid_pos(const float pos) { return (pos < -1000 || pos > LIBUM_MAX_POSITION) && !um_arg_undef (pos); }

static int um2nm(const float um) { return (int) (um * 1000.0); }

static float nm2um(const int nm) { return (float) nm / 1000.0f; }

int um_cmd(um_state *hndl, const int dev, const int cmd, const int argc, const int *argv) {
    return um_send_msg (hndl, dev, cmd, argc, argv, 0, NULL, 0, NULL);
}

int
um_cmd_ext(um_state *hndl, const int dev, const int cmd, const int argc, const int *argv, int respsize, int *response) {
    return um_send_msg (hndl, dev, cmd, argc, argv, 0, NULL, respsize, response);
}

int um_init_zero(um_state *hndl, const int dev, const int axis_mask) {
    return um_cmd (hndl, dev, SMCP1_CMD_INIT_ZERO, axis_mask ? 1 : 0, &axis_mask);
}

int um_save_zero(um_state *hndl, const int dev) {
    return um_cmd (hndl, dev, SMCP1_CMD_SAVE_ZERO, 0, NULL);
}

int ump_calibrate_load(um_state *hndl, const int dev) {
    int arg = 0;
    return um_cmd (hndl, dev, SMCP1_CMD_CALIBRATE, 1, &arg);
}

int ump_led_control(um_state *hndl, const int dev, const int off) {
    int ret, arg = 0;
    if (off < 0 || off > 1) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if ((ret = um_set_feature (hndl, dev, SMCP10_FEAT_PREVENT_MOVEMENT, off)) < 0) {
        return ret;
    }
    if (off) {
        return um_cmd (hndl, dev, SMCP1_CMD_SLEEP, 1, &arg);
    }
    return um_cmd (hndl, dev, SMCP1_CMD_WAKEUP, 0, NULL);
}

static int calc_speed(const float speed) {
    if (speed < 1.0) {
        return (int) (speed * (-1000.0));
    }
    return (int) speed;
}

int um_goto_position(um_state *hndl, const int dev, const float x, const float y, const float z, const float d,
                     const float speed, const int mode, const int max_acc) {
    int ret, args[7], argc = 0;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if (um_invalid_pos (x) || um_invalid_pos (y) || um_invalid_pos (z) || um_invalid_pos (d)) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (speed < 0.0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }

    args[argc++] = um_arg_undef (x) ? SMCP1_ARG_UNDEF : um2nm (x);
    args[argc++] = um_arg_undef (y) ? SMCP1_ARG_UNDEF : um2nm (y);
    args[argc++] = um_arg_undef (z) ? SMCP1_ARG_UNDEF : um2nm (z);
    if (!um_arg_undef (d) || speed || mode) {
        args[argc++] = um_arg_undef (d) ? SMCP1_ARG_UNDEF : um2nm (d);
    }
    if (speed || mode || max_acc) {
        args[argc++] = calc_speed (speed);
    }
    if (mode || max_acc) {
        args[argc++] = mode;
    }
    if (max_acc) {
        args[argc++] = max_acc;
    }
    ret = um_cmd (hndl, dev, SMCP1_CMD_GOTO_POS, argc, args);
    um_set_drive_status (hndl, dev, ret >= 0 ? LIBUM_POS_DRIVE_BUSY : LIBUM_POS_DRIVE_FAILED);
    return ret;
}

static float get_max_speed(const float X, const float Y, const float Z, const float D) {
    float ret = X;
    if (Y > ret) {
        ret = Y;
    }
    if (Z > ret) {
        ret = Z;
    }
    if (D > ret) {
        ret = D;
    }
    return ret;
}

int um_goto_position_ext(um_state *hndl, const int dev, const float x, const float y, const float z, const float d,
                         const float speedX, const float speedY, const float speedZ, const float speedD, const int mode,
                         const int max_acc) {
    int ret, args[7], args2[4], argc = 0, argc2 = 0;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if (um_invalid_pos (x) || um_invalid_pos (y) || um_invalid_pos (z) || um_invalid_pos (d)) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (!um_arg_undef (x) && speedX <= 0.0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (!um_arg_undef (y) && speedY <= 0.0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (!um_arg_undef (z) && speedZ <= 0.0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (!um_arg_undef (d) && speedD <= 0.0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }

    args[argc++] = um_arg_undef (x) ? SMCP1_ARG_UNDEF : um2nm (x);
    args[argc++] = um_arg_undef (y) ? SMCP1_ARG_UNDEF : um2nm (y);
    args[argc++] = um_arg_undef (z) ? SMCP1_ARG_UNDEF : um2nm (z);
    args[argc++] = um_arg_undef (d) ? SMCP1_ARG_UNDEF : um2nm (d);
    // backward compatibility trick for uMs or uMp not supporting second sub block,
    // but just one speed argument shared by all axis
    args[argc++] = calc_speed (get_max_speed (speedX, speedY, speedZ, speedD));
    if (mode || max_acc) {
        args[argc++] = mode;
    }
    if (max_acc) {
        args[argc++] = max_acc;
    }
    if (!um_arg_undef (x) || !um_arg_undef (y) || !um_arg_undef (z) || !um_arg_undef (d)) {
        args2[argc2++] = calc_speed (speedX);
    }
    if (!um_arg_undef (y) || !um_arg_undef (z) || !um_arg_undef (d)) {
        args2[argc2++] = calc_speed (speedY);
    }
    if (!um_arg_undef (z) || !um_arg_undef (d)) {
        args2[argc2++] = calc_speed (speedZ);
    }
    if (!um_arg_undef (d)) {
        args2[argc2++] = calc_speed (speedD);
    }
    ret = um_send_msg (hndl, dev, SMCP1_CMD_GOTO_POS, argc, args, argc2, args2, 0, NULL);
    um_set_drive_status (hndl, dev, ret >= 0 ? LIBUM_POS_DRIVE_BUSY : LIBUM_POS_DRIVE_FAILED);
    return ret;
}

float um_get_speed(um_state *hndl, const int dev, const char axis) {
    if (!hndl || is_invalid_dev (dev)) {
        return 0.0;
    }
    int dev_id = um_resolve_dev_id (dev);
    um_positions *positions = &hndl->last_positions[dev_id];
    if (!positions->updated_us) {
        return 0.0;
    }
    switch (axis) {
        case 'x':
        case 'X':
            return positions->speed_x;
        case 'y':
        case 'Y':
            return positions->speed_y;
        case 'z':
        case 'Z':
            return positions->speed_z;
        case 'w':
        case 'W':
        case '4':
            return positions->speed_d;
    }
    return 0;
}

float um_get_position(um_state *hndl, const int dev, const char axis) {
    if (!hndl || is_invalid_dev (dev)) {
        return 0;
    }
    int dev_id = um_resolve_dev_id (dev);

    um_positions *positions = &hndl->last_positions[dev_id];
    if (!positions->updated_us) {
        return 0.0;
    }
    switch (axis) {
        case 'x':
        case 'X':
            if (positions->x == (float) SMCP1_ARG_UNDEF) {
                return 0.0;
            }
            return nm2um (positions->x);
        case 'y':
        case 'Y':
            if (positions->y == (float) SMCP1_ARG_UNDEF) {
                return 0.0;
            }
            return nm2um (positions->y);
        case 'z':
        case 'Z':
            if (positions->z == (float) SMCP1_ARG_UNDEF) {
                return 0.0;
            }
            return nm2um (positions->z);
        case 'w':
        case 'W':
        case 'd':
        case 'D':
        case '4':
            if (positions->d == SMCP1_ARG_UNDEF) {
                return 0.0;
            }
            return nm2um (positions->d);
    }
    return 0;
}

int um_stop(um_state *hndl, const int dev) {
    return um_cmd (hndl, dev, SMCP1_CMD_STOP, 0, NULL);
}

int um_stop_all(um_state *hndl) {
    return um_stop (hndl, SMCP1_ALL_DEVICES);
}

static int um_update_position_cache_time(um_state *hndl, const int sender_id) {
    int ret = 0;
    um_positions *positions = &hndl->last_positions[sender_id];
    unsigned long long ts_us = um_get_timestamp_us ();
    if (positions->updated_us) {
        ret = (int) (ts_us - positions->updated_us);
    }
    positions->updated_us = ts_us;
    return ret;
}

static int um_update_positions_cache(um_state *hndl, const int sender_id, const int axis_index, const int pos_nm,
                                     const int time_step_us) {
    um_positions *positions = &hndl->last_positions[sender_id];
    int *pos_ptr = NULL;
    float *speed_ptr = NULL;
    int step_nm;

    switch (axis_index) {
        case 0:
            pos_ptr = &positions->x;
            speed_ptr = &positions->speed_x;
            break;
        case 1:
            pos_ptr = &positions->y;
            speed_ptr = &positions->speed_y;
            break;
        case 2:
            pos_ptr = &positions->z;
            speed_ptr = &positions->speed_z;
            break;
        case 3:
            pos_ptr = &positions->d;
            speed_ptr = &positions->speed_d;
            break;
    }
    if (!pos_ptr) {
        return -1;
    }
    step_nm = pos_nm - *pos_ptr;
    *pos_ptr = pos_nm;
    if (time_step_us > 0) {
        *speed_ptr = (float) step_nm * 1000.0f / (float) time_step_us;
    } else {
        *speed_ptr = 0.0;
    }
    return axis_index;
}

int um_set_slow_speed_mode(um_state *hndl, const int dev, const int activated) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    return um_set_ext_feature (hndl, dev, SMCP10_EXT_FEAT_CUST_LOW_SPEED, activated);
}

int um_get_slow_speed_mode(um_state *hndl, const int dev) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    return um_get_ext_feature (hndl, dev, SMCP10_EXT_FEAT_CUST_LOW_SPEED);
}

int um_set_soft_start_mode(um_state *hndl, const int dev, const int activated) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    return um_set_ext_feature (hndl, dev, SMCP10_EXT_FEAT_SOFT_START, activated);
}

int um_get_soft_start_mode(um_state *hndl, const int dev) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    return um_get_ext_feature (hndl, dev, SMCP10_EXT_FEAT_SOFT_START);
}

#define UMP_RECEIVE_ACK_GOT  1
#define UMP_RECEIVE_RESP_GOT 2

int um_recv_ext(um_state *hndl, um_message *msg, int *ext_data_type, void *ext_data_ptr, const int timeout) {
    IPADDR from;
    int receiver_id, sender_id, message_id, type, sub_blocks, data_size = 0, data_type = SMCP1_DATA_VOID, options, status;
    int i, data_type2, data_size2, pos_nm, time_step_us = 0, ext_data_size = 0, ret = 0;
    uint32_t value;
    uint32_t *ext_data = (uint32_t *) ext_data_ptr;
    smcp1_frame *header = (smcp1_frame *) msg;
    smcp1_subblock_header *sub_block2, *sub_block = (smcp1_subblock_header *) ((unsigned char *) msg +
                                                                               SMCP1_FRAME_SIZE);

    int32_t *data2_ptr, *data_ptr =
            (int32_t *) (msg) + (SMCP1_FRAME_SIZE + SMCP1_SUB_BLOCK_HEADER_SIZE) / sizeof (int32_t);

    um_positions *positions;
    smcp1_frame ack;

    if (ext_data_type != NULL) {
        *ext_data_type = -1;
    }

    if (!hndl || hndl->socket == INVALID_SOCKET) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (!msg) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }

    memset(msg, 0, sizeof (um_message));

    if ((ret = udp_recv (hndl, (unsigned char *) msg, sizeof (um_message), &from, timeout)) < 1) {
        if (!ret) {
            return set_last_error (hndl, LIBUM_TIMEOUT);
        }
        return set_last_error (hndl, LIBUM_OS_ERROR);
    }

    if (ret < (int) SMCP1_FRAME_SIZE) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    if (header->version != SMCP1_VERSION) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    receiver_id = ntohs(header->receiver_id);
    sender_id = ntohs(header->sender_id);
    options = ntohl(header->options);
    type = ntohs(header->type);
    message_id = ntohs(header->message_id);
    sub_blocks = ntohs(header->sub_blocks);

    // sender_dev_id is serial number in direct address mode, while sender_id is in SMCPv1 format and thus 16bit
    int sender_dev_id = sender_id;
    um_resolve_sno (sender_id, &sender_dev_id);

    um_log_print (hndl, 3, __PRETTY_FUNCTION__, "type %d id %d sender %d/%d receiver %d options 0x%02X from %s:%d",
                  type, message_id, sender_id, sender_dev_id, receiver_id, options, inet_ntoa (from.sin_addr),
                  ntohs(from.sin_port));
    // Cache is now 64K long and thus any sender id is in the cache
    memcpy(&hndl->addresses[sender_id], &from, sizeof (IPADDR));

    // Filter messages by receiver id, level 1, include broadcasts
    if (receiver_id != SMCP1_ALL_CUS && receiver_id != SMCP1_ALL_PCS && receiver_id != SMCP1_ALL_CUS_OR_PCS &&
        receiver_id != hndl->own_id) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }

    // Notifications, handles also broadcasted ones
    if (sub_blocks > 0 && options & SMCP1_OPT_NOTIFY && is_valid_dev (sender_dev_id)) {
        data_size = ntohs(sub_block->data_size);
        data_type = ntohs(sub_block->data_type);

        switch (type) {
            case SMCP1_NOTIFY_POSITION_CHANGED:
                if (data_size > 0 && (data_type == SMCP1_DATA_INT32 || data_type == SMCP1_DATA_UINT32)) {
                    positions = &hndl->last_positions[sender_id];
                    time_step_us = um_update_position_cache_time (hndl, sender_id);
                    // X axis
                    pos_nm = ntohl(*data_ptr++);
                    um_update_positions_cache (hndl, sender_id, 0, pos_nm, time_step_us);
                    if (data_size > 1) {
                        pos_nm = ntohl(*data_ptr++);
                        um_update_positions_cache (hndl, sender_id, 1, pos_nm, time_step_us);
                    }
                    if (data_size > 2) {
                        pos_nm = ntohl(*data_ptr++);
                        um_update_positions_cache (hndl, sender_id, 2, pos_nm, time_step_us);
                    }
                    if (data_size > 3) {
                        pos_nm = ntohl(*data_ptr++);
                        um_update_positions_cache (hndl, sender_id, 3, pos_nm, time_step_us);
                    }
                    um_log_print (hndl, 2, __PRETTY_FUNCTION__,
                                  "dev %d updated %d position%s %1.3f %1.3f %1.3f %1.3f speeds %1.1f %1.1f %1.1f %1.1fum/s",
                                  sender_id, data_size, data_size > 1 ? "s" : "", nm2um (positions->x),
                                  nm2um (positions->y), nm2um (positions->z), nm2um (positions->d), positions->speed_x,
                                  positions->speed_y, positions->speed_z, positions->speed_d);
                } else {
                    um_log_print (hndl, 2, __PRETTY_FUNCTION__, "unexpected data type %d or size %d for positions",
                                  data_size, ntohs(sub_block->data_type));
                }
                break;
            case SMCP1_NOTIFY_STATUS_CHANGED:
                if (data_size > 0 && (data_type == SMCP1_DATA_INT32 || data_type == SMCP1_DATA_UINT32)) {
                    hndl->last_status[sender_id] = status = ntohl(*data_ptr);
                    um_log_print (hndl, 2, __PRETTY_FUNCTION__, "dev %d updated status %d (0x%08X)", sender_id, status,
                                  status);
                }
                break;
            case SMCP1_NOTIFY_GOTO_POS_COMPLETED:
                if (data_size > 0 && (data_type == SMCP1_DATA_INT32 || data_type == SMCP1_DATA_UINT32)) {
                    status = ntohl(*data_ptr);
                    if (message_id != hndl->drive_status_id[sender_id]) {
                        if (status == 0 ||
                            status == 2) { // Non-zero "not found" erro code at the end of memory position drive
                            hndl->drive_status[sender_id] = LIBUM_POS_DRIVE_COMPLETED;
                        } else {
                            hndl->drive_status[sender_id] = LIBUM_POS_DRIVE_FAILED;
                        }
                        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "dev %d updated drive status %d msg id %d",
                                      sender_id, status, message_id);
                        hndl->drive_status_id[sender_id] = message_id;
                    } else {
                        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "dev %d duplicated drive status %d msg id %d",
                                      sender_id, status, message_id);
                    }
                }
                break;
            case SMCP1_NOTIFY_UMA_SAMPLES:
                if (data_size > 0 && (data_type == SMCP1_DATA_INT32 || data_type == SMCP1_DATA_UINT32) &&
                    ext_data_type != NULL) {
                    *ext_data_type = SMCP1_NOTIFY_UMA_SAMPLES;
                    ext_data_size = data_size * sizeof (uint32_t);
                    return ext_data_size;
                }
                break;

            case SMCP1_VERSION:
            case SMCP1_GET_VERSION:
                um_log_print (hndl, 2, __PRETTY_FUNCTION__, "Version returned", __PRETTY_FUNCTION__);
                break;
            case SMCP1_NOTIFY_CALIBRATE_COMPLETED:
                break;
            case SMCP1_NOTIFY_PRESSURE_CHANGED:
                if (data_size > 0 && (data_type == SMCP1_DATA_INT32 || data_type == SMCP1_DATA_UINT32)) {
                    status = ntohl(*data_ptr);
                } else {
                    status = -1;
                }
                um_log_print (hndl, 2, __PRETTY_FUNCTION__,
                              "Pressure changed notification from %d/%d, %d channel%s, valves 0x%02x", sender_id,
                              sender_dev_id, data_size - 1, data_size - 1 > 1 ? "s" : "", status);
                break;
            default:
                um_log_print (hndl, 2, __PRETTY_FUNCTION__, "unsupported notification type %d ignored", type);
        }
    }

    // Send ACK if it's requested
    if (options & SMCP1_OPT_REQ_ACK &&
        (receiver_id == hndl->own_id || receiver_id == SMCP1_ALL_CUS || receiver_id == SMCP1_ALL_PCS)) {
        um_log_print (hndl, 3, __PRETTY_FUNCTION__, "Sending ACK to %d id %d", type, message_id);
        // type and message id copied from request

        memcpy(&ack, msg, sizeof (ack));
        ack.sender_id = ntohs(hndl->own_id);
        ack.receiver_id = header->sender_id;
        ack.options = htonl(SMCP1_OPT_ACK);
        ack.sub_blocks = 0;
        um_send (hndl, sender_id, (unsigned char *) &ack, sizeof (ack));
    }

    // uMa samples
    if (ext_data_type != NULL && *ext_data_type == SMCP1_NOTIFY_UMA_SAMPLES && ext_data_size) {
        if (ext_data_ptr) {
            memcpy(ext_data_ptr, data_ptr, ext_data_size);
        }
        return ext_data_size;
    }

    if (sub_blocks > 1 && ext_data_type != NULL && *ext_data_type >= 0) {
        sub_block2 = (smcp1_subblock_header *) ((unsigned char *) msg + SMCP1_FRAME_SIZE + SMCP1_SUB_BLOCK_HEADER_SIZE +
                                                data_size * sizeof (uint32_t));

        data_size2 = ntohs(sub_block2->data_size);
        data_type2 = ntohs(sub_block2->data_type);

        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "ext data type %d, %d item%s", *ext_data_type, data_size2,
                      data_size2 > 1 ? "s" : "");

        if ((data_type == SMCP1_DATA_INT32 || data_type == SMCP1_DATA_UINT32) &&
            (data_type2 == SMCP1_DATA_INT32 || data_type2 == SMCP1_DATA_UINT32)) {
            ext_data_size = data_size2;
            data2_ptr = data_ptr + data_size + (SMCP1_SUB_BLOCK_HEADER_SIZE) / sizeof (int32_t);
            for (i = 0; i < data_size2; i++) {
                value = ntohl(*data2_ptr++);
                if (i == 0 || i == 1 || i == data_size2 - 2 || i == data_size2 - 1) {
                    um_log_print (hndl, 3, __PRETTY_FUNCTION__, "ext_data[%d]\t0x%08x", i, value);
                }
                if (ext_data != NULL) {
                    ext_data[i] = value;
                }
            }
        } else {
            um_log_print (hndl, 2, __PRETTY_FUNCTION__, "unsupported ext data format %d", data_type2);
        }
        return ext_data_size;
    }

    // For responses or ACKs to our own request, accept only messages sent to our own id
    if (receiver_id != hndl->own_id) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    hndl->last_device_received = sender_id;

    // ACK sent to our own message
    if (options & SMCP1_OPT_ACK) {
        // ACK to our latest request
        if (message_id == hndl->message_id) {
            um_log_print (hndl, 3, __PRETTY_FUNCTION__, "ACK to %d request %d", type, message_id);
            return UMP_RECEIVE_ACK_GOT;
        }
        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "ACK to %d id %d while %d expected", type, message_id,
                      hndl->message_id);
        return 0;
    }

    if (!(options & SMCP1_OPT_REQ)) {
        // Response to our own request
        if (message_id == hndl->message_id) {
            um_log_print (hndl, 3, __PRETTY_FUNCTION__, "response to %d request %d", type, message_id);
            return UMP_RECEIVE_RESP_GOT;
        }
        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "response to %d id %d while %d expected", type, message_id,
                      hndl->message_id);
        return 0;
    }

    if (options & SMCP1_OPT_REQ) // request to us
    {
        // TODO request to us - at least ping or version query?
        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "unsupported request type %d", type);
    }

    if (options & SMCP1_OPT_ERROR) {
        return set_last_error (hndl, LIBUM_PEER_ERROR);
    }
    return 0;
}

int um_recv(um_state *hndl, um_message *msg) { return um_recv_ext (hndl, msg, NULL, NULL, hndl->timeout); }

int um_receive(um_state *hndl, const int timelimit) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    int dev, ret, count = 0;
    um_message resp;
    unsigned long long now = um_get_timestamp_ms ();

    if (!timelimit) {
        do {
            if ((ret = um_recv_ext (hndl, &resp, NULL, NULL, 0)) >= 0 || ret == LIBUM_INVALID_DEV) {
                count++;
            }
        } while (ret >= 0 || ret == LIBUM_INVALID_DEV);
    } else {
        do {
            if ((ret = um_recv (hndl, &resp)) >= 0) {
                count++;
            } else if (ret < 0 && ret != LIBUM_TIMEOUT && ret != LIBUM_INVALID_DEV) {
                return ret;
            }
        } while ((int) get_elapsed (now) < timelimit);
    }

    for (dev = 1; dev < LIBUM_MAX_DEVS; dev++) {
        unsigned long long ts = hndl->last_msg_ts[dev];
        if (ts && hndl->addresses[dev].sin_family && now - ts > 30000) {
            if (um_cmd (hndl, dev, SMCP1_CMD_PING, 0, NULL) < 0) {
                memset(&hndl->addresses[dev], 0, sizeof (IPADDR));
                hndl->last_msg_ts[dev] = 0;
            }
        }
    }

    return count;
}

int um_ping(um_state *hndl, const int dev) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    int ret, dev_id = um_resolve_dev_id (dev);
    if ((ret = um_cmd (hndl, dev_id, SMCP1_CMD_PING, 0, NULL)) < 0) {
        return ret;
    }
    hndl->last_device_sent = dev;
    return ret;
}

void swap_byte_order(unsigned char *data) {
    unsigned char *other = data;
    int i, count = sizeof (data);
    for (i = 0; i < count; i++)
        *data++ = other[count - (i + 1)];
}

int um_cmd_options(um_state *hndl, const int optionbits) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (optionbits) {
        hndl->next_cmd_options |= optionbits;
    } else {
        hndl->next_cmd_options = 0;
    }
    return hndl->next_cmd_options;
}

static int um_send_msg(um_state *hndl, const int dev, const int cmd, const int argc, const int *argv, const int argc2,
                       const int *argv2, // optional second subblock
                       const int respc, int *respv) {
    int i, j, resp_data_size, resp_data_type, ret = 0;
    int options = SMCP1_OPT_REQ, req_size = SMCP1_FRAME_SIZE;
    um_message req, resp;
    smcp1_frame *req_header = (smcp1_frame *) &req;
    smcp1_frame *resp_header = (smcp1_frame *) &resp;
    smcp1_subblock_header *req_sub_header = (smcp1_subblock_header *) (((unsigned char *) &req) + SMCP1_FRAME_SIZE);
    smcp1_subblock_header *req_sub_header2 = (smcp1_subblock_header *) (((unsigned char *) &req) + SMCP1_FRAME_SIZE +
                                                                        SMCP1_SUB_BLOCK_HEADER_SIZE +
                                                                        argc * sizeof (int32_t));
    int32_t *req_data_ptr = (int32_t *) (&req) + (SMCP1_FRAME_SIZE + SMCP1_SUB_BLOCK_HEADER_SIZE) / sizeof (int32_t);
    int32_t *req_data_ptr2 =
            (int32_t *) (&req) + (SMCP1_FRAME_SIZE + 2 * SMCP1_SUB_BLOCK_HEADER_SIZE) / sizeof (int32_t) + argc;

    smcp1_subblock_header *resp_sub_header = (smcp1_subblock_header *) (((unsigned char *) &resp) + SMCP1_FRAME_SIZE);
    int32_t *resp_data_ptr = (int32_t *) (&resp) + (SMCP1_FRAME_SIZE + SMCP1_SUB_BLOCK_HEADER_SIZE) / sizeof (int32_t);

    unsigned long long start;
    bool ack_received = false, ack_requested = false;
    bool resp_option_requested = false;

    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }

    int dev_id = um_resolve_dev_id (dev);

    memset(&req, 0, sizeof (req));
    memset(&resp, 0, sizeof (resp));
    req_header->version = SMCP1_VERSION;
    req_header->sender_id = htons(hndl->own_id);
    req_header->receiver_id = htons(dev_id);
    req_header->type = htons(cmd);
    req_header->message_id = htons(++hndl->message_id);

    if (dev != SMCP1_ALL && dev != SMCP1_ALL_DEVICES && dev != SMCP1_ALL_CUS && dev != SMCP1_ALL_OTHERS &&
        dev != SMCP1_ALL_PCS) {
        options |= SMCP1_OPT_REQ_ACK;
        ack_requested = true;
    }
    if (cmd == SMCP1_CMD_GOTO_MEM || cmd == SMCP1_CMD_GOTO_POS) {
        options |= SMCP1_OPT_REQ_NOTIFY;
    }
    // SDK emulating a device
    if (cmd == SMCP1_NOTIFY_POSITION_CHANGED || cmd == SMCP1_NOTIFY_STATUS_CHANGED) {
        options |= SMCP1_OPT_NOTIFY;
    }
    if (respc) {
        options |= SMCP1_OPT_REQ_RESP;
    }

    // If there are additional options set, use them
    if (hndl->next_cmd_options) {
        options |= hndl->next_cmd_options;
        if (options & SMCP1_OPT_REQ_RESP && !respc) {
            resp_option_requested = true;
        }
        if (options & SMCP1_OPT_REQ_ACK) {
            ack_requested = true;
        }
    }

    req_header->options = htonl(options);

    // Reset option flags.
    if (hndl->next_cmd_options) {
        hndl->next_cmd_options = 0;
    }

    if (argc > 0 && argv != NULL) {
        req_header->sub_blocks = htons(1);
        req_size += sizeof (smcp1_subblock_header) + argc * sizeof (int32_t);
        req_sub_header->data_type = htons(SMCP1_DATA_INT32);
        req_sub_header->data_size = htons(argc);
        for (j = 0; j < argc; j++)
            *req_data_ptr++ = htonl(*argv++);

        if (argc2 > 0 && argv2 != NULL) {
            req_header->sub_blocks = htons(2);
            req_size += sizeof (smcp1_subblock_header) + argc2 * sizeof (int32_t);
            req_sub_header2->data_type = htons(SMCP1_DATA_INT32);
            req_sub_header2->data_size = htons(argc2);

            for (j = 0; j < argc2; j++)
                *req_data_ptr2++ = htonl(*argv2++);
        }
    }

    // No ACK or RESP requested, just send the message
    if (!ack_requested && (!respc && !resp_option_requested)) {
        return um_send (hndl, dev_id, (unsigned char *) &req, req_size);
    }

    start = um_get_timestamp_ms ();
    for (i = 0; i < (ack_requested ? hndl->retransmit_count : 1); i++) {
        // Do not resend message if ACK was already got
        if (!ack_received && (ret = um_send (hndl, dev_id, (unsigned char *) &req, req_size)) < 0) {
            return ret;
        }
        while ((ret = um_recv (hndl, &resp)) >= 0 ||
               ((ret == LIBUM_TIMEOUT || ret == LIBUM_INVALID_DEV) && (int) get_elapsed (start) < hndl->timeout)) {
            um_log_print (hndl, 4, __PRETTY_FUNCTION__, "ret %d %dms left", ret,
                          (int) (hndl->timeout - get_elapsed (start)));
            if (ret == 1) {
                ack_received = true;
            }
            // If not expecting a response, getting ACK is enough.
            if ((!respc && !resp_option_requested) && ret == 1) {
                return 0;
            }
            // Expecting response
            if ((respc || resp_option_requested) && ret == 2) {
                // A notification may be received between the request and response,
                // a more pedantic response validation is needed.
                if (req_header->type != resp_header->type) {
                    continue;
                }
                if (req_header->message_id != resp_header->message_id) {
                    continue;
                }
                if (ntohs(resp_header->sub_blocks) < 1) {
                    if (ntohl(resp_header->options) & SMCP1_OPT_ERROR) {
                        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "peer error");
                        return set_last_error (hndl, LIBUM_PEER_ERROR);
                    } else {
                        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "empty response");
                        return set_last_error (hndl, LIBUM_INVALID_RESP);
                    }
                }
                resp_data_size = ntohs(resp_sub_header->data_size);
                resp_data_type = ntohs(resp_sub_header->data_type);
                um_log_print (hndl, 3, __PRETTY_FUNCTION__, "%d data item%s of type %d", resp_data_size,
                              resp_data_size > 1 ? "s" : "", resp_data_type);
                switch (resp_data_type) {
                    case SMCP1_DATA_UINT32:
                        for (j = 0; j < resp_data_size && j < respc; j++)
                            *respv++ = (int32_t) ntohl(*resp_data_ptr++);
                        break;
                    case SMCP1_DATA_INT32:
                        for (j = 0; j < resp_data_size && j < respc; j++)
                            *respv++ = ntohl(*resp_data_ptr++);
                        break;
                    case SMCP1_DATA_CHAR_STRING:
                        memcpy(respv, resp_data_ptr, resp_data_size);
                        //resp_data_size=1;
                        break;
                        //memcpy(respv, &resp, resp_data_size);
                        //resp_data_size=1;
                        //break;
                    default:
                        um_log_print (hndl, 2, __PRETTY_FUNCTION__, "unexpected data type %d", resp_data_type);
                        return set_last_error (hndl, LIBUM_INVALID_RESP);
                }
                return resp_data_size;
            }
        }
    }
    return ret;
}

int um_cmd_may_cause_movement(const int cmd) {
    switch (cmd) {
        case SMCP1_CMD_INIT_ZERO:
        case SMCP1_CMD_CALIBRATE:
        case SMCP1_CMD_DRIVE_LOOP:
        case SMCP1_CMD_GOTO_MEM:
        case SMCP1_CMD_GOTO_POS:
        case SMCP1_CMD_TAKE_STEP:
        case SMCP1_CMD_TAKE_LEGACY_STEP:
            return 1;
    }
    return 0;
}


int um_set_param(um_state *hndl, const int dev, const int param_id, const int value) {
    int args[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    args[0] = param_id;
    args[1] = value;
    return um_cmd (hndl, dev, SMCP1_SET_PARAMETER, 2, args);
}

int um_get_param(um_state *hndl, const int dev, const int param_id, int *value) {
    int ret, resp[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_GET_PARAMETER, 1, &param_id, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != param_id || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    *value = resp[1];
    return 1;
}


int um_set_feature(um_state *hndl, const int dev, const int feature_id, const int value) {
    int args[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    args[0] = feature_id;
    args[1] = value;
    return um_cmd (hndl, dev, SMCP1_SET_FEATURE, 2, args);
}

int um_set_ext_feature(um_state *hndl, const int dev, const int feature_id, const int value) {
    int args[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    args[0] = feature_id;
    args[1] = value;
    return um_cmd (hndl, dev, SMCP1_SET_EXT_FEATURE, 2, args);
}

int ump_get_axis_angle(um_state *hndl, const int dev, float *value) {
    int ret, args[2];
    int resp, axis_count;
    if ((axis_count = um_get_axis_count (hndl, dev)) < 0) {
        return axis_count;
    }
    args[0] = (axis_count == 4) ? 3 : 0;
    args[1] = 1;
    if ((ret = um_send_msg (hndl, dev, SMCP1_CMD_GET_AXIS_ANGLE, 2, args, 0, NULL, 1, &resp)) < 0) {
        return ret;
    }
    if (value) {
        *value = (float) resp / 10.0f;
    }
    return resp;
}

int ump_get_handedness_configuration(um_state *hndl, const int dev) {
    int config;
    int resp = um_get_param (hndl, dev, SMCP1_PARAM_AXIS_HEAD_CONFIGURATION, &config);
    if (resp >= 0) {
        resp = config & (1 << 1) ? 1 : 0;
    }
    return resp;
}

static int
ump_resolve_cls_mode(const float step_x, const float step_y, const float step_z, const float step_w, const int speed_x,
                     const int speed_y, const int speed_z, const int speed_w) {
    // CLS MODE SELECTION
    int smallest_speed = 1000;
    int cls_mode = 0;

    if (step_x != 0.0 && speed_x > 0 && speed_x != SMCP1_ARG_UNDEF) {
        smallest_speed = speed_x;
    }
    if (step_y != 0.0 && speed_y > 0 && speed_y != SMCP1_ARG_UNDEF && speed_y < smallest_speed) {
        smallest_speed = speed_y;
    }
    if (step_z != 0.0 && speed_z > 0 && speed_z != SMCP1_ARG_UNDEF && speed_z < smallest_speed) {
        smallest_speed = speed_z;
    }
    if (step_w != 0.0 && speed_w > 0 && speed_w != SMCP1_ARG_UNDEF && speed_w < smallest_speed) {
        smallest_speed = speed_w;
    }

    if (smallest_speed <= 50 && smallest_speed >= 10) {
        cls_mode = 1; // CLS = 2
    } else if (smallest_speed < 10) {
        cls_mode = 2;
    } // CLS = 1
    return cls_mode;
}

int um_take_step(um_state *hndl, const int dev, const float step_x, const float step_y, const float step_z,
                 const float step_w, const int spd_x, const int spd_y, const int spd_z, const int spd_w, const int mode,
                 const int max_acc) {
    int args[10], argc = 0;
    int speed_x = spd_x, speed_y = spd_y, speed_z = spd_z, speed_w = spd_w;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }

    if (step_x && !speed_x) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    } else if (!step_x) {
        speed_x = 0;
    }
    if (step_y && !speed_y) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    } else if (!step_y) {
        speed_y = 0;
    }
    if (step_z && !speed_z) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    } else if (!step_z) {
        speed_z = 0;
    }
    if (step_w && !speed_w) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    } else if (!step_w) {
        speed_w = 0;
    }
    args[argc++] = um2nm (step_x);
    args[argc++] = um2nm (step_y);
    args[argc++] = um2nm (step_z);
    args[argc++] = um2nm (step_w);
    args[argc++] = speed_x;
    args[argc++] = speed_y;
    args[argc++] = speed_z;
    args[argc++] = speed_w;

    int clsMode = mode;
    // Dynamic CLS mode selection if not given as an argument
    if (!mode) {
        clsMode = ump_resolve_cls_mode (step_x, step_y, step_z, step_w, speed_x, speed_y, speed_z, speed_w);
    }
    if (clsMode >= 0) {
        args[argc++] = clsMode;
    } else {
        args[argc++] = 0;
    }
    if (max_acc) {
        args[argc++] = max_acc;
    }
    return um_cmd (hndl, dev, SMCP1_CMD_TAKE_STEP, argc, args);
}

int um_get_feature(um_state *hndl, const int dev, const int feature_id) {
    int ret, resp[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_GET_FEATURE, 1, &feature_id, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != feature_id || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    return resp[1];
}

int um_get_ext_feature(um_state *hndl, const int dev, const int feature_id) {
    int ret, resp[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_GET_EXT_FEATURE, 1, &feature_id, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != feature_id || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    return resp[1];
}

int um_get_feature_mask(um_state *hndl, const int dev, const int feature_id) {
    int ret, resp[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_CMD_GET_FEATURE_MASK, 1, &feature_id, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != feature_id || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    return resp[1];
}

int um_get_feature_functionality(um_state *hndl, const int dev, const int feature_id) {
    int ret, resp[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_CMD_GET_FEATURE_FUNCTIONALITY, 1, &feature_id, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != feature_id || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    return resp[1];
}

int um_read_version(um_state *hndl, const int dev, int *version, const int size) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    return um_send_msg (hndl, dev, SMCP1_GET_VERSION, 0, NULL, 0, NULL, size, version);
}

int um_get_axis_count(um_state *hndl, const int dev) {
    int ret, value = 0;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if ((ret = um_get_param (hndl, dev, SMCP1_PARAM_AXIS_COUNT, &value)) < 0) {
        return ret;
    }
    return value;
}

int um_get_positions(um_state *hndl, const int dev, const int time_limit, float *x, float *y, float *z, float *d,
                     int *elapsedptr) {
    int resp[4], ret = 0;
    um_positions *positions;
    unsigned long long start, elapsed;

    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }

    int dev_id = um_resolve_dev_id (dev);
    positions = &hndl->last_positions[dev_id];

    elapsed = get_elapsed (positions->updated_us / 1000LL);

    if ((elapsed < (unsigned long) time_limit || time_limit == LIBUM_TIMELIMIT_CACHE_ONLY) &&
        time_limit != LIBUM_TIMELIMIT_DISABLED) {
        if (positions->x != SMCP1_ARG_UNDEF && x) {
            *x = nm2um (positions->x);
            ret++;
        }
        if (positions->y != SMCP1_ARG_UNDEF && y) {
            *y = nm2um (positions->y);
            ret++;
        }
        if (positions->z != SMCP1_ARG_UNDEF && z) {
            *z = nm2um (positions->z);
            ret++;
        }
        if (positions->d != SMCP1_ARG_UNDEF && d) {
            *d = nm2um (positions->d);
            ret++;
        }
        if (elapsedptr) {
            *elapsedptr = (int) elapsed;
        }
        if (ret > 0) {
            return ret;
        }
    }
    // Too old or missing positions, request them from the manipulator
    memset(resp, 0, sizeof (resp));
    start = um_get_timestamp_ms ();
    if ((ret = um_send_msg (hndl, dev, SMCP1_GET_POSITIONS, 0, NULL, 0, NULL, 4, resp)) > 0) {
        int time_step = um_update_position_cache_time (hndl, dev_id);
        um_update_positions_cache (hndl, dev_id, 0, resp[0], time_step);
        if (x) {
            *x = positions->x != SMCP1_ARG_UNDEF ? nm2um (positions->x) : 0.0f;
        }
        if (ret > 1) {
            um_update_positions_cache (hndl, dev_id, 1, resp[1], time_step);
            if (y) {
                *y = positions->x != SMCP1_ARG_UNDEF ? nm2um (positions->y) : 0.0f;
            }
        }
        if (ret > 2) {
            um_update_positions_cache (hndl, dev_id, 2, resp[2], time_step);
            if (z) {
                *z = positions->z != SMCP1_ARG_UNDEF ? nm2um (positions->z) : 0.0f;
            }
        }
        if (ret > 3) {
            um_update_positions_cache (hndl, dev_id, 3, resp[3], time_step);
            if (d) {
                *d = positions->d != SMCP1_ARG_UNDEF ? nm2um (positions->d) : 0.0f;
            }
        }
        positions->updated_us = um_get_timestamp_us ();
    }
    if (elapsedptr && positions->updated_us) {
        *elapsedptr = (int) get_elapsed (start);
    }
    return ret;
}

int um_get_speeds(um_state *hndl, const int dev, float *x, float *y, float *z, float *d, int *elapsedptr) {
    int ret = 0;
    um_positions *positions;
    unsigned long long elapsed;

    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }

    int dev_id = um_resolve_dev_id (dev);
    positions = &hndl->last_positions[dev_id];
    elapsed = get_elapsed (positions->updated_us / 1000LL);

    if (x) {
        *x = positions->speed_x;
        ret++;
    }
    if (y) {
        *y = positions->speed_y;
        ret++;
    }
    if (z) {
        *z = positions->speed_z;
        ret++;
    }
    if (d) {
        *d = positions->speed_d;
        ret++;
    }

    if (elapsedptr) {
        *elapsedptr = (int) elapsed;
    }
    return ret;
}

int um_read_positions(um_state *hndl, const int dev, const int time_limit) {
    int resp[4], ret = 0;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    int dev_id = um_resolve_dev_id (dev);
    um_positions *positions = &hndl->last_positions[dev_id];
    unsigned long long elapsed = get_elapsed (positions->updated_us / 1000LL);
    // Use values from the cache if new enough
    if ((elapsed < (unsigned long) time_limit || time_limit == LIBUM_TIMELIMIT_CACHE_ONLY) &&
        time_limit != LIBUM_TIMELIMIT_DISABLED) {
        if (hndl->last_positions[dev_id].x != SMCP1_ARG_UNDEF) {
            ret++;
        }
        if (hndl->last_positions[dev_id].y != SMCP1_ARG_UNDEF) {
            ret++;
        }
        if (hndl->last_positions[dev_id].z != SMCP1_ARG_UNDEF) {
            ret++;
        }
        if (hndl->last_positions[dev_id].d != SMCP1_ARG_UNDEF) {
            ret++;
        }
        if (ret > 0) {
            return ret;
        }
    }

    // Request positions from the manipulator
    memset(resp, 0, sizeof (resp));
    if ((ret = um_send_msg (hndl, dev, SMCP1_GET_POSITIONS, 0, NULL, 0, NULL, 4, resp)) > 0) {
        positions->x = resp[0];
        if (ret > 1) {
            positions->y = resp[1];
        }
        if (ret > 2) {
            positions->z = resp[2];
        }
        if (ret > 3) {
            positions->d = resp[3];
        }
    }
    positions->updated_us = um_get_timestamp_us ();
    return ret;
}


int um_get_device_list(um_state *hndl, int *devs, const int size) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    int i, ret, found = 0;

    um_cmd_options (hndl, SMCP1_OPT_REQ_ACK);
    if ((ret = um_ping (hndl, SMCP1_ALL_DEVICES)) < 0 && ret != LIBUM_INVALID_DEV && ret != LIBUM_TIMEOUT) {
        return ret;
    }

    if ((ret = um_receive (hndl, hndl->timeout)) < 0) {
        return ret;
    }

    for (i = 0; i < LIBUM_MAX_DEVS; i++) {
        // Do not include TSC and PC/SDK into device list.
        if (i >= SMCP1_ALL_DEVICES && i <= SMCP1_UMP_DEV_ID_OFFSET) {
            continue;
        }
        if (hndl->addresses[i].sin_family != 0) {
            if (devs) {
                int sno = 0;
                if (um_resolve_sno (i, &sno)) {
                    devs[found] = sno;
                } else {
                    devs[found] = i;
                }
            }
            found++;
            if (found >= size) {
                break;
            }
        }
    }
    return found;
}

int um_clear_device_list(um_state *hndl) {
    int i, found = 0;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    for (i = 0; i < LIBUM_MAX_DEVS; i++) {
        if (hndl->addresses[i].sin_family != 0) {
            hndl->addresses[i].sin_family = 0;
            found++;
        }
    }
    return found;
}

int um_set_uma_reg(um_state *hndl, const int dev, const int addr, const int value) {
    int args[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    args[0] = addr;
    args[1] = value;
    return um_cmd (hndl, dev, SMCP1_SET_UMA_REG, 2, args);
}

int um_get_uma_reg(um_state *hndl, const int dev, const int addr, int *value) {
    int args[1], ret, resp[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    args[0] = addr;
    ret = um_send_msg (hndl, dev, SMCP1_GET_UMA_REG, 1, args, 0, NULL, 2, resp);
    if (ret < 0) {
        return ret;
    }
    if (resp[0] != addr || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    *value = resp[1];
    return 1;
}

int um_set_uma_regs(um_state *hndl, const int dev, const int count, const int *values) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (count < 1 || count > UMA_REG_COUNT) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    return um_cmd (hndl, dev, SMCP1_SET_UMA_REGS, count, values);
}

int um_get_uma_regs(um_state *hndl, const int dev, const int count, int *values) {
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (count < 1 || count > UMA_REG_COUNT) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    return um_send_msg (hndl, dev, SMCP1_GET_UMA_REGS, 0, NULL, 0, NULL, count, values);
}

// uMv specific commands
int umc_set_pressure_setting(um_state *hndl, const int dev, const int channel, const float pressure_kpa) {
    int args[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    // Currently uMv pressure range is -70 - +70kPa
    if (channel < 1 || channel > 8 || pressure_kpa < -100.0 || pressure_kpa > 100.0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    args[0] = channel - 1;
    args[1] = (int) (pressure_kpa * 1000.0);
    return um_cmd (hndl, dev, SMCP1_UMV_SET_PRESSURE, 2, args);
}

int umc_get_pressure_setting(um_state *hndl, const int dev, const int channel, float *pressure_kpa) {
    int ret, resp[2], chn = channel - 1;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (channel < 1 || channel > 8) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_UMV_GET_PRESSURE, 1, &chn, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != chn || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    *pressure_kpa = (float) resp[1] / 1000.0f;
    return abs (resp[1]);
}

int umc_measure_pressure(um_state *hndl, const int dev, const int channel, float *pressure_kpa) {
    int ret, resp[2], chn = channel - 1;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (channel < 1 || channel > 8) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_UMV_MEASURE_PRESSURE, 1, &chn, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != chn || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    *pressure_kpa = (float) resp[1] / 1000.0f;
    return abs (resp[1]);
}

int umc_get_pressure_monitor_adc(um_state *hndl, const int dev, const int channel) {
    int ret, resp[2], chn = channel - 1;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (channel < 1 || channel > 8) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_UMV_GET_MONITOR_ADC, 1, &chn, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != chn || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    return resp[1];
}

int umc_set_valve(um_state *hndl, const int dev, const int channel, const int value) {
    int args[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (channel < 1 || channel > 8 || value < 0 || value > 1) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    args[0] = channel - 1;
    args[1] = value;
    return um_cmd (hndl, dev, SMCP1_UMV_SET_VALVE, 2, args);
}

int umc_get_valve(um_state *hndl, const int dev, const int channel) {
    int ret, resp[2], chn = channel - 1;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }

    if (channel < 1 || channel > 8) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }

    if ((ret = um_send_msg (hndl, dev, SMCP1_UMV_GET_VALVE, 1, &chn, 0, NULL, 2, resp)) < 0) {
        return ret;
    }
    if (resp[0] != chn || ret != 2) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    return resp[1];
}

int umc_reset_fluid_detector(um_state *hndl, const int dev, const int channel) {
    int arg;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (channel < 1 || channel > 8) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    arg = channel - 1;
    return um_cmd (hndl, dev, SMCP1_UMV_RESET_FLUID_DETECTOR, 1, &arg);
}

int umc_read_fluid_detectors(um_state *hndl, const int dev) {
    int ret, resp;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_UMV_READ_FLUID_DETECTORS, 0, NULL, 0, NULL, 1, &resp)) < 0) {
        return ret;
    }
    return resp;
}

int umc_reset_sensor_offset(um_state *hndl, const int dev, const int channel) {
    int arg;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    // channel argument 0 to reset all pressure sensor offsets
    if (channel < 0 || channel > 8) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    arg = channel - 1;
    if (channel > 0) {
        return um_cmd (hndl, dev, SMCP1_UMV_RESET_SENSOR_OFFSET, 1, &arg);
    }
    return um_cmd (hndl, dev, SMCP1_UMV_RESET_SENSOR_OFFSET, 0, NULL);
}

int umc_pressure_calib(um_state *hndl, const int dev, const int channel, const int delay) {
    int args[2];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    // channel argument 0 to calibrate all channels
    if (channel < 0 || channel > 8) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (delay < 0 || delay > 10000) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    args[0] = channel - 1;
    args[1] = delay;
    if (channel > 0) {
        return um_cmd (hndl, dev, SMCP1_UMV_PRESSURE_CALIB, delay > 0 ? 2 : 1, args);
    }
    return um_cmd (hndl, dev, SMCP1_UMV_PRESSURE_CALIB, 0, NULL);
}

// uMs specific commands
int ums_set_lens_position(um_state *hndl, const int dev, const int position, const float lift, const float dip) {
    int argc = 0, args[3];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if (position < 0 || position > 9) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (lift < 0.0 || dip < 0.0) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    args[argc++] = position;
    if (!um_arg_undef (lift)) {
        args[argc++] = um2nm (lift);
        if (!um_arg_undef (dip)) {
            args[argc++] = um2nm (dip);
        }
    }
    return um_cmd (hndl, dev, SMCP1_CMD_UMS_SET_LENS_POSITION, argc, args);
}

int ums_get_lens_position(um_state *hndl, const int dev) {
    int ret, resp;
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if ((ret = um_send_msg (hndl, dev, SMCP1_CMD_UMS_GET_LENS_POSITION, 0, NULL, 0, NULL, 1, &resp)) < 0) {
        return ret;
    }
    // Map unknown and center (nothing seen on microscope) positions to the same in this API so that it's not
    // conflicting with SMCPv1 level error -1.
    if (resp == -1) {
        resp = 0;
    }
    return resp;
}

int ums_set_objective_configuration(um_state *hndl, const int dev, const ums_objective_conf *obj1,
                                    const ums_objective_conf *obj2) {
    int args[2 * 4];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if (!obj1 || !obj2) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    if (obj1->mag <= 0 || obj2->mag <= 0 || obj1->mag > 1000 || obj2->mag > 1000) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    args[0] = obj1->mag;
    args[1] = um2nm (obj1->x_offset);
    args[2] = um2nm (obj1->y_offset);
    args[3] = um2nm (obj1->z_offset);
    args[4] = obj2->mag;
    args[5] = um2nm (obj2->x_offset);
    args[6] = um2nm (obj2->y_offset);
    args[7] = um2nm (obj2->z_offset);
    return um_cmd (hndl, dev, SMCP1_CMD_UMS_SET_OBJECTIVE_CONTROL, 8, args);
}

int ums_get_objective_configuration(um_state *hndl, const int dev, ums_objective_conf *obj1, ums_objective_conf *obj2) {
    int ret, resp[8];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    memset(obj1, 0, sizeof (ums_objective_conf));
    memset(obj2, 0, sizeof (ums_objective_conf));

    if ((ret = um_send_msg (hndl, dev, SMCP1_CMD_UMS_GET_OBJECTIVE_CONTROL, 0, NULL, 0, NULL, 8, resp)) < 0) {
        return ret;
    }
    if (ret != 8) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    obj1->mag = resp[0];
    obj1->x_offset = nm2um (resp[1]);
    obj1->y_offset = nm2um (resp[2]);
    obj1->z_offset = nm2um (resp[3]);
    obj2->mag = resp[4];
    obj2->x_offset = nm2um (resp[5]);
    obj2->y_offset = nm2um (resp[6]);
    obj2->z_offset = nm2um (resp[7]);
    return ret;
}

int
ums_set_bowl_control(um_state *hndl, const int dev, const ums_bowl_control *control, const ums_bowl_center *centers) {
    int i, args[UMS_BOWL_CONTROL_HEADER_SIZE + UMS_BOWL_MAX_COUNT];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    if (control->count < 0 || control->count > UMS_BOWL_MAX_COUNT) {
        return set_last_error (hndl, LIBUM_INVALID_ARG);
    }
    args[0] = control->count;
    args[1] = (int) (control->objective_od * 1000000.0);
    args[2] = (int) (control->bowl_id * 1000000.0);
    args[3] = (int) (control->z_limit_low * 1000000.0);
    args[4] = (int) (control->z_limit_high * 1000000.0);

    for (i = 0; i < control->count; i++) {
        args[UMS_BOWL_CONTROL_HEADER_SIZE + i * 2] = (int) (centers[i].x * 1000000.0);
        args[UMS_BOWL_CONTROL_HEADER_SIZE + i * 2 + 1] = (int) (centers[i].y * 1000000.0);
    }
    return um_cmd (hndl, dev, SMCP1_CMD_UMS_SET_BOWL_CONTROL, UMS_BOWL_CONTROL_HEADER_SIZE + control->count * 2, args);
}

int ums_get_bowl_control(um_state *hndl, const int dev, ums_bowl_control *control, ums_bowl_center *centers) {
    int ret, i, resp[UMS_BOWL_CONTROL_HEADER_SIZE + UMS_BOWL_MAX_COUNT];
    if (!hndl) {
        return set_last_error (hndl, LIBUM_NOT_OPEN);
    }
    if (is_invalid_dev (dev)) {
        return set_last_error (hndl, LIBUM_INVALID_DEV);
    }
    memset(control, 0, sizeof (ums_bowl_control));

    if ((ret = um_send_msg (hndl, dev, SMCP1_CMD_UMS_GET_BOWL_CONTROL, 0, NULL, 0, NULL,
                            UMS_BOWL_CONTROL_HEADER_SIZE + UMS_BOWL_MAX_COUNT, resp)) < 0) {
        return ret;
    }
    if (ret < UMS_BOWL_CONTROL_HEADER_SIZE) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    control->count = resp[0];
    if (control->count < 0 || control->count > UMS_BOWL_MAX_COUNT) {
        return set_last_error (hndl, LIBUM_INVALID_RESP);
    }
    control->objective_od = (float) resp[1] / 1000000.0f;
    control->bowl_id = (float) resp[2] / 1000000.0f;
    control->z_limit_low = (float) resp[2] / 1000000.0f;
    control->z_limit_high = (float) resp[4] / 1000000.0f;
    for (i = 0; i < control->count && i < UMS_BOWL_MAX_COUNT; i++) {
        centers[i].x = (float) resp[UMS_BOWL_CONTROL_HEADER_SIZE + i * 2] / 1000000.0f;
        centers[i].y = (float) resp[UMS_BOWL_CONTROL_HEADER_SIZE + i * 2 + 1] / 1000000.0f;
    }
    return control->count;
}
