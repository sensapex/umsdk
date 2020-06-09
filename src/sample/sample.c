/*
 * A sample C-program for Sensapex micromanipulator SDK (umsdk)
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
#include "libum.h"

#define VERSION_STR   "v0.122"
#define COPYRIGHT "Copyright (c) Sensapex. All rights reserved"

#define DEV     1
#define UNDEF  (-1)
#define UPDATE  200

typedef struct params_s
{
    float x, y, z, d, X, Y, Z, D;
    int verbose, update, loop, dev, speed;
    char *address;
} params_struct;

void usage(char **argv)
{
    fprintf(stderr,"usage: %s [opts]\n",argv[0]);
    fprintf(stderr,"Generic options\n");
    fprintf(stderr,"-d\tdev (def: %d)\n", DEV);
    fprintf(stderr,"-v\tverbose\n");
    fprintf(stderr,"-a\taddress (def: %s)\n", LIBUM_DEF_BCAST_ADDRESS);
    fprintf(stderr,"Position change\n");
    fprintf(stderr,"-x\trelative target (um, decimal value accepted)\n");
    fprintf(stderr,"-y\trelative target\n");
    fprintf(stderr,"-z\trelative target\n");
    fprintf(stderr,"-d\trelative target\n");
    fprintf(stderr,"-X\tabs target (um, decimal value accepted)\n");
    fprintf(stderr,"-Y\tabs target\n");
    fprintf(stderr,"-Z\tabs target\n");
    fprintf(stderr,"-D\tabs target\n");
    fprintf(stderr,"-n\tcount\tloop current and target positions\n");
    exit(1);
}

// Exits via usage() if an error occurs
void parse_args(int argc, char *argv[], params_struct *params)
{
    int i, v;
    float f;
    memset(params, 0, sizeof(params_struct));
    params->X = UNDEF;
    params->Y = UNDEF;
    params->Z = UNDEF;
    params->D = UNDEF;
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
    int ret, loop = 0;
    float target_x = 0, target_y = 0, target_z = 0, target_d = 0;
    float home_x = 0, home_y = 0, home_z = 0, home_d = 0;
    um_state *handle;
    params_struct params;

    parse_args(argc, argv, &params);

    if((handle = um_open(params.address, 100)) == NULL)
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

    printf("Axis count %d\n", um_get_axis_count(handle, params.dev));

    /*
     * These functions providing the axis position as return value are convenient e.g. for mathlab usage.
     * For C code use ump_get_positions(handle, dev, time_limit, &x, &y, &z, &w, &elapsed)
     *
     * First read the position from the manipulator (or check that cache inside SDK contains new and valid values)
     */

    if(um_read_positions(handle, params.dev, params.update) < 0)
    {
        fprintf(stderr, "read positions failed - %s\n", um_last_errorstr(handle));
        um_close(handle);
        exit(3);
    }
    // next obtain the position values
    home_x = um_get_position(handle, params.dev, 'x');
    home_y = um_get_position(handle, params.dev, 'y');
    home_z = um_get_position(handle, params.dev, 'z');
    home_d = um_get_position(handle, params.dev, 'd');

    printf("Current position: %3.2f %3.2f %3.2f %3.2f\n", home_x, home_y, home_z, home_d);

    // Calculate target positions, relative
    if(params.x)
        target_x = home_x + params.x;
    if(params.y)
        target_y = home_y + params.y;
    if(params.z)
        target_z = home_z + params.z;
    if(params.d)
        target_d = home_d + params.d;
    // or absolutely
    if(params.X != UNDEF)
        target_x = params.X;
    if(params.Y != UNDEF)
        target_y = params.Y;
    if(params.Z != UNDEF)
        target_z = params.Z;
    if(params.D != UNDEF)
        target_d = params.D;

    do
    {
        float x, y, z, d;
        if(loop&1)
            x = home_x, y = home_y, z = home_z, d = home_d;
        else
            x = target_x, y = target_y, z = target_z, d = target_d;
        if(params.loop)
            printf("Target position: %3.2f %3.2f %3.2f %3.2f (%d/%d)\n", x, y, z, d, loop+1, params.loop);
        else
            printf("Target position: %3.2f %3.2f %3.2f %3.2f\n", x, y, z, d);

        // Movement mode and acceleration using device default values 0
        if((ret = um_goto_position(handle, params.dev, x, y, z, d, params.speed, 0, 0)) < 0)
        {
            fprintf(stderr, "Goto position failed - %s\n", um_last_errorstr(handle));
            continue;
        }
        if(!params.loop && !params.verbose)
            break;
        while(um_get_drive_status(handle, params.dev) == LIBUM_POS_DRIVE_BUSY)
        {
            if(params.verbose)
            {
                if(um_get_positions(handle, params.dev, params.update, &x, &y, &z, &d, NULL) < 0)
                    fprintf(stderr, "Get positions failed - %s\n", um_last_errorstr(handle));
                else
                    printf("%3.2f\t%3.2f\t%3.2f\t%3.2f\n", x, y, z, d);
            }
            um_receive(handle, params.update);
       }
	} while(++loop < params.loop);
    um_close(handle);
    exit(!ret);
}
