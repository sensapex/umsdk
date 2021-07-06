/*
 * A sample C-program for Sensapex micromanipulator SDK (umpsdk)
 *
 * Copyright (c) 2016-2021, Sensapex Oy
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/timeb.h>
#include <stdint.h>
#include <stdbool.h>
#include "libum.h"

#define VERSION_STR   "v0.123"
#define COPYRIGHT "Copyright (c) Sensapex. All rights reserved"

#define DEV     1
#define UPDATE  200

typedef struct params_s
{
    float x, y, z, d, X, Y, Z, D, pressure_kpa;
    int verbose, update, loop, dev, speed, timeout, value, dim_leds, group;
    int calibrate_pressure, pressure_channel, valve_channel, reset_fluid_detector, pressure_sensor;
    bool lens_position, read_fluid_detectors;
    char *address;
} params_struct;

void usage(char **argv)
{
    fprintf(stderr,"usage: %s [opts]\n", argv[0]);
    fprintf(stderr,"Generic options\n");
    fprintf(stderr,"-d\tdev (def: %d)\n", DEV);
    fprintf(stderr,"-g\tgroup (def: 0)\n");
    fprintf(stderr,"-e\tverbose\n");
    fprintf(stderr,"-a\taddress (def: %s)\n", LIBUM_DEF_BCAST_ADDRESS);
    fprintf(stderr,"-u\tposition and status update period (def: %d ms)\n", UPDATE);
    fprintf(stderr,"-t\tcommand timeout (def: %d ms)\n", LIBUM_DEF_TIMEOUT);
    fprintf(stderr,"Position change\n");
    fprintf(stderr,"-x\trelative target (um, decimal value accepted, negative value for backward)\n");
    fprintf(stderr,"-y\trelative target \n");
    fprintf(stderr,"-z\trelative target \n");
    fprintf(stderr,"-w\trelative target for the 4th axis\n");
    fprintf(stderr,"-X\tabs target (um, decimal value accepted\n");
    fprintf(stderr,"-Y\tabs target \n");
    fprintf(stderr,"-Z\tabs target \n");
    fprintf(stderr,"-W\tabs target for the 4th axis\n");
    fprintf(stderr,"-D\tabs target for the 4th axis, alias for above\n");
    fprintf(stderr,"-n\tcount\tloop between current and target positions or take multiple relative steps into same direction\n");

    fprintf(stderr,"Get or set lens changer position\n");
    fprintf(stderr,"-L\t\tto get or set lens position, set if -v defined\n");
    fprintf(stderr,"-v\tvalue 0 for center, 1-X for positions\n");

    fprintf(stderr,"Get or set uMc controls (give -v value to set)\n");
    fprintf(stderr,"-C\tchn\tpressure channel 1-8 (0 to disable)\n");
    fprintf(stderr,"-v\tvalue\tkPa, range -70.0 - +70.0\n");
    fprintf(stderr,"-V\tchn\tvalve channel 1-8 (0 to disable)\n");
    fprintf(stderr,"-v\tvalue\t0 or 1\n");
    fprintf(stderr,"-R\tchn\tread pressure sensor, 1-8 (0 to disable)\n");
    fprintf(stderr,"-r\tchn\treset/calibrate fluid detectors 1-8 (0 to disable)\n");
    fprintf(stderr,"-U\t\tread fluid detectors\n");
    fprintf(stderr,"-B\tchn\tpressure channel to be calibrated\n");
    fprintf(stderr,"uMp controls\n");
    fprintf(stderr,"-l\t0|1\t1 to dim manipulator LEDs. 0 to restore normal functionality\n");
    exit(1);
}

// Exits via usage() if an error occurs
void parse_args(int argc, char *argv[], params_struct *params)
{
    int i, v;
    float f;
    memset(params, 0, sizeof(params_struct));
    params->X = LIBUM_ARG_UNDEF;
    params->Y = LIBUM_ARG_UNDEF;
    params->Z = LIBUM_ARG_UNDEF;
    params->D = LIBUM_ARG_UNDEF;
    params->pressure_kpa = LIBUM_ARG_UNDEF;
    params->dev = DEV;
    params->value = -1;
    params->dim_leds = -1;
    params->update = UPDATE;
    params->address = LIBUM_DEF_BCAST_ADDRESS;
    params->timeout = LIBUM_DEF_TIMEOUT;
    for(i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'h':
                usage(argv);
                break;
            case 'e':
                params->verbose++;
                break;
            case '1':
                params->verbose = 0;
                break;
            case 'L':
                params->lens_position = true;
                break;
            case 'n':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v > 0)
                    params->loop = v;
                else
                    usage(argv);
                break;
            case 'u':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v > 0)
                    params->update = v;
                else
                    usage(argv);
                break;
            case 'd':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v > 0)
                    params->dev = v;
                else
                    usage(argv);
                break;
            case 'g':
                if (i < argc - 1 && sscanf(argv[++i],"%d", &v) == 1 && v > 0)
                    params->group = v;
                else
                    usage(argv);
                break;
            case 't':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v > 0)
                    params->timeout = v;
                else
                    usage(argv);
                break;
            case 's':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v > 0)
                    params->speed = v;
                else
                    usage(argv);
                break;
            case 'x':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1)
                    params->x = f;
                else
                    usage(argv);
                break;
            case 'y':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1)
                    params->y = f;
                else
                    usage(argv);
                break;
            case 'z':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1)
                    params->z = f;
                else
                    usage(argv);
                break;
            case 'w':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1)
                    params->d = f;
                else
                    usage(argv);
                break;
            case 'X':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1 && f >= 0)
                    params->X = f;
                else
                    usage(argv);
                break;
            case 'Y':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1 && f >= 0)
                    params->Y = f;
                else
                    usage(argv);
                break;
            case 'Z':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1 && f >= 0)
                    params->Z = f;
                else
                    usage(argv);
                break;
            case 'D':
            case 'W':
                if(i < argc-1 && sscanf(argv[++i],"%f",&f) == 1 && f >= 0)
                    params->D = f;
                else
                    usage(argv);
                break;
            case 'a':
                if(i < argc-1 && argv[i+1][0] != '-')
                    params->address = argv[++i];
                else
                    usage(argv);
                break;

            case 'v':
                if(i < argc-1 && (sscanf(argv[++i],"0x%x",&v) == 1 ||
                                  sscanf(argv[i],"%d",&v) == 1))
                    params->value = v;
                else
                    usage(argv);
                if(params->pressure_channel && sscanf(argv[i], "%f", &f) == 1)
                    params->pressure_kpa = f;
                break;

            // uMc additions
            case 'C':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v >= 1 && v <= 8)
                    params->pressure_channel = v;
                else
                    usage(argv);
                break;
            case 'U':
                params->read_fluid_detectors = true;
                break;
            case 'V':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v >= 1 && v <= 8)
                    params->valve_channel = v;
                else
                    usage(argv);
                break;
            case 'R':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v >= 1 && v <= 8)
                    params->pressure_sensor = v;
                else
                    usage(argv);
                break;
            case 'B':
                if(i < argc-1 && sscanf(argv[++i],"%d",&v) == 1 && v >= 1 && v <= 8)
                    params->calibrate_pressure = v;
                else
                    usage(argv);
                break;
            case 'r':
                if(sscanf(argv[++i],"%d",&v) == 1 && v >= 0 && v <= 8)
                    params->reset_fluid_detector = v;
                else
                    usage(argv);
                break;
            case 'l':
                if(sscanf(argv[++i],"%d",&v) == 1 && v >= 0 && v <= 1)
                    params->dim_leds = v;
                else
                    usage(argv);
                break;
            default:
                usage(argv);
                break;
            }
        }
        else
            usage(argv);
    }
}

#if defined(_WINDOWS) || (defined(__APPLE__) && defined(__MACH__))
static bool isnanf(const float arg)
{
    return isnan(arg);
}
#endif

int main(int argc, char *argv[])
{
    um_state *handle = NULL;
    int ret, status, loop = 0;
    float pressure_kpa, home_x = 0.0, home_y = 0.0, home_z = 0.0, home_d = 0.0;
    params_struct params;

    parse_args(argc, argv, &params);

    if((handle = um_open(params.address, params.timeout, params.group)) == NULL)
    {
        // Feeding NULL handle is intentional, it obtains the
        // last OS error which prevented the port to be opened
        fprintf(stderr, "Open failed - %s\n", um_last_errorstr(handle));
        exit(1);
    }

    // uMs features, get or set lens/objective position
    if(params.lens_position)
    {
        if((ret = ums_get_lens_position(handle, params.dev)) < 0)
            fprintf(stderr, "Get lens position failed - %s\n", um_last_errorstr(handle));
        else
        {
            if(params.value >= 0)
            {
                printf("Lens position was %d\n", ret);
                // 5mm move away, no dip
                if((ret = ums_set_lens_position(handle, params.dev,  params.value, 5000, 0)) < 0)
                    fprintf(stderr, "Set lens position failed - %s\n", um_last_errorstr(handle));
                else
                    printf("Lens position now %d\n", params.value);
            }
            else
                printf("Lens position %d\n", ret);
        }
        um_close(handle);
        exit(ret>=0?0:ret);
    }

    //uMc features
    if(params.pressure_channel)
    {
        do
        {
            if((ret = umc_get_pressure_setting(handle, params.dev, params.pressure_channel, &pressure_kpa)) < 0)
                fprintf(stderr, "Get pressure setting failed - %s\n", um_last_errorstr(handle));
            else
                printf("Channel %d pressure setting: %1.3f kPa\n", params.pressure_channel, pressure_kpa);
            if(!params.loop)
                break;
            if(loop < params.loop-1)
                um_receive(handle, params.update);
        }
        while(++loop < params.loop);

        if(ret >= 0 && !isnan(params.pressure_kpa))
        {
            if((ret = umc_set_pressure_setting(handle, params.dev, params.pressure_channel, params.pressure_kpa)) < 0)
                fprintf(stderr, "Set pressure failed - %s\n", um_last_errorstr(handle));
            else
                printf("Channel %d pressure set: %1.3f kPa\n", params.pressure_channel, params.pressure_kpa);
        }
        um_close(handle);
        exit(ret>=0?0:ret);
    }

    if(params.valve_channel)
    {
        do
        {
            if((ret = umc_get_valve(handle, params.dev, params.valve_channel)) < 0)
                fprintf(stderr, "Get valve failed - %s\n", um_last_errorstr(handle));
            else
                printf("%d: Channel %d valve: %d\n", loop, params.valve_channel, ret);
            if(!params.loop)
                break;
            if(loop < params.loop-1)
                um_receive(handle, params.update);
        }
        while(++loop < params.loop);

        if(ret >= 0 && params.value >= 0)
        {
            if((ret = umc_set_valve(handle, params.dev, params.valve_channel, params.value)) < 0)
                fprintf(stderr, "Set valve failed - %s\n", um_last_errorstr(handle));
            else
                printf("Channel %d valve set: %d\n", params.valve_channel, params.value);
        }
        um_close(handle);
        exit(ret>=0?0:-ret);
    }

    if(params.pressure_sensor)
    {
        do
        {
            if((ret = umc_measure_pressure(handle, params.dev, params.pressure_sensor, &pressure_kpa)) < 0)
                fprintf(stderr, "Get pressure value failed - %s\n", um_last_errorstr(handle));
            else
                printf("%d: Channel %d pressure %1.3f kPa\n", loop, params.pressure_sensor, pressure_kpa);
            if(!params.loop)
                break;
            if(loop < params.loop-1)
                um_receive(handle, params.update);
        }
        while(++loop < params.loop);
        um_close(handle);
        exit(ret>=0?0:-ret);
    }

    if(params.read_fluid_detectors)
    {
        do
        {
            if((ret = umc_read_fluid_detectors(handle, params.dev)) < 0)
                fprintf(stderr, "Read fluid detectors failed - %s\n", um_last_errorstr(handle));
            else
                printf("%d: Fluid detectors 0x%04x\n", loop, ret);
            if(!params.loop)
                break;
            if(loop < params.loop-1)
                um_receive(handle, params.update);
        }
        while(++loop < params.loop);
        um_close(handle);
        exit(ret>=0?0:-ret);
    }


    if(params.reset_fluid_detector)
    {
        if((ret = umc_reset_fluid_detector(handle, params.dev, params.reset_fluid_detector)) < 0)
            fprintf(stderr, "reset fluid detector failed - %s\n", um_last_errorstr(handle));
        else
            printf("Channel %d fluid detector reset\n", params.reset_fluid_detector);
        um_close(handle);
        exit(ret>=0?0:-ret);
    }

    if(params.read_fluid_detectors)
    {
        do
        {
            if((ret = umc_read_fluid_detectors(handle, params.dev)) < 0)
                fprintf(stderr, "Read fluid detectors failed - %s\n", um_last_errorstr(handle));
            else
                printf("%d: Fluid detectors 0x%04x\n", loop, ret);
            if(!params.loop)
                break;
            if(loop < params.loop-1)
                um_receive(handle, params.update);
        }
        while(++loop < params.loop);
        um_close(handle);
        exit(ret>=0?0:-ret);
    }
    if(params.calibrate_pressure)
    {
        do
        {
            ret = umc_pressure_calib(handle, params.dev, params.calibrate_pressure, params.update);
            if(!params.loop)
                break;
            if(loop < params.loop-1)
                um_receive(handle, params.update);
            else
                printf("Channel %d calibration started\n", params.reset_fluid_detector);
        }
        while(++loop < params.loop);
        um_close(handle);
        exit(ret>=0?0:ret);
    }
    if(params.dim_leds >= 0)
    {
        if((ret = ump_led_control(handle, params.dev, params.dim_leds)) < 0)
            fprintf(stderr, "Manipulator LED control failed - %s\n", um_last_errorstr(handle));
        else
            printf("Manipulator %d LEDs %s\n", params.dev, params.dim_leds?"OFF":"ON");
        um_close(handle);
        exit(ret>=0?0:ret);
    }

    // uMp or uMs goto position commands - or just reading current position
    if(um_get_positions(handle, params.dev, params.update, &home_x, &home_y, &home_z, &home_d, NULL) < 0)
    {
        fprintf(stderr, "Get positions failed - %s\n", um_last_errorstr(handle));
        um_close(handle);
        exit(2);
    }
    else
        printf("Current position: %3.2f %3.2f %3.2f %3.2f\n", home_x, home_y, home_z, home_d);

    do
    {
        float x, y, z, d;

        if(!isnanf(params.X))
            x = (loop&1)?home_x:params.X;
        else if(params.x)
            x = home_x + (loop+1)*params.x;
        else
            x = LIBUM_ARG_UNDEF;

        if(!isnanf(params.Y))
            y = (loop&1)?home_y:params.Y;
        else if(params.y)
            y = home_y + (loop+1)*params.y;
        else
            y = LIBUM_ARG_UNDEF;

        if(!isnanf(params.Z))
            z = (loop&1)?home_z:params.Z;
        else if(params.z)
            z = home_z + (loop+1)*params.z;
        else
            z = LIBUM_ARG_UNDEF;

        if(!isnanf(params.D))
            d = (loop&1)?home_d:params.D;
        else if(params.d)
            d = home_d + (loop+1)*params.d;
        else
            d = LIBUM_ARG_UNDEF;

        if(params.loop)
            printf("Target position: %3.2f %3.2f %3.2f %3.2f (%d/%d)\n", x, y, z, d, loop+1, params.loop);
        else
            printf("Target position: %3.2f %3.2f %3.2f %3.2f\n", x, y, z, d);

        if((ret = um_goto_position(handle, params.dev, x, y, z, d, params.speed, 0, 0)) < 0)
        {
            fprintf(stderr, "Goto position failed - %s\n", um_last_errorstr(handle));
            continue;
        }
        if(!params.loop && !params.verbose)
            break;
        do
        {
            um_receive(handle, params.update);
            status = um_get_drive_status(handle, params.dev);
            if(params.verbose)
            {
                if(status < 0)
                    fprintf(stderr, "Status read failed - %s\n", um_last_errorstr(handle));
                else if(um_get_positions(handle, params.dev, params.update, &x, &y, &z, &d, NULL) < 0)
                    fprintf(stderr, "Get positions failed - %s\n", um_last_errorstr(handle));
                else
                    printf("%3.2f %3.2f %3.2f %3.2f status %02X\n", x, y, z, d, status);
            }
        }
        while(status == LIBUM_POS_DRIVE_BUSY);
    } while(++loop < params.loop);
    um_close(handle);
    exit(!ret);
}

