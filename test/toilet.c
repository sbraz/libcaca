/*
 *  TOIlet        The Other Implementation’s letters
 *  Copyright (c) 2006 Sam Hocevar <sam@zoy.org>
 *                All Rights Reserved
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Do What The Fuck You Want To
 *  Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#include "config.h"
#include "common.h"

#if !defined(__KERNEL__)
#   if defined(HAVE_INTTYPES_H)
#      include <inttypes.h>
#   endif
#   if defined(HAVE_GETOPT_H)
#      include <getopt.h>
#   endif
#   include <stdio.h>
#   include <string.h>
#   include <stdlib.h>
#endif

#include "cucul.h"
#include "caca.h"

/* String to canvas transformations */
static cucul_canvas_t *cuculize_big(uint32_t const *, unsigned int);
static cucul_canvas_t *cuculize_tiny(uint32_t const *, unsigned int);

/* Canvas special effects */
static void make_gay(cucul_canvas_t *);

int main(int argc, char *argv[])
{
    cucul_canvas_t *cv;
    cucul_buffer_t *buffer;

    uint32_t *string = NULL;
    unsigned int length;

    int i;

    char const *export = "utf8";
    unsigned flag_gay = 0;

#if defined(HAVE_GETOPT_H)
    for(;;)
    {
#   ifdef HAVE_GETOPT_LONG
#       define MOREINFO "Try `%s --help' for more information.\n"
        int option_index = 0;
        static struct option long_options[] =
        {
            /* Long option, needs arg, flag, short option */
            { "gay", 0, NULL, 'g' },
            { "irc", 0, NULL, 'i' },
            { "help", 0, NULL, 'h' },
            { "version", 0, NULL, 'v' },
            { NULL, 0, NULL, 0 }
        };

        int c = getopt_long(argc, argv, "gihv", long_options, &option_index);
#   else
#       define MOREINFO "Try `%s -h' for more information.\n"
        int c = getopt(argc, argv, "gihv");
#   endif
        if(c == -1)
            break;

        switch(c)
        {
        case 'h': /* --help */
            printf("Usage: %s [ -gihv ] [ message ]\n", argv[0]);
#   ifdef HAVE_GETOPT_LONG
            printf("  -g, --gay        add a rainbow effect to the text\n");
            printf("  -i, --irc        output IRC colour codes\n");
            printf("  -h, --help       display this help and exit\n");
            printf("  -v, --version    output version information and exit\n");
#   else
            printf("  -g    add a rainbow effect to the text\n");
            printf("  -i    output IRC colour codes\n");
            printf("  -h    display this help and exit\n");
            printf("  -v    output version information and exit\n");
#   endif
            return 0;
        case 'g': /* --gay */
            flag_gay = 1;
            break;
        case 'i': /* --irc */
            export = "irc";
            break;
        case 'v': /* --version */
            printf("TOIlet Copyright 2006 Sam Hocevar %s\n", VERSION);
            printf("Internet: <sam@zoy.org> Version: 0, date: 21 Sep 2006\n");
            printf("\n");
            return 0;
        case '?':
            printf(MOREINFO, argv[0]);
            return 1;
        default:
            printf("%s: invalid option -- %i\n", argv[0], c);
            printf(MOREINFO, argv[0]);
            return 1;
        }
    }
#else
#   define MOREINFO "Usage: %s message...\n"
    int optind = 1;
#endif

    if(optind >= argc)
    {
        printf("%s: too few arguments\n", argv[0]);
        printf(MOREINFO, argv[0]);
        return 1;
    }

    /* Load rest of commandline into a UTF-32 string */
    for(i = optind, length = 0; i < argc; i++)
    {
        unsigned int k, guessed_len, real_len;

        guessed_len = strlen(argv[i]);

        if(i > optind)
            string[length++] = (uint32_t)' ';

        string = realloc(string, (length + guessed_len + 1) * 4);

        for(k = 0, real_len = 0; k < guessed_len; real_len++)
        {
            unsigned int char_len;

            string[length + real_len] =
                cucul_utf8_to_utf32(argv[i] + k, &char_len);

            k += char_len;
        }

        length += real_len;
    }

    /* Do gay stuff with our string (léopard) */
    cv = cuculize_big(string, length);
    if(flag_gay)
        make_gay(cv);

    /* Output char */
    buffer = cucul_export_canvas(cv, export);
    fwrite(cucul_get_buffer_data(buffer),
           cucul_get_buffer_size(buffer), 1, stdout);
    cucul_free_buffer(buffer);

    cucul_free_canvas(cv);

    return 0;
}

static cucul_canvas_t *cuculize_big(uint32_t const *string,
                                    unsigned int length)
{
    cucul_canvas_t *cv;
    cucul_font_t *f;
    char const * const * fonts;
    unsigned char *buf;
    unsigned int w, h, x, y, miny, maxy;

    cv = cucul_create_canvas(length, 1);
    for(x = 0; x < length; x++)
        cucul_putchar(cv, x, 0, string[x]);

    fonts = cucul_get_font_list();
    f = cucul_load_font(fonts[0], 0);

    /* Create our bitmap buffer (32-bit ARGB) */
    w = cucul_get_canvas_width(cv) * cucul_get_font_width(f);
    h = cucul_get_canvas_height(cv) * cucul_get_font_height(f);
    buf = malloc(4 * w * h);

    /* Render the canvas onto our image buffer */
    cucul_render_canvas(cv, f, buf, w, h, 4 * w);

    /* Free our canvas, and allocate a bigger one */
    cucul_free_font(f);
    cucul_free_canvas(cv);
    cv = cucul_create_canvas(w, h);

    /* Render the image buffer on the canvas */
    cucul_set_color(cv, CUCUL_COLOR_WHITE, CUCUL_COLOR_TRANSPARENT);
    cucul_clear_canvas(cv);

    miny = h; maxy = 0;

    for(y = 0; y < h; y++)
       for(x = 0; x < w; x++)
    {
        unsigned char c = buf[4 * (x + y * w) + 2];

        if(c >= 0xa0)
            cucul_putstr(cv, x, y, "█");
        else if(c >= 0x80)
            cucul_putstr(cv, x, y, "▓");
        else if(c >= 0x40)
            cucul_putstr(cv, x, y, "▒");
        else if(c >= 0x20)
            cucul_putstr(cv, x, y, "░");
    }

    free(buf);

    return cv;
}

static cucul_canvas_t *cuculize_tiny(uint32_t const *string,
                                     unsigned int length)
{
    unsigned int x;
    cucul_canvas_t *cv = cucul_create_canvas(length, 1);

    for(x = 0; x < length; x++)
        cucul_putchar(cv, x, 0, string[x]);

    return cv;
}

static void make_gay(cucul_canvas_t *cv)
{
    static unsigned char const rainbow[] =
    {
        CUCUL_COLOR_LIGHTMAGENTA,
        CUCUL_COLOR_LIGHTRED,
        CUCUL_COLOR_YELLOW,
        CUCUL_COLOR_LIGHTGREEN,
        CUCUL_COLOR_LIGHTCYAN,
        CUCUL_COLOR_LIGHTBLUE,
    };
    unsigned int x, y, w, h;

    w = cucul_get_canvas_width(cv);
    h = cucul_get_canvas_height(cv);

    for(y = 0; y < h; y++)
        for(x = 0; x < w; x++)
    {
        unsigned long int ch = cucul_getchar(cv, x, y);
        if(ch != (unsigned char)' ')
        {
            cucul_set_color(cv, rainbow[(x / 2 + y) % 6],
                                CUCUL_COLOR_TRANSPARENT);
            cucul_putchar(cv, x, y, ch);
        }
    }
}
