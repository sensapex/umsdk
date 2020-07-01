/*
 * A sample C-program for Sensapex micromanipulator SDK (umpsdk)
 *
 * Copyright (c) 2016-2020, Sensapex Oy
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
#include "libum.h"

#define VERSION_STR   "v0.121"
#define COPYRIGHT "Copyright (c) Sensapex. All rights reserved"

#define DEV     1
#define UPDATE  200

typedef struct params_s
{
    float x, y, z, d, X, Y, Z, D;
    int verbose, update, loop, dev, speed;
    char *address;
} params_struct;

void usage(char **argv)
{
    fprintf(stderr,"usage: %s [opts]\n", argv[0]);
    fprintf(stderr,"Generic options\n");
    fprintf(stderr,"-d\tdev (def: %d)\n", DEV);
    fprintf(stderr,"-v\tverbose\n");
    fprintf(stderr,"-a\taddress (def: %s)\n", LIBUM_DEF_BCAST_ADDRESS);
    fprintf(stderr,"-u\tposition and status update period (def: %d ms)\n", UPDATE);
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
    params->dev = DEV;
    params->update = UPDATE;
    params->address = LIBUM_DEF_BCAST_ADDRESS;
    for(i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 'h':
                usage(argv);
                break;
            case 'v':
                params->verbose++;
                break;
            case '1':
                params->verbose = 0;
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
            default:
                usage(argv);
                break;
            }
        }
        else
            usage(argv);
    }
}

int main(int argc, char *argv[])
{
    um_state *handle = NULL;
    int ret, status, loop = 0;
    float home_x = 0.0, home_y = 0.0, home_z = 0.0, home_d = 0.0;
    params_struct params;

    parse_args(argc, argv, &params);

    if((handle = um_open(params.address, LIBUM_DEF_TIMEOUT)) == NULL)
    {
        // Feeding NULL handle is intentional, it obtains the
        // last OS error which prevented the port to be opened
        fprintf(stderr, "Open failed - %s\n", um_last_errorstr(handle));
        exit(1);
    }

    if(um_ping(handle, params.dev) <0)
    {
        fprintf(stderr, "Pinging dev failed - %s\n", um_last_errorstr(handle));
        um_close(handle);
        exit(2);
    }

    float angle;
    if(ump_get_axis_angle(handle, params.dev, &angle) < 0)
        fprintf(stderr, "Get axis angle failed - %s\n", um_last_errorstr(handle));
    else
        printf("Axis angle %1.1f degree\n", angle);

    if(um_get_positions(handle, params.dev, params.update, &home_x, &home_y, &home_z, &home_d, NULL) < 0)
        fprintf(stderr, "Get positions failed - %s\n", um_last_errorstr(handle));
    else
        printf("Current position: %3.2f %3.2f %3.2f %3.2f\n", home_x, home_y, home_z, home_d);

    do
    {
        float x, y, z, d;

        if(params.X != LIBUM_ARG_UNDEF)
            x = (loop&1)?home_x:params.X;
        else if(params.x)
            x = home_x + (loop+1)*params.x;
        else
            x = LIBUM_ARG_UNDEF;

        if(params.Y != LIBUM_ARG_UNDEF)
            y = (loop&1)?home_y:params.Y;
        else if(params.y)
            y = home_y + (loop+1)*params.y;
        else
            y = LIBUM_ARG_UNDEF;

        if(params.Z != LIBUM_ARG_UNDEF)
            z = (loop&1)?home_z:params.Z;
        else if(params.z)
            z = home_z + (loop+1)*params.z;
        else
            z = LIBUM_ARG_UNDEF;

        if(params.D != LIBUM_ARG_UNDEF)
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
