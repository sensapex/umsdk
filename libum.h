/**
 * @file    libum.h
 * @author  Sensapex <support@sensapex.com>
 * @date    29 May 2020
 * @brief   This file contains a public API for the 2015 series Sensapex uM product family SDK
 * @copyright   Copyright (c) 2016-2021 Sensapex. All rights reserved
 *
 * The Sensapex uM product family SDK is free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * This version of SDK contains support for Sensapex uMp micromanipulators (uMp),
 * uMs microscope motorization products (uMs) and automated pressure controller products (uMc)
 *
 * The Sensapex Micromanipulator SDK is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Sensapex micromanipulator SDK. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef LIBUM_H
#define LIBUM_H

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
#  ifndef _WINDOWS
#   define _WINDOWS
#  endif
#  ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#  ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#    define _WINSOCK_DEPRECATED_NO_WARNINGS
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
#  include <math.h>
#  define SOCKOPT_CAST   (char *)           /**< cross platform trick, non-standard variable type requires typecasting in windows for socket options */
#  define socklen_t       int               /**< cross platform trick, socklen_t is not defined in windows */
#  define size_t          int               /**< cross platform trick, winsock use int instead of size_t e.g. in recvfrom */
#  define sscanf          sscanf_s          /**< cross platform trick, get rid of unsafe compiler error */
#  define getLastError()  WSAGetLastError() /**< cross platform trick, using winsocket function instead of errno */
#  define timeoutError    WSAETIMEDOUT      /**< cross platform trick, detect timeout with winsocket error number */
typedef struct sockaddr_in IPADDR;          /**< alias for sockaddr_in */

// Define this is embedding SDK into application directly, but note the LGPL license requirements
# ifdef LIBUM_SHARED_DO_NOT_EXPORT
#  define LIBUM_SHARED_EXPORT
# else
#  if defined(LIBUM_LIBRARY)
#   define LIBUM_SHARED_EXPORT __declspec(dllexport) /**< cross platform trick, declspec for windows*/
#  else
#   define LIBUM_SHARED_EXPORT __declspec(dllimport) /**< cross platform trick, declspec for windows*/
#  endif
# endif
#else // !_WINDOWS
# include <unistd.h>
# include <math.h>
# include <arpa/inet.h>
# include <sys/errno.h>
typedef int SOCKET;                         /**< cross platform trick, int instead of SOCKET (defined by winsock) in posix systems */
typedef struct sockaddr_in IPADDR;          /**< alias for sockaddr_in */
# define SOCKET_ERROR   -1                  /**< cross platform trick, replace winsocket return value in posix systems */
# define INVALID_SOCKET -1                  /**< cross platform trick, replace winsocket return value in posix systems */
# define getLastError() errno               /**< cross platform trick, errno instead of WSAGetLastError() in posix systems */
# define timeoutError   ETIMEDOUT           /**< cross platform trick, errno ETIMEDOUT instead of WSAETIMEDOUT in posix systems */
# define closesocket    close               /**< cross platform trick, close() instead of closesocket() in posix systems */
# define SOCKOPT_CAST                       /**< cross platform trick, non-standard typecast not needed in posix systems */
# define LIBUM_SHARED_EXPORT                /**< cross platform trick, declspec for windows DLL, empty for posix systems */
#endif

#ifdef _WINDOWS
# ifndef LIBUM_SHARED_DO_NOT_EXPORT
LIBUM_SHARED_EXPORT HRESULT __stdcall DllRegisterServer(void);
LIBUM_SHARED_EXPORT HRESULT __stdcall DllUnregisterServer(void);
# endif
#endif

/**
 * The precompiler condition below is utilized by C++ compilers and is
 * ignored by pure C ones
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDK error enums
 */
typedef enum um_error_e
{
    LIBUM_NO_ERROR     =  0,  /**< No error */
    LIBUM_OS_ERROR     = -1,  /**< Operating System level error */
    LIBUM_NOT_OPEN     = -2,  /**< Communication socket not open */
    LIBUM_TIMEOUT      = -3,  /**< Timeout occurred */
    LIBUM_INVALID_ARG  = -4,  /**< Illegal command argument */
    LIBUM_INVALID_DEV  = -5,  /**< Illegal Device Id */
    LIBUM_INVALID_RESP = -6,  /**< Illegal response received */
} um_error;

/**
 * @brief Device status enums
 *
 * These cause busy state
 */

typedef enum um_status_e
{
    LIBUM_STATUS_READ_ERROR    = -1,     /**< Failure at status reading */
    LIBUM_STATUS_OK            = 0,      /**< No error and status idle */
    LIBUM_STATUS_BUSY          = 1,      /**< Device busy (not necessarily moving) */
    LIBUM_STATUS_ERROR         = 8,      /**< Device in error state */
    LIBUM_STATUS_X_MOVING      = 0x10,   /**< X-axis is moving */
    LIBUM_STATUS_Y_MOVING      = 0x20,   /**< Y-axis  is moving */
    LIBUM_STATUS_Z_MOVING      = 0x40,   /**< Z-axis is moving */
    LIBUM_STATUS_W_MOVING      = 0x80,   /**< 4th actuator is moving */
    LIBUM_STATUS_UMC_CHN1_BUSY = 0x0100, /**< Channel 1 is busy */
    LIBUM_STATUS_UMC_CHN2_BUSY = 0x0200, /**< Channel 2 is busy */
    LIBUM_STATUS_UMC_CHN3_BUSY = 0x0400, /**< Channel 3 is busy */
    LIBUM_STATUS_UMC_CHN4_BUSY = 0x0800, /**< Channel 4 is busy */
    LIBUM_STATUS_UMC_CHN5_BUSY = 0x1000, /**< Channel 5 is busy */
    LIBUM_STATUS_UMC_CHN6_BUSY = 0x2000, /**< Channel 6 is busy */
    LIBUM_STATUS_UMC_CHN7_BUSY = 0x4000, /**< Channel 7 is busy */
    LIBUM_STATUS_UMC_CHN8_BUSY = 0x8000, /**< Channel 8 is busy */
} um_status;

/*
 * Some default values and other platform independent defines
 */

#define LIBUM_DEF_STORAGE_ID      0        /**< default position storage */
#define LIBUM_DEF_TIMEOUT         20       /**< default message timeout in milliseconds */
#define LIBUM_DEF_BCAST_ADDRESS  "169.254.255.255" /**< default link-local broadcast address */
#define LIBUM_DEF_GROUP           0        /**< default manipulator group, group 0 is called 'A' on TCU UI */
#define LIBUM_MAX_TIMEOUT         60000    /**< maximum message timeout in milliseconds */
#define LIBUM_MAX_LOG_LINE_LENGTH 256      /**< maximum log message length */

#define LIBUM_ARG_UNDEF           NAN      /**< function argument undefined (used for float when 0.0 is a valid value) */

#define LIBUM_FEATURE_VIRTUALX    0        /**< id number for virtual X axis feature */

#define LIBUM_MAX_DEVS            0xFFFF   /**< Max count of concurrent devices supported by this SDK version*/
#define LIBUM_DEF_REFRESH_TIME    20       /**< The default positions refresh period in ms */
#define LIBUM_MAX_POSITION        125000   /**< The upper absolute position limit */

#define LIBUM_TIMELIMIT_CACHE_ONLY 0       /**< Read position always from the cache */
#define LIBUM_TIMELIMIT_DISABLED  -1       /**< Skip the internal position cache.
                                                 Use this definition as a parameter to read an actuator position
                                                 directly from a manipulator */
#define LIBUM_POS_DRIVE_COMPLETED  0       /**< position drive completed */
#define LIBUM_POS_DRIVE_BUSY       1       /**< position drive busy */
#define LIBUM_POS_DRIVE_FAILED    -1       /**< position drive failed */

/**
 * @brief Positions used in #um_state
 */
typedef struct um_positions_s
{
    int x;                 /**< X-actuator position */
    int y;                 /**< Y-actuator position */
    int z;                 /**< Z-actuator position */
    int d;                 /**< D-actuator position */
    float speed_x;         /**< X-actuator movement speed between last two position updates */
    float speed_y;         /**< Y-actuator movement speed between last two position updates */
    float speed_z;         /**< Z-actuator movement speed between last two position updates */
    float speed_d;         /**< D-actuator movement speed between last two position updates */
    unsigned long long updated_us; /**< Timestamp (in microseconds) when positions were updated */
} um_positions;

/**
 * @brief Prototype for the log print callback function
 *
 * @param   level   Verbosity level of the message
 * @param   arg     Optional argument e.g. a file handle, optional, may be NULL
 * @param   message Pointer to a static buffer containing the log print line without trailing line feed
 *
 * @return  Pointer to an error string
 */

typedef void (*um_log_print_func)(int level, const void *arg, const char *func, const char *message);

/**
 * @brief The state struct, pointer to this is the session handle in the C API
 */
typedef struct um_state_s
{
    unsigned long last_received_time;                   /**< Timestamp of the latest incoming message */
    SOCKET socket;                                      /**< UDP socket */
    int own_id;                                         /**< The device ID if this SDK */
    unsigned short message_id;                          /**< Message id (autoincremented counter for messages sent by this SDK */
    int last_device_sent;                               /**< Device ID of selected and/or communicated target device */
    int last_device_received;                           /**< ID of device that has sent the latest message */
    int retransmit_count;                               /**< Resend count for requests requesting ACK */
    int refresh_time_limit;                             /**< Refresh time limit for the position cache */
    int last_error;                                     /**< Error code of the latest error */
    int last_os_errno;                                  /**< OS level errno of the latest error */
    int timeout;                                        /**< UDP transport message timeout */
    int udp_port;                                       /**< Target UDP port */
    int local_port;                                     /**< Local UDP port */
    int last_status[LIBUM_MAX_DEVS];                    /**< Status cache */
    int drive_status[LIBUM_MAX_DEVS];                   /**< Position drive state #LIBUM_DRIVE_BUSY, #LIBUM_DRIVE_COMPLETED or #LIBUM_DRIVE_FAILED */
    unsigned short drive_status_id[LIBUM_MAX_DEVS];     /**< Message ids of the above notifications, used to detect duplicates */
    IPADDR addresses[LIBUM_MAX_DEVS];                   /**< Address cache */
    um_positions last_positions[LIBUM_MAX_DEVS];        /**< Position cache */
    IPADDR laddr;                                       /**< UDP local address */
    IPADDR raddr;                                       /**< UDP remote address */
    char errorstr_buffer[LIBUM_MAX_LOG_LINE_LENGTH];    /**< The work buffer of the latest error string handler */
    int verbose;                                        /**< Enable log printouts to stderr, utilized for SDK development */
    um_log_print_func log_func_ptr;                     /**< External log print function pointer */
    const void *log_print_arg;                          /**< Argument for the above */
    int next_cmd_options;                               /**< Option bits to set for the smcp commands:
                                                        SMCP1_OPT_WAIT_TRIGGER_1 0x00000200 // Set message to be run when triggered by physical trigger line2
                                                        SMCP1_OPT_PRIORITY       0x00000100 // Prioritizes message to run first. // 0 = normal message
                                                        SMCP1_OPT_REQ_BCAST      0x00000080 // send ACK, RESP or NOTIFY to the bcast address (combine with REQs below), 0 = unicast to the sender
                                                        SMCP1_OPT_REQ_NOTIFY     0x00000040 //request notification (e.g. on completed memory drive), 0 = do not notify
                                                        SMCP1_OPT_REQ_RESP       0x00000020 // request ACK, 0 = no ACK requested
                                                        SMCP1_OPT_REQ_ACK        0x00000010 // request ACK, 0 = no ACK requested
                                                        */
    unsigned long long drive_status_ts[LIBUM_MAX_DEVS]; /**< position drive state check timestamp - last time PWM seen busy, updated by get_drive_status */
    unsigned long long last_msg_ts[LIBUM_MAX_DEVS];     /**< Time stamp of last sent packet per device */
} um_state;

/**
 * @brief Open UDP socket, allocate and initialize state structure
 *
 * @param   udp_target_address    typically an UDP broadcast address
 * @param   timeout               message timeout in milliseconds
 * @param   group                 m0 for default group 'A' on TSC UI
 *
 * @return  Pointer to created session handle. NULL if an error occurred
 */

LIBUM_SHARED_EXPORT um_state *um_open(const char *udp_target_address, const unsigned int timeout, const int group);

/**
 * @brief Close the UDP socket if open and free the state structure allocated in open
 *
 * @param   hndl    Pointer to session handle
 * @return  None
 */

LIBUM_SHARED_EXPORT void um_close(um_state *hndl);


/**
 * For most C functions returning int, a negative values means error,
 * get the possible error number or description using some of these functions.
 * Often the last one is enough.
 */

/**
 * @brief Get the latest error
 *
 * @param   hndl    Pointer to session handle
 * @return  `um_error` error code
 */
LIBUM_SHARED_EXPORT um_error um_last_error(const um_state *hndl);

/**
 * @brief This function can be used to get the actual operating system level error number
 * when um_last_error returns LIBUM_OS_ERROR.
 *
 * @param   hndl    Pointer to session handle
 * @return  Error code
 */
LIBUM_SHARED_EXPORT int um_last_os_errno(const um_state *hndl);

/**
 * @brief Translate an error code to human readable format
 *
 * @param   error_code    Error code to be translated to text string
 * @return  Pointer to an error string
 */
LIBUM_SHARED_EXPORT const char *um_errorstr(const um_error error_code);

/**
 * @brief Get the latest error in human readable format
 *
 * @param   hndl    Pointer to session handle
 * @return  Pointer to an error string
 */
LIBUM_SHARED_EXPORT const char *um_last_errorstr(um_state *hndl);


/**
 * @brief Set up external log print function. By default the library writes
 *        to the stderr if verbose level is higher than zero.
 *
 * @param   hndl            Pointer to session handle
 * @param   verbose_level   Verbose level (zero to disable, higher value for more detailed printouts)
 * @param   func            Pointer to the custom log print function.
 *                          May be NULL if setting only verbose level for internal log print out to stderr
 * @param   arg             Pointer argument to be looped to the above function may be e.g. a typecasted
 *                          file handle, optional, may be NULL
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_set_log_func(um_state *hndl, const int verbose_level,
                                          um_log_print_func func, const void *arg);

/**
 * @brief Get SDK library version
 *
 * @return  Pointer to version string
 */
LIBUM_SHARED_EXPORT const char *um_get_version();

/**
 * @brief Ping device
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */
LIBUM_SHARED_EXPORT int um_ping(um_state *hndl, const int dev);

/**
 * @brief Check if a device is busy.
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_is_busy(um_state *hndl, const int dev);

/**
 * @brief Obtain position drive status
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @return  Status of the selected manipulator, #LIBUM_POS_DRIVE_COMPLETED,
 *          #LIBUM_POS_DRIVE_BUSY or #LIBUM_POS_DRIVE_FAILED
 */

LIBUM_SHARED_EXPORT int um_get_drive_status(um_state *hndl, const int dev);

/**
 * @brief Read device firmware version.
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @param[out]  version   Pointer to an allocated buffer for firmware numbers
 * @param   size    size of the above buffer (number of integers)
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_read_version(um_state *hndl, const int dev,
                                              int *version, const int size);

/**
 * @brief Read uMp or uMs axis count
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @return  Negative value if an error occurred. Axis count otherwise
 */

LIBUM_SHARED_EXPORT int um_get_axis_count(um_state * hndl, const int dev);

/**
 * @brief Initialize uMp or uMs zero position
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @param   axis_mask Select axis to move, 1 = X, 2 = Y, 4 = Z, 8 = D, 0 or 15 for all.
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int um_init_zero(um_state * hndl, const int dev, const int axis_mask);

/**
 * @brief Save uMp or uMs zero positions.
 *
 * @param   hndl        Pointer to session handle
 * @param   dev         Device ID *
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_save_zero(um_state *hndl, const int dev);

/**
 * @brief Manipulator load calibration
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int ump_calibrate_load(um_state *hndl, const int dev);

/**
 * @brief Manipulator LED control
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @param   off     1 to turn off all LEDS including position sensors, 0 to restore normal operation
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int ump_led_control(um_state *hndl, const int dev, const int off);

/**
 * @brief Drive uMp or uMs to a defined position.
 *
 * @param   hndl        Pointer to session handle
 * @param   dev         Device ID
 * @param   x, y, z, w  Positions in um, LIBUM_ARG_UNDEF for axis not to be moved
 * @param   speed       speed in um/s
 * @param   mode        0 = one-by-one, 1 = move all axis simultanously.
 * @param   max_acc     maximum acceleration in um/s^2
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_goto_position(um_state *hndl, const int dev,
                                         const float x, const float y,
                                         const float z, const float d,
                                         const float speed, const int mode,
                                         const int max_acc);

/**
 * @brief An alternative API to drive uMp or uMs to a defined position with axis specific speed
 *
 * @param   hndl        Pointer to session handle
 * @param   dev         Device ID
 * @param   x, y, z, w  Positions, #LIBUM_ARG_UNDEF for axis not to be moved
 * @param   speedX, speedY, speedZ, speedW  in um/s, zero for axis not to be moved
 * @param   mode        0 = one-by-one, 1 = move all axis simultaneously.
 * @param   max_acc     maximum acceleration in um/s^2
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_goto_position_ext(um_state *hndl, const int dev,
                                             const float x, const float y,
                                             const float z, const float d,
                                             const float speedX, const float speedY,
                                             const float speedZ, const float speedW,
                                             const int mode, const int max_acc);
/**
 * @brief Stop device
 *
 * @param   hndl        Pointer to session handle
 * @param   dev         Device ID, SMCP1_ALL_DEVICES to stop all.
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_stop(um_state * hndl, const int dev);

/**
 * @brief Read socked to update the position and status caches
 *
 * This function can be used instead of a millisecond accurate delay
 * to read the socket and thus update the status and positions
 * into the cache
 *
 * @param   hndl       Pointer to session handle
 * @para    timelimit  delay in milliseconds, zero for process queued packages with zero wait
 * @return  Positive value indicates the count of received messages.
 *          Zero if no related messages was received. Negative value indicates an error.
 */

LIBUM_SHARED_EXPORT int um_receive(um_state *hndl, const int timelimit);

/**
 * @brief Read positions allowing to control the position value timings.
 *
 * A zero time_limit (LIBUM_TIMELIMIT_CACHE_ONLY) reads cached positions without
 * sending any request to the device.
 * Value -1 (LIBUM_TIMELIMIT_DISABLED) obtains the position always from the device.
 *
 * @param       hndl        Pointer to session handle
 * @param       dev         Device ID
 * @param       time_limit  Timelimit of cache values. If 0 then cached positions are used always.
 * @param[out]  x           Pointer to an allocated buffer for x-actuator position
 * @param[out]  y           Pointer to an allocated buffer for y-actuator position
 * @param[out]  z           Pointer to an allocated buffer for z-actuator position
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_get_positions(um_state *hndl, const int dev, const int time_limit,
                                         float *x, float *y, float *z, float *w, int *elapsed);

/**
 * @brief Read latest speeds and obtain time when the values were updated
 *
 *
 * @param       hndl        Pointer to session handle
 * @param       dev         Device ID
 *
 * @param[out]  x           Pointer to an allocated buffer for x-actuator speed
 * @param[out]  y           Pointer to an allocated buffer for y-actuator speed
 * @param[out]  z           Pointer to an allocated buffer for z-actuator speed
 * @param[out]  w           Pointer to an allocated buffer for w-actuator speed
 * @param[out]  elapsed     Pointer to an allocated buffer for value indicating position value age in ms
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT  int um_get_speeds(um_state *hndl, const int dev, float *x, float*y, float *z, float *w, int *elapsedptr);

/**
 * @brief Read positions of certain manipulator to the cache
 *
 * Value -1 (LIBUM_TIMELIMIT_DISABLED) for the time_limit obtains the position always
 * from the manipulator.
 *
 *
 * @param       hndl        Pointer to session handle
 * @param       dev         Device ID
 * @param       time_limit  Timelimit of cache values. If 0 then cached positions are used always.
 *                          If
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_read_positions(um_state *hndl, const int dev, const int time_limit);

/**
 * @brief An advanced API for obtaining single axis position value from the cache,
 * call after succeeded #um_read_positions_ext
 *
 * @param       hndl        Pointer to session handle
 * @param       dev         Device ID
 * @param       axis        Axis name 'x','y','z' or 'd'
 * @return  axis position from the cache, 0.0 if value not available
 */

LIBUM_SHARED_EXPORT float um_get_position(um_state *hndl, const int dev, const char axis);

/**
 * @brief An advanced API for obtaining single axis speed from the cache,
 * works when manipulator is moving and updating the positions periodically
 *
 * @param       hndl        Pointer to session handle
 * @param       dev         Device ID
 * @param       axis        Axis name 'x','y','z' or 'd'
 * @return  axis movement speed in um/s
 */

LIBUM_SHARED_EXPORT float um_get_speed(um_state *hndl, const int dev, const char axis);

/**
 * @brief Take a step (relative movement from current position)
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @param   step_x   step length (in um) for X axis, negative value for backward, zero for axis not to be moved
 * @param   step_y   step length (in um) for Y axis, negative value for backward, zero for axis not to be moved
 * @param   step_z   step length (in um) for Z axis, negative value for backward, zero for axis not to be moved
 * @param   step_w   step length (in um) for D axis, negative value for backward, zero for axis not to be moved
 * @param   speed_x  movement speed (in um/s) for X axis
 * @param   speed_y  movement speed (in um/s) for Y axis
 * @param   speed_z  movement speed (in um/s) for Z axis
 * @param   speed_w  movement speed (in um/s) for D axis
 * @param   mode     movement mode (CLS for manipulator or microstepping mode for stage, value 0 for automatic selection)
 * @param   max_acceleration in um/s^2, give value 0 to use default
 *
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_take_step(um_state *hndl, const int dev,
                                     const float step_x, const float step_y, const float step_z, const float step_d,
                                     const int speed_x, const int speed_y, const int speed_z, const int speed_d,
                                     const int mode, const int max_acceleration);

/**
 * @brief um_cmd_options
 * Set options for next cmd to be sent for manipulator.
 * This is one time set and will be reset after sending the next message.
 * Can be used to set the trigger for next command (e.g. goto position)
 * @param   hndl        Pointer to session handle
 * @param   optionbits  Options bit to set. Use following:
 *  SMCP1_OPT_WAIT_TRIGGER_1 Set message to be run when triggered by physical trigger line2
 *  SMCP1_OPT_PRIORITY       Prioritizes message to run first. // 0 = normal message
 *  SMCP1_OPT_REQ_BCAST      Send ACK, RESP or NOTIFY to the bcast address (combine with REQs below), 0 = unicast to the sender
 *  SMCP1_OPT_REQ_NOTIFY     Request notification (e.g. on completed memory drive), 0 = do not notify
 *  SMCP1_OPT_REQ_RESP       Request RESP, 0 = no RESP requested
 *  SMCP1_OPT_REQ_ACK        Request ACK, 0 = no ACK requested
 *
 *  REQ_NOTIFY, REQ_RESP and REQ_ACK are applied automatically for various commands
 *
 * This option is applied only to the next command.
 * Options are cumulated (OR'ed) i.e. function can be called multiple time.
 * Call with value zero to reset options.
 *
 * @return  returns set flags
 */

LIBUM_SHARED_EXPORT int um_cmd_options(um_state *hndl, const int optionbits);

/**
 * @brief Get a manipulator parameter value
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @param   param_id Parameter id
 * @param[out] value Pointer to an allocated variable
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_get_param(um_state *hndl, const int dev, const int param_id, int *value);

/**
 * @brief Set a manipulator parameter value
 *
 * Note! This API is mainly for Sensapex internal development and production purpose and
 * should not be used unless you really know what you are doing.
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   param_id  Parameter id
 * @param   value     Data to be written
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_set_param(um_state *hndl, const int dev,
                                     const int param_id, const int value);
/**
 * @brief Write manipulator slow speed mode
 *
 * @param   hndl      Pointer to session handle
 * @param   activated On/off settings to enable/disable slow speed mode (0=deactivated, 1 = activated)
 *
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */
LIBUM_SHARED_EXPORT int um_set_slow_speed_mode(um_state *hndl, const int dev, const int activated);

/**
 * @brief Read manipulator slow speed mode
 *
 * @param   hndl      Pointer to session handle
 * @return  Negative value if an error occurred. 0 = disabled or 1 = enabled value otherwise
 */
LIBUM_SHARED_EXPORT int um_get_slow_speed_mode(um_state *hndl, const int dev);


/**
 * @brief Write manipulator soft start mode
 *
 * @param   hndl      Pointer to session handle
 * @param   activated On/off settings to enable/disable slow speed mode (0=deactivated, 1 = activated)
 *
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */
LIBUM_SHARED_EXPORT int um_set_soft_startmode(um_state *hndl, const int dev, const int activated);

/**
 * @brief Read manipulator soft startmode
 *
 * @param   hndl      Pointer to session handle
 * @return  Negative value if an error occurred. 0 = disabled or 1 = enabled value otherwise
 */
LIBUM_SHARED_EXPORT int um_get_soft_start_mode(um_state *hndl, const int dev);

/**
 * @brief Get state of a manipulator feature
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @param   id      Feature id
 * @return  Negative value if an error occurred. 0 if feature disabled, 1 if enabled
 */

LIBUM_SHARED_EXPORT int um_get_feature(um_state *hndl, const int dev, const int id);
LIBUM_SHARED_EXPORT int um_get_ext_feature(um_state *hndl, const int dev, const int id);

/**
 * @brief Enable or disable a device feature
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   id        Feature id
 * @param   value     0 to disable and 1 to enable feature
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int um_set_feature(um_state *hndl, const int dev,
                                         const int id, const int value);
LIBUM_SHARED_EXPORT int um_set_ext_feature(um_state *hndl, const int dev,
                                         const int id, const int value);
/**
 * @brief Get state of device feature & feature mask
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 * @param   feature_id Feature id
 * @return  Negative value if an error occurred. 0 if feature disabled, 1 if enabled
 */

LIBUM_SHARED_EXPORT int um_get_feature_functionality(um_state *hndl, const int dev, const int feature_id);

/**
 * @brief Get axis angle, supported by manipulators, not by uMs
 * @param hndl       Pointer to session handle
 * @param dev        Device id
 * @param[out] value Pointer to an allocated variable, angle in degrees with 0.1 degree resolution.
 *                   May be NULL if integer return value with 1 degree resolution is enough
 * @return Negative value if an error occurred, angle in degrees otherwise
 */

LIBUM_SHARED_EXPORT int ump_get_axis_angle(um_state * hndl, const int dev, float *value);

/**
 *
 * @brief Pressure controller (uMc) specific commands
 */

/**
 * @brief Set pressure value - regulator settings value
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   channel   Pressure channel, valid values 1-8
 * @param   value     pressure in kPa, value may be negative
 *
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int umc_set_pressure_setting(um_state *hndl, const int dev,
                                                 const int channel, const float value);
/**
 * @brief Get current pressure regulator settings value
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   channel   Pressure channel, valid values 1-8
 * @param[out] value  Pointer to an allocated variable, will get pressure setting in kPa, may be negative
 *
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int umc_get_pressure_setting(um_state *hndl, const int dev, const int channel, float *value);

/**
 * @brief Set valve stage
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   channel   Pressure channel, valid values 1-8
 * @param   value     0 (user/atmosphere) or 1 (pressure regulator output)
 *
 * @return  Negative value if an error occurred. Zero or positive value otherwise
 */

LIBUM_SHARED_EXPORT int umc_set_valve(um_state *hndl, const int dev, const int channel, const int value);

/**
 * @brief Get current valve state (operation depends on the actual valve type)
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   channel   Pressure channel, valid values 1-8
 *
 * @return  Negative value if an error occurred. 0 (user/atmosphere) or 1 (pressure regulator output)
 */

LIBUM_SHARED_EXPORT int umc_get_valve(um_state *hndl, const int dev, const int channel);

/**
 * @brief Measure real pressure on output manifold
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   channel   Pressure channel, valid values 1-8
 * @param[out] value  Pointer to an allocated variable, will get pressure in kPa, may be negative
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 *
 */

LIBUM_SHARED_EXPORT int umc_measure_pressure(um_state *hndl, const int dev, const int channel, float *value);

/**
 * @brief Get pressure regulator monitor line ADC value
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   channel   Pressure channel, valid values 1-8
 * @return  Negative value if an error occurred. Zero or positive value otherwise carrying ADC reading
 */

LIBUM_SHARED_EXPORT int umc_get_pressure_monitor_adc(um_state *hndl, const int dev, const int channel);

/**
 * @brief Reset/calibrate fluid detector.
 * This function should be called if tube has been replaced or remounted to the detector after cleaning.
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   channel   Pressure channel, valid values 1-8
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int umc_reset_fluid_detector(um_state *hndl, const int dev, const int channel);

/**
 * @brief Read fluid detectors state
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 * Bit map of channels which has detected fluid e.g. 6 for channels 2 and 3
 */

LIBUM_SHARED_EXPORT int umc_read_fluid_detectors(um_state *hndl, const int dev);

/**
 * @brief Read pressure sequence
 *
 * @param      Filename
 * @param[out] sequence Pressure sequence read from the file as an array of integers
 *                      a buffer will be allocated inside this function call,
 *                      buffer will be free'd by start_sequence
 * @return  Negative value if an error occurred. Number items in the sequence otherwise
 */

/**
 * @brief Reset pressure sensor offset
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   chn       Pressure channel 1-8, 0 for all channels
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int umc_reset_sensor_offset(um_state *hndl, const int dev, const int chn);

/**
 * @brief Pressure calibration
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       Device ID
 * @param   chn       Pressure channel 1-8, 0 for all channels
 * @param   delay     Delay between setting and measuring pressure in ms, set to zero to use default value.
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int umc_pressure_calib(um_state *hndl, const int dev, const int chn, const int delay);

/**
 * @brief Get list of manipulators or other compatible devices.
 *        Call to this function attempts to cause fast list update by sending a ping as broadcast
 * @param   hndl      Pointer to session handle
 * @param      size   Size of the device list, number of integers
 * @param[out] devs   Pointer to list of devices found
 *
 *
 * This function should be called in this way
 * int devids[20];
 * int ret = um_cu_get_device_list(handle, devids, 20);
 * for(i = 0; i < ret; i++)
 *    int dev = devids[i]; // do anything to the dev id
 *
 * @return   Negative value if an error occurred, count of found devices otherwise
 */

LIBUM_SHARED_EXPORT int um_get_device_list(um_state *hndl, int *devs, const int size);

/**
 * @brief  Clear SDK internal list of manipulators or other compatible devices,
 *         which are found on current group.
 * @return Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int um_clear_device_list(um_state *hndl);

/**
 * @brief Check if device unicast address is known
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID
 *
 * @return  Negative value if an error occurred (null handle). 0 or 1 otherwise
 */

LIBUM_SHARED_EXPORT int um_has_unicast_address(um_state *hndl, const int dev);

/**
 * @brief uMs-specific commands
 */

/**
 * @brief Set lens changer position
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       uMs Device ID
 * @param   position  position of the objective 0-X, zero for center position -
 *                    nothing seen on microscope, but lens out of way.
 * @param   lift      how much objective is lifted before changing the objective, in um.
 *                    #LIBUM_ARG_UNDEF to use default value stored in uMs eeprom.
 * @param   dip       dip depth in um after objective has been changed, 0 to disable,
 *                    #LIBUM_ARG_UNDEF to use default value stored in uMs eeprom.
 *                    Argument ignored if lift is #LIBUM_ARG_UNDEF.
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int ums_set_lens_position(um_state *hndl, const int dev, const int position,
                                              const float move_away, const float dip);

/**
 * @brief Get lens changer position
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID of UMS
 *
 * @return  Negative value if an error occurred. 0 if position is unknown or at center, 1 or 2 otherwise.
 */

LIBUM_SHARED_EXPORT int ums_get_lens_position(um_state *hndl, const int dev);

/**
 *
 * @brief Objective configuration struct
 *
 */

typedef struct ums_objective_conf_s
{
    int mag;        /**< magnification e.g. 5 or 40 */
    float x_offset; /**< X-axis offset in um */
    float y_offset; /**< Y-axis offset in um */
    float z_offset; /**< Z-axis offset in um */
} ums_objective_conf;

/**
 * @brief Set objective configurations
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       uMs Device ID
 * @param   obj1      objective 1 configuration in struct #ums_objective_conf
 * @param   obj2      objective 1 configuration in struct #ums_objective_conf
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int ums_set_objective_configuration(um_state *hndl, const int dev,
                                                        const ums_objective_conf *obj1,
                                                        const ums_objective_conf *obj2);

/**
 * @brief Get objective configurations
 *
 * @param   hndl      Pointer to session handle
 * @param   dev       uMs Device ID
 * @param[out] obj1      objective 1 configuration in struct #ums_objective_conf
 * @param[out] obj2      objective 1 configuration in struct #ums_objective_conf
 * @return  Negative value if an error occurred. Zero or positive value otherwise.
 */

LIBUM_SHARED_EXPORT int ums_get_objective_configuration(um_state *hndl, const int dev,
                                                        ums_objective_conf *obj1,
                                                        ums_objective_conf *obj2);

// uMs bowl control
#define UMS_BOWL_MAX_COUNT            24 /**< maximum number of bowls on microscope stage supported by commands below */
#define UMS_BOWL_CONTROL_HEADER_SIZE   5 /**< ums_set/get_bowl_control command header size */

typedef struct
{
    float x;             /**< coordinate in um */
    float y;             /**< coordinate in um */
} ums_bowl_center;

typedef struct
{
    int count;           /** number of bowls under microscope stage, zero to disable feature */
    float objective_od;  /** objective outer diameter in um */
    float bowl_id;       /** bowl inner diameter in um */
    float z_limit_low;   /** max safe focus position where XY stage can be moved to any position in um */
    float z_limit_height; /** max safe focus position before objective is touching the bottom of the bowl in um */
} ums_bowl_control;

/**
 * @brief Set uMs bowl controls
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID of UMS
 * @param   control pointer to ums_bowl_control struct where control parameters are populated
 * @param   centers an array of bowl center coordinates of ums_bowl_control.count size
 *
 *          ums_bowl_contr control;
 *          ums_bowl_center centers[2]
 *          control.count = 2;
 *          control.  ... // set also other params in the struct
 *          centers[0].x = 50000.0;
 *          centers[0].y = 60000.0;
 *          centers[1].x = 30000.0;
 *          centers[1].y = 40000.0;
 *          ret = ums_set_bowl_control(hndl, dev, &control, centers);
 *
 * @return  Negative value if an error occurred. 0 if position is unknown or at center, 1-X otherwise.
 */

LIBUM_SHARED_EXPORT int ums_set_bowl_control(um_state *hndl, const int dev, const ums_bowl_control *control, const ums_bowl_center *centers);

/**
 * @brief Get uMs bowl controls
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID of UMS
 * @param   control pointer to ums_bowl_control struct where control parameters read from the uMs will be populated
 * @param   centers an array of bowl center coordinates, reserve space for #UMS_BOWL_MAX_COUNT coordinates
 *
 *          ums_bowl_contr control;
 *          ums_bowl_center centers[UMS_BOWL_MAX_COUNT]
 *          ret = ums_get_bowl_control(hndl, dev, &control, centers);
 *
 * @return  Negative value if an error occurred. Number of bowls (value of control.count) otherwise.
 */

LIBUM_SHARED_EXPORT int ums_get_bowl_control(um_state *hndl, const int dev, ums_bowl_control *control, ums_bowl_center *centers);

/**
 * @brief Set uMs bowl controls
 *
 * @param   hndl    Pointer to session handle
 * @param   dev     Device ID of UMS
 * @param   control pointer to ums_bowl_control struct where control parameters are populated
 * @return  Negative value if an error occurred. 0 if position is unknown or at center, 1-X otherwise.
 */

LIBUM_SHARED_EXPORT int ums_set_bowl_control(um_state *hndl, const int dev, const ums_bowl_control *control, const ums_bowl_center *centers);

/**
 * @brief get millisecond accurate epoch (cross platform compatible way without using any extra library)
 * @return  timestamp
 */

LIBUM_SHARED_EXPORT unsigned long long um_get_timestamp_ms();

/*
 * End of the C-API
 */

#ifdef __cplusplus
} // end of extern "C"

#define LIBUM_USE_LAST_DEV  0     /**< Use the selected device ID */

/*!
 * @class LibUm
 * @brief An inline C++ wrapper class for a public Sensapex uM SDK
 *        not depending on Qt or std classes
*/

class LibUm
{
public:
    /**
     * @brief Constructor
     */
    LibUm() { _handle = NULL; }
    /**
     * @brief Destructor
     */
    virtual ~LibUm()
    {   if(_handle) um_close(_handle); }

    /**
     * @brief Open socket and initialize class state to communicate with manipulators
     * @param broadcastAddress  UDP target address as a string with traditional IPv4 syntax e.g. "169.254.255.255"
     * @param timeout           UDP message timeout in milliseconds
     * @param group             device group, default 0 is group 'A' on TSC
     * @return `true` if operation was successful, `false` otherwise
     */
    bool open(const char *broadcastAddress = LIBUM_DEF_BCAST_ADDRESS, const unsigned int timeout = LIBUM_DEF_TIMEOUT, const int group = 0)
    {	return (_handle = um_open(broadcastAddress, timeout, group)) != NULL; }

    /**
     * @brief Check if socket is open for device communication
     * @return `true` if this instance of `LibUm` holds an open UDP socket.
     */
    bool isOpen()
    { 	return _handle != NULL; }

    /**
     * @brief Close the socket (if open) and free the state structure allocated in open
     */
    void close()
    {	um_close(_handle); _handle = NULL; }

    /**
     * @brief SDK library version
     * @return Pointer to version string
     */
    static const char *version()
    {   return um_get_version(); }

    /**
     * @brief Ping device
     * @param dev   Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool ping(const int dev = LIBUM_USE_LAST_DEV)
    {   return um_ping(_handle, getDev(dev)) >= 0; }

    /**
     * @brief Check if device is busy
     * @param dev   Device ID
     * @return `true` if device is busy, `false` otherwise
     */
    bool busy(const int dev = LIBUM_USE_LAST_DEV)
    {   return um_is_busy(_handle, getDev(dev)) > 0; }

    /**
     * @brief Obtain memory or position drive status
     * @param   dev     Device ID
     * @return  Status of the selected uMp or uMs, #LIBUM_POS_DRIVE_COMPLETED,
     *          #LIBUM_POS_DRIVE_BUSY or #LIBUM_POS_DRIVE_FAILED
     */

    int driveStatus(const int dev = LIBUM_USE_LAST_DEV)
    {   return um_get_drive_status(_handle, getDev(dev)); }

    /**
     * @brief cmdOptions
     * Set options for next cmd to be sent to the device.
     * This is one time set and will be reset after sending the next message.
     * Can be used to set the trigger for next command (e.g. goto position)
     * @param   hndl        Pointer to session handle
     * @param   optionbits  Options bit to set. Use following:
     *  SMCP1_OPT_WAIT_TRIGGER_1 0x00000200 // Set message to be run when triggered by physical trigger line2
     *  SMCP1_OPT_PRIORITY       0x00000100 // Prioritizes message to run first. // 0 = normal message
     *  SMCP1_OPT_REQ_BCAST      0x00000080 // send ACK, RESP or NOTIFY to the bcast address (combine with REQs below), 0 = unicast to the sender
     *  SMCP1_OPT_REQ_NOTIFY     0x00000040 //request notification (e.g. on completed memory drive), 0 = do not notify
     *  SMCP1_OPT_REQ_RESP       0x00000020 // request RESP, 0 = no RESP requested
     *  SMCP1_OPT_REQ_ACK        0x00000010 // request ACK, 0 = no ACK requested
     * @return  returns set flags
     */
    int cmdOptions(const int flags)
    {  return um_cmd_options(_handle, flags); }

    /**
     * @brief Read parameter from the device
     * @param paramId    Parameter id
     * @param[out] value parameter value
     * @param dev        Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool getParam(const int paramId, int *value, const int dev = LIBUM_USE_LAST_DEV)
    {	return  um_get_param(_handle, getDev(dev), paramId, value) >= 0; }

    /**
     * @brief Set parameter to he device
     * @param paramId   Parameter id
     * @param value     Data to be written
     * @param dev       Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool setParam(const int paramId, const short value, const int dev = LIBUM_USE_LAST_DEV)
    {	return  um_set_param(_handle, getDev(dev), paramId, value) >= 0; }

    /**
     * @brief Get device feature state
     * @param featureId  feature id
     * @param[out] value value
     * @param dev        Device ID
     *
     * @return `true` if operation was successful, `false` otherwise
     */
    bool getFeature(const int featureId, bool *value, const int dev = LIBUM_USE_LAST_DEV)
    {
        int ret;
        if((ret = um_get_feature(_handle, getDev(dev), featureId)) < 0)
            return false;
        *value = ret > 0;
        return true;
    }

    bool getExtFeature(const int featureId, bool *value, const int dev = LIBUM_USE_LAST_DEV)
    {
        int ret;
        if((ret = um_get_ext_feature(_handle, getDev(dev), featureId)) < 0)
            return false;
        *value = ret > 0;
        return true;
    }

    /**
     * @brief Enable or disable feature
     * @param featureId  feature id
     * @param state      enable or disable
     * @param dev        Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool setFeature(const int featureId, const bool state, const int dev = LIBUM_USE_LAST_DEV)
    {	return  um_set_feature(_handle, getDev(dev), featureId, state) >= 0; }

    /**
     * @brief Enable or disable extended feature
     * @param featureId  feature id
     * @param state      enable or disable
     * @param dev        Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool setExtFeature(const int featureId, const bool state, const int dev = LIBUM_USE_LAST_DEV)
    {	return  um_set_ext_feature(_handle, getDev(dev), featureId, state) >= 0; }

    /**
     * @brief Obtain the position of actuators.
     * @param x     Pointer to x-actuator position (may be NULL)
     * @param y     Pointer to y-actuator position (may be NULL)
     * @param z     Pointer to z-actuator position (may be NULL)
     * @param w     Pointer to w-actuator position (may be NULL)
     * @param dev   Device ID
     * @param       timeLimit  Timelimit of cache values. If `timeLimit` is 0 then
     * cached positions are used always. If `timeLimit` is #LIBUM_TIMELIMIT_DISABLED
     * then positions are always read from uMp or uMs bypassing the cache.
     * @return `true` if operation was successful, `false` otherwise
     */
    bool getPositions(float *x, float *y, float *z, float *w,
                      const int dev = LIBUM_USE_LAST_DEV,
                      const unsigned int timeLimit = LIBUM_DEF_REFRESH_TIME)
    {   return um_get_positions(_handle, getDev(dev), timeLimit, x, y, z, w, NULL) >= 0; }


    /**
     * @brief Move actuators to given position
     * @param x      Destination position for x-actuator, use #LIBUM_ARG_UNDEF for axis not to be moved
     * @param y      Destination position for y-actuator in um
     * @param z      Destination position for z-actuator
     * @param w      Destination position for w-actuator
     * @param speed  Movement speed in um/s(0 to use default value)
     * @param dev    Device ID (#LIBUM_USE_LAST_DEV to use selected one)
     * @param allAxisSimultaneously Drive mode (default one-by-one)
     * @param max_acc Maximum acceleration in um/s^2, give value zero to use default
     *
     * @return `true` if operation was successful, `false` otherwise
     */
    bool gotoPos(const float x, const float y, const float z, const float w,
                 const float speed,  const int dev = LIBUM_USE_LAST_DEV,
                 const bool allAxisSimultaneously = false,
                 const int max_acc = 0)
    {   return um_goto_position(_handle, getDev(dev), x, y, z, w, speed, allAxisSimultaneously, max_acc) >= 0; }

    /**
     * @brief Stop device - typically movement, but also uMc calibration
     * @param dev   Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool stop(const int dev = LIBUM_USE_LAST_DEV)
    {   return um_stop(_handle, getDev(dev)) >= 0; }

    /**
     * @brief Get the latest error code
     * @return #um_error error code
     */
    um_error lastError()
    {	return um_last_error(_handle); }

    /**
     * @brief Get the latest error description
     * @return Pointer to error description
     */
    const char *lastErrorText()
    { 	return um_last_errorstr(_handle); }

    /**
     * @brief Get device firmware version
     * @param[out] version  Pointer to an allocated buffer for firmware version numbers
     * @param      size     Size of the above buffer (number of integers)
     * @param      dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool readVersion(int *version, const int size, const int dev = LIBUM_USE_LAST_DEV)
    {   return um_read_version(_handle, getDev(dev), version, size) >= 0; }

    /**
     * @brief Get list of devices on current network
     *
     * @param[out] devs   Pointer to list of devices found
     * @param      size     Size of the above buffer (number of integers)
     * @return number of found devices
     */
    int getDeviceList(int *devs = NULL, const int size = 0)
    {   return um_get_device_list(_handle, devs, size); }

    /**
      * @brief Clear above device list
      * @return `true` if operation was successful, `false` otherwise
      */
    bool clearDeviceList()
    {  return um_clear_device_list(_handle) >= 0; }

    /**
     * @brief Get uMp or uMs axis count
     * @param      dev      Device ID
     * @return  Negative value if an error occurred. Axis count otherwise
     */
    int getAxisCount(const int dev = LIBUM_USE_LAST_DEV)
    {   return um_get_axis_count(_handle, getDev(dev)); }

    /**
     * @brief Take a step (relative movement from current position)
     * @param   x,y,z,d  step length (in um), negative value for backward, zero for axis not to be moved
     * @param   speed    movement speed in um/s for all axis, zero to use default value
     * @param   dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool takeStep(const float x, const float y = 0, const float z = 0, const float d = 0,
                  const int speed = 0, const int dev = LIBUM_USE_LAST_DEV)

    {   return um_take_step(_handle, getDev(dev), x, y, z, d, speed, speed, speed, speed, 0, 0) >= 0; }

    /**
     * @brief Take a step (relative movement from current position) with separate speed for every axis
     * @param   step_x   step length (in um) for X axis negative value for backward, zero for axis not to be moved
     * @param   step_y   step length (in um) for Y axis negative value for backward, zero for axis not to be moved
     * @param   step_z   step length (in um) for Z axis negative value for backward, zero for axis not to be moved
     * @param   step_d   step length (in um) for D axis negative value for backward, zero for axis not to be moved
     * @param   speed_x  movement speed in um/s for X axis, zero to use default value
     * @param   speed_y  movement speed in um/s for Y axis, zero to use default value
     * @param   speed_z  movement speed in um/s for Z axis, zero to use default value
     * @param   speed_d  movement speed in um/s for D axis, zero to use default value
     * @param   mode     (CLS or microstepping resolution) mode
     * @param   max_acceleration
     * @param   dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool takeStep(const int step_x, const int step_y, const int step_z, const int step_d,
                  const int speed_x, const int speed_y, const int speed_z,
                  const int speed_d, const int mode = 0, const int max_acceleration = 0,
                  const int dev = LIBUM_USE_LAST_DEV)
    {   return um_take_step(_handle, getDev(dev), step_x, step_y, step_z, step_d,
                            speed_x, speed_y, speed_z, speed_d,
                            mode, max_acceleration) >= 0; }

     /**
     * @brief uMp LED control
     * @param disable  true to turn all manipulator LEDs off, false to restore normal operation
     * @param dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
     bool umpLEDcontrol(const bool disable, const int dev = LIBUM_USE_LAST_DEV)
     {  return ump_led_control(_handle, getDev(dev), disable) >= 0; }


     /**
     * @brief uMs set lens position
     * @param position 1 or 2
     * @param dev      Device ID
     * @param lift     how much objective is lifted before changing the objective, in um.
     *                 default #LIBUM_ARG_UNDEF to use value stored in uMs eeprom (recommended).
     * @param dip      dip depth in um after objective has been changed, 0 to disable,
     *                 default #LIBUM_ARG_UNDEF to use value stored in uMs eeprom (recommended).
     *                 Argument ignored if lift is #LIBUM_ARG_UNDEF.
     * @return `true`  if operation was successful, `false` otherwise
     */
     bool umsSetLensPosition(const int position, const int dev = LIBUM_USE_LAST_DEV,
                             const float lift = LIBUM_ARG_UNDEF, const float dip = LIBUM_ARG_UNDEF)
     {  return ums_set_lens_position(_handle, getDev(dev), position, lift, dip) >= 0; }

     /**
     * @brief uMs get lens position
     * @param dev      Device ID
     * @return 1 or 2, 0 if position is unknown and negative for an error
     */
     int umsGetLensPosition(const int dev = LIBUM_USE_LAST_DEV)
     {  return ums_get_lens_position(_handle, getDev(dev)); }

    /**
     * @brief Set pressure
     * @param channel  1-8
     * @param value    pressure value in kPa
     * @param dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool umcSetPressure(const int channel, const float value, const int dev = LIBUM_USE_LAST_DEV)
    {	return umc_set_pressure_setting(_handle, getDev(dev), channel, value) >= 0; }

    /**
     * @brief Get pressure setting
     * @param channel  1-8
     * @param[out]     Pressure in kPa
     * @param dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool umcGetPressure(const int channel, float *value, const int dev = LIBUM_USE_LAST_DEV)
    {	return umc_get_pressure_setting(_handle, getDev(dev), channel, value) >= 0; }

    /**
     * @brief Measure pressure
     * @param channel  1-8
     * @param[out]     Pressure in kPa
     * @param dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool umcMeasurePressure(const int channel, float *value, const int dev = LIBUM_USE_LAST_DEV)
    {	return umc_measure_pressure(_handle, getDev(dev), channel, value) >= 0; }

    /**
     * @brief Set valve
     * @param channel  1-8
     * @param state    true or false
     * @param dev      Device ID
     * @return `true` if operation was successful, `false` otherwise
     */
    bool umcSetValve(const int channel, const bool state, const int dev = LIBUM_USE_LAST_DEV)
    {	return umc_set_valve(_handle, getDev(dev), channel, state) >= 0; }

    /**
     * @brief Get valve state
     * @param channel  1-8
     * @param dev      Device ID
     * @return negative if an error occurred, valve state 0 or 1 otherwise
     */

    int umcGetValve(const int channel, const int dev = LIBUM_USE_LAST_DEV)
    {	return umc_get_valve(_handle, getDev(dev), channel); }

    /**
    * @brief Reset/calibrate fluid detector.
    * This function should be called if tube has been replaced or remounted to the detector after cleaning.
    * First uMc units had a separate control line for each channel. Later ones has a shared line for all channels.
    *
    * Fluid detector is an optional accessory.
    *
    * @param   channel   Pressure channel, valid values 1-8
    * @param   dev       Device ID
    * @return `true` if operation was successful, `false` otherwise
    */

    bool umcResetFluidDetector(const int channel = 1, const int dev = LIBUM_USE_LAST_DEV)
    {   return umc_reset_fluid_detector(_handle, getDev(dev), channel); }

    /**
     * @brief Get state of fluid detectors
     *
     * @param dev      Device ID
     * @return negative if an error occurred, detector state otherwise
     */

    int umcReadFluidDetectors(const int dev = LIBUM_USE_LAST_DEV)
    {   return umc_read_fluid_detectors(_handle, getDev(dev)); }

    /**
     * @brief Pressure calibration
     *
     * @param   channeln  Pressure channel 1-8, 0 for all channels
     * @param   delay     Delay between setting and measuring pressure in ms, set to zero to use default value.
     * @param   dev       Device ID
     *
     * @return `true` if operation was successful, `false` otherwise
     */

     int umcCalibratePressure(const int channel, const int delay = 0, const int dev = LIBUM_USE_LAST_DEV)
     {  return umc_pressure_calib(_handle, getDev(dev), channel, delay); }

    /**
     * @brief Get C-API handle
     * @return pointer to #um_state handle
     */
    um_state *getHandle()
    {   return _handle; }

    /**
     * @brief Check that the device's unicast address is known
     * @param   dev   Device ID
     * @return `true` if device's unicast address is known, `false` otherwise
     */
    bool hasUnicastAddress(const int dev = LIBUM_USE_LAST_DEV)
    {   return um_has_unicast_address(_handle, getDev(dev)) > 0; }

    /**
     * @brief Set up external log print function by default the library writes
     *        to the stderr if verbose level is higher than zero.
     *
     * @param   hndl            Pointer to session handle
     * @param   verbose_level   Verbose level (zero to disable, higher value for more detailed printouts)
     * @param   func            Pointer to the custom log print function.
     *                          May be NULL if setting only verbose level for internal log print out to stderr
     * @param   arg             Pointer argument to be looped to the above function may be e.g. a typecasted
     *                          file handle, optional, may be NULL
     * @return  Negative value if an error occurred. Zero or positive value otherwise
     */
    bool setLogCallback(const int verbose_level, um_log_print_func func, const void *arg)
    {	return um_set_log_func(_handle, verbose_level, func, arg); }

    /**
     * @brief Process incoming messages (may update status or location cache)
     * @return number of messages received
     */
    int recv(const int timelimit)
    {   return um_receive(_handle, timelimit); }

private:
    /**
     * @brief Resolves device ID, #LIBUM_USE_LAST_DEV handled in a special way
     * @param  dev    Device ID
     * @return Device ID
     */
    int getDev(const int dev)
    {
        if(dev == LIBUM_USE_LAST_DEV && _handle)
            return _handle->last_device_sent;
        return dev;
    }

    /**
     * @brief Session handle
     */
    um_state *_handle;
};

#endif // C++
#endif // LIBUM_H

